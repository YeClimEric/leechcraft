/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "pendingartistbio.h"
#include <algorithm>
#include <QNetworkReply>
#include <QDomDocument>
#include <QtDebug>
#include "util.h"

namespace LeechCraft
{
namespace Lastfmscrobble
{
	PendingArtistBio::PendingArtistBio (const QString& name,
			QNetworkAccessManager *nam, QObject *parent)
	: QObject (parent)
	{
		QMap<QString, QString> params;
		params ["artist"] = name;
		params ["autocorrect"] = "1";
		AddLanguageParam (params);
		auto reply = Request ("artist.getInfo", nam, params);
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleError ()));
	}

	QObject* PendingArtistBio::GetQObject ()
	{
		return this;
	}

	Media::ArtistBio PendingArtistBio::GetArtistBio () const
	{
		return Bio_;
	}

	void PendingArtistBio::handleFinished ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		QDomDocument doc;
		if (!doc.setContent (reply->readAll ()))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to parse reply";
			emit error ();
			deleteLater ();
			return;
		}

		const auto& artist = doc.documentElement ().firstChildElement ("artist");
		Bio_.BasicInfo_ = GetArtistInfo (artist);
		std::reverse (Bio_.BasicInfo_.Tags_.begin (), Bio_.BasicInfo_.Tags_.end ());

		emit ready ();
		deleteLater ();
	}

	void PendingArtistBio::handleError ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		qWarning () << Q_FUNC_INFO
				<< reply->errorString ();
		reply->deleteLater ();
		deleteLater ();

		emit error ();
	}
}
}
