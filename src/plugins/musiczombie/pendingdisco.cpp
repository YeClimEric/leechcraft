/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2012  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "pendingdisco.h"
#include <memory>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QDomDocument>
#include <QtDebug>
#include <QPointer>
#include <util/queuemanager.h>
#include "artistlookup.h"

namespace LeechCraft
{
namespace MusicZombie
{
	PendingDisco::PendingDisco (Util::QueueManager *queue, const QString& artist,
			const QString& release, QNetworkAccessManager *nam, QObject *parent)
	: QObject (parent)
	, ReleaseName_ (release.toLower ())
	, Queue_ (queue)
	, NAM_ (nam)
	, PendingReleases_ (0)
	{
		Queue_->Schedule ([this, artist, nam] () -> void
			{
				auto idLookup = new ArtistLookup (artist, nam, this);
				connect (idLookup,
						SIGNAL(gotID (QString)),
						this,
						SLOT (handleGotID (QString)));
				connect (idLookup,
						SIGNAL (replyError ()),
						this,
						SLOT (handleIDError ()));
				connect (idLookup,
						SIGNAL (networkError ()),
						this,
						SLOT (handleIDError ()));
			}, this);
	}

	QObject* PendingDisco::GetQObject ()
	{
		return this;
	}

	QList<Media::ReleaseInfo> PendingDisco::GetReleases () const
	{
		return Releases_;
	}

	void PendingDisco::DecrementPending ()
	{
		if (!--PendingReleases_)
		{
			emit ready ();
			deleteLater ();
		}
	}

	void PendingDisco::handleGotID (const QString& id)
	{
		const auto urlStr = "http://musicbrainz.org/ws/2/artist/" + id + "?inc=releases";

		Queue_->Schedule ([this, urlStr] () -> void
			{
				auto reply = NAM_->get (QNetworkRequest (QUrl (urlStr)));
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleLookupFinished ()));
				connect (reply,
						SIGNAL (error (QNetworkReply::NetworkError)),
						this,
						SLOT (handleLookupError ()));
			}, this);
	}

	void PendingDisco::handleIDError ()
	{
		emit error (tr ("Error getting artist MBID."));
		deleteLater ();
	}

	void PendingDisco::handleLookupFinished ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		const auto& data = reply->readAll ();
		QDomDocument doc;
		if (!doc.setContent (data))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to parse"
					<< data;
			emit error (tr ("Unable to parse MusicBrainz reply."));
			deleteLater ();
		}

		QMap<QString, QMap<QString, Media::ReleaseInfo>> infos;

		auto releaseElem = doc
				.documentElement ()
				.firstChildElement ("artist")
				.firstChildElement ("release-list")
				.firstChildElement ("release");
		while (!releaseElem.isNull ())
		{
			std::shared_ptr<void> guard (nullptr,
					[&releaseElem] (void*)
						{ releaseElem = releaseElem.nextSiblingElement ("release"); });

			auto elemText = [&releaseElem] (const QString& sub)
			{
				return releaseElem.firstChildElement (sub).text ();
			};

			if (elemText ("status") != "Official")
				continue;

			const auto& dateStr = elemText ("date");
			const int dashPos = dateStr.indexOf ('-');
			const int date = (dashPos > 0 ? dateStr.left (dashPos) : dateStr).toInt ();
			if (date < 1000)
				continue;

			const auto& title = elemText ("title");
			if (!ReleaseName_.isEmpty () && title.toLower () != ReleaseName_)
				continue;

			infos [title] [elemText ("country")] =
			{
				releaseElem.attribute ("id"),
				title,
				date,
				Media::ReleaseInfo::Type::Standard,
				QList<QList<Media::ReleaseTrackInfo>> ()
			};
		}

		for (const auto& key : infos.keys ())
		{
			const auto& countries = infos [key];
			const auto& release = countries.contains ("US") ?
					countries ["US"] :
					countries.values ().first ();
			Releases_ << release;

			++PendingReleases_;

			const auto urlStr = "http://musicbrainz.org/ws/2/release/" + release.ID_ + "?inc=recordings";

			Queue_->Schedule ([this, urlStr] () -> void
				{
					auto reply = NAM_->get (QNetworkRequest (QUrl (urlStr)));
					connect (reply,
							SIGNAL (finished ()),
							this,
							SLOT (handleReleaseLookupFinished ()));
					connect (reply,
							SIGNAL (error (QNetworkReply::NetworkError)),
							this,
							SLOT (handleReleaseLookupError ()));
				}, this);
		}

		std::sort (Releases_.begin (), Releases_.end (),
				[] (decltype (Releases_.at (0)) left, decltype (Releases_.at (0)) right)
					{ return left.Year_ < right.Year_; });
	}

	void PendingDisco::handleLookupError ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		qWarning () << Q_FUNC_INFO
				<< "error looking stuff up"
				<< reply->errorString ();
		emit error (tr ("Error performing artist lookup: %1.")
					.arg (reply->errorString ()));
	}

	void PendingDisco::handleReleaseLookupFinished ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		std::shared_ptr<void> decrementGuard (nullptr, [this] (void*) { DecrementPending (); });

		const auto& data = reply->readAll ();
		QDomDocument doc;
		if (!doc.setContent (data))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to parse"
					<< data;
		}

		const auto& releaseElem = doc.documentElement ().firstChildElement ("release");
		const auto& id = releaseElem.attribute ("id");
		auto pos = std::find_if (Releases_.begin (), Releases_.end (),
				[id] (decltype (Releases_.at (0)) release)
					{ return release.ID_ == id; });
		if (pos == Releases_.end ())
		{
			qWarning () << Q_FUNC_INFO
					<< "release"
					<< id
					<< "not found";
			return;
		}

		auto& release = *pos;
		auto mediumElem = releaseElem.firstChildElement ("medium-list").firstChildElement ("medium");
		while (!mediumElem.isNull ())
		{
			auto trackElem = mediumElem.firstChildElement ("track-list").firstChildElement ("track");

			QList<Media::ReleaseTrackInfo> tracks;
			while (!trackElem.isNull ())
			{
				const int num = trackElem.firstChildElement ("number").text ().toInt ();

				const auto& recElem = trackElem.firstChildElement ("recording");
				const auto& title = recElem.firstChildElement ("title").text ();
				const int length = recElem.firstChildElement ("length").text ().toInt () / 1000;

				tracks.push_back ({ num, title, length });
				trackElem = trackElem.nextSiblingElement ("track");
			}

			release.TrackInfos_ << tracks;

			mediumElem = mediumElem.nextSiblingElement ("medium");
		}
	}

	void PendingDisco::handleReleaseLookupError ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		qWarning () << Q_FUNC_INFO
				<< "error looking release stuff up"
				<< reply->errorString ();
		DecrementPending ();
	}
}
}
