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

#include "syncunmountablemanager.h"
#include <interfaces/lmp/iunmountablesync.h>
#include "core.h"
#include "localcollection.h"

namespace LeechCraft
{
namespace LMP
{
	SyncUnmountableManager::SyncUnmountableManager (QObject *parent)
	: SyncManagerBase (parent)
	, CopyMgr_ (new CopyManager<CopyJob> (this))
	{
		connect (CopyMgr_,
				SIGNAL (startedCopying (QString)),
				this,
				SLOT (handleStartedCopying (QString)));
		connect (CopyMgr_,
				SIGNAL (finishedCopying ()),
				this,
				SLOT (handleFinishedCopying ()));
	}

	void SyncUnmountableManager::AddFiles (const AddFilesParams& params)
	{
		auto coll = Core::Instance ().GetLocalCollection ();

		const auto& format = params.TCParams_.FormatID_;

		auto syncer = params.Syncer_;
		for (const auto& file : params.Files_)
		{
			const auto trackId = coll->FindTrack (file);
			if (trackId < 0)
				continue;

			const auto trackNumber = coll->GetTrackData (trackId, LocalCollection::Role::TrackNumber).toInt ();
			const auto& trackTitle = coll->GetTrackData (trackId, LocalCollection::Role::TrackTitle).toString ();

			const auto album = coll->GetTrackAlbum (trackId);
			if (!album)
				continue;

			const auto& artists = coll->GetAlbumArtists (album->ID_);
			if (artists.isEmpty ())
				continue;

			const auto& artist = coll->GetArtist (artists.at (0));

			syncer->SetFileInfo (file,
					{
						format.isEmpty () ?
							QFileInfo (file).suffix ().toLower () :
							format,
						trackNumber,
						trackTitle,
						artist.Name_,
						album->Name_,
						album->Year_,
						album->CoverPath_,
						QStringList ()
					});

			Source2Params_ [file] = params;
		}

		SyncManagerBase::AddFiles (params.Files_, params.TCParams_);
	}

	void SyncUnmountableManager::handleFileTranscoded (const QString& from, const QString& transcoded, QString)
	{
		SyncManagerBase::HandleFileTranscoded (from, transcoded);

		const auto& params = Source2Params_.take (from);
		if (!params.Syncer_)
		{
			qWarning () << Q_FUNC_INFO
					<< "no syncer for file"
					<< from;
			return;
		}

		const CopyJob copyJob
		{
			transcoded,
			from != transcoded,
			params.Syncer_,
			params.DevID_,
			params.StorageID_,
			from
		};
		CopyMgr_->Copy (copyJob);
	}
}
}
