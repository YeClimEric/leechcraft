/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
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

#include "lastfmradiotuner.h"
#include <QtDebug>
#include "util.h"

namespace LeechCraft
{
namespace Lastfmscrobble
{
	LastFmRadioTuner::LastFmRadioTuner (const lastfm::RadioStation& station,
			QNetworkAccessManager *nam, QObject *parent)
	: QObject (parent)
	, NAM_ (nam)
	, NumTries_ (0)
	{
		QList<QPair<QString, QString>> params;
		params << QPair<QString, QString> ("station", station.url ());
		auto reply = Request ("radio.tune", NAM_, params);
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleTuned ()));
	}

	lastfm::Track LastFmRadioTuner::GetNextTrack ()
	{
		lastfm::Track result;
		if (!Queue_.isEmpty ())
			result = Queue_.takeFirst ();
		if (Queue_.isEmpty ())
			FetchMoreTracks ();
		return result;
	}

	void LastFmRadioTuner::FetchMoreTracks ()
	{
		QList<QPair<QString, QString>> params;
		params << QPair<QString, QString> ("rtp", "1");
		auto reply = Request ("radio.getPlaylist", NAM_, params);
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleGotPlaylist ()));
	}

	bool LastFmRadioTuner::TryAgain ()
	{
		if (++NumTries_ > 5)
			return false;

		FetchMoreTracks ();
		return true;
	}

	void LastFmRadioTuner::handleTuned ()
	{
		sender ()->deleteLater ();
	}

	void LastFmRadioTuner::handleGotPlaylist ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		const auto& data = reply->readAll ();
		qDebug () << data;
		QDomDocument doc;
		if (!doc.setContent (data))
		{
			qWarning () << Q_FUNC_INFO
					<< "error parsing playlist";
			emit error ("Parse error");
		}

		const auto& docElem = doc.documentElement ();
		if (docElem.attribute ("status") == "failed")
		{
			const auto& errElem = docElem.firstChildElement ("error");
			qWarning () << Q_FUNC_INFO
					<< errElem.text ();
			emit error (errElem.text ());
			return;
		}

		try
		{
#if LASTFM_VERSION < 0x00010000
			lastfm::Xspf xspf (doc.documentElement ().firstChildElement ("playlist"));
			auto tracks = xspf.tracks ();
#else
			lastfm::Xspf xspf (doc.documentElement ().firstChildElement ("playlist"), this);
			QList<lastfm::Track> tracks;
			while (!xspf.isEmpty ())
				tracks << xspf.takeFirst ();
#endif
			if (tracks.isEmpty ())
			{
				qWarning () << Q_FUNC_INFO << "no tracks";
				throw lastfm::ws::TryAgainLater;
			}

			NumTries_ = 0;

			Q_FOREACH (auto track, tracks)
				lastfm::MutableTrack (track).setSource (lastfm::Track::LastFmRadio);

			Queue_ += tracks;
			emit trackAvailable ();
		}
		catch (const lastfm::ws::ParseError& e)
		{
#if LASTFM_VERSION < 0x00010000
			qWarning () << Q_FUNC_INFO << e.what ();
			if (e.enumValue () != lastfm::ws::TryAgainLater)
				emit error (e.what ());
#else
			qWarning () << Q_FUNC_INFO << e.message ();
			if (e.enumValue () != lastfm::ws::TryAgainLater)
				emit error (e.message ());
#endif
			if (!TryAgain ())
				emit error ("out of tries");
		}
		catch (const lastfm::ws::Error& e)
		{
			qWarning () << Q_FUNC_INFO << e;
		}
	}
}
}