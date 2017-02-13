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

#include "localcollection.h"
#include <functional>
#include <algorithm>
#include <numeric>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QtConcurrentMap>
#include <QtConcurrentRun>
#include <QTimer>
#include <QtDebug>
#include <util/sll/either.h>
#include <util/xpc/util.h>
#include "localcollectionstorage.h"
#include "core.h"
#include "util.h"
#include "localfileresolver.h"
#include "player.h"
#include "albumartmanager.h"
#include "xmlsettingsmanager.h"
#include "localcollectionwatcher.h"
#include "localcollectionmodel.h"

namespace LeechCraft
{
namespace LMP
{
	LocalCollection::LocalCollection (QObject *parent)
	: QObject (parent)
	, IsReady_ (false)
	, Storage_ (new LocalCollectionStorage (this))
	, CollectionModel_ (new LocalCollectionModel (this))
	, FilesWatcher_ (new LocalCollectionWatcher (this))
	, AlbumArtMgr_ (new AlbumArtManager (this))
	, Watcher_ (new QFutureWatcher<MediaInfo> (this))
	, UpdateNewArtists_ (0)
	, UpdateNewAlbums_ (0)
	, UpdateNewTracks_ (0)
	{
		connect (Watcher_,
				SIGNAL (finished ()),
				this,
				SLOT (handleScanFinished ()));
		connect (Watcher_,
				SIGNAL (progressValueChanged (int)),
				this,
				SIGNAL (scanProgressChanged (int)));

		auto loadWatcher = new QFutureWatcher<LocalCollectionStorage::LoadResult> (this);
		connect (loadWatcher,
				SIGNAL (finished ()),
				this,
				SLOT (handleLoadFinished ()));
		auto future = QtConcurrent::run ([] { return LocalCollectionStorage ().Load (); });
		loadWatcher->setFuture (future);

		auto& xsd = XmlSettingsManager::Instance ();
		QStringList oldDefault (xsd.property ("CollectionDir").toString ());
		oldDefault.removeAll (QString ());
		AddRootPaths (xsd.Property ("RootCollectionPaths", oldDefault).toStringList ());
		connect (this,
				SIGNAL (rootPathsChanged (QStringList)),
				this,
				SLOT (saveRootPaths ()));
	}

	bool LocalCollection::IsReady () const
	{
		return IsReady_;
	}

	AlbumArtManager* LocalCollection::GetAlbumArtManager () const
	{
		return AlbumArtMgr_;
	}

	LocalCollectionStorage* LocalCollection::GetStorage () const
	{
		return Storage_;
	}

	QAbstractItemModel* LocalCollection::GetCollectionModel () const
	{
		return CollectionModel_;
	}

	QVariant LocalCollection::GetTrackData (int trackId, LocalCollectionModel::Role role) const
	{
		return CollectionModel_->GetTrackData (trackId, role);
	}

	void LocalCollection::Clear ()
	{
		Storage_->Clear ();
		CollectionModel_->Clear ();
		Artists_.clear ();
		PresentPaths_.clear ();

		Path2Track_.clear ();
		Track2Path_.clear ();

		Track2Album_.clear ();
		AlbumID2Album_.clear ();
		AlbumID2ArtistID_.clear ();

		RemoveRootPaths (RootPaths_);
	}

	namespace
	{
		struct IterateResult
		{
			QSet<QString> UnchangedFiles_;
			QSet<QString> ChangedFiles_;
		};
	}

	void LocalCollection::Scan (const QString& path, bool root)
	{
		auto watcher = new QFutureWatcher<IterateResult> (this);
		connect (watcher,
				SIGNAL (finished ()),
				this,
				SLOT (handleIterateFinished ()));
		watcher->setProperty ("Path", path);

		if (root)
			AddRootPaths ({ path });

		const bool symLinks = XmlSettingsManager::Instance ()
				.property ("FollowSymLinks").toBool ();
		auto worker = [path, symLinks] () -> IterateResult
		{
			IterateResult result;

			LocalCollectionStorage storage;

			const auto& paths = storage.GetTracksPaths ();

			const auto& allInfos = RecIterateInfo (path, symLinks);
			for (const auto& info : allInfos)
			{
				const auto& trackPath = info.absoluteFilePath ();
				const auto& mtime = info.lastModified ();
				try
				{
					const auto& storedDt = storage.GetMTime (trackPath);
					if (storedDt.isValid () &&
							std::abs (storedDt.msecsTo (mtime)) < 1500)
					{
						result.UnchangedFiles_ << trackPath;
						continue;
					}
				}
				catch (const std::exception& e)
				{
					qWarning () << Q_FUNC_INFO
							<< "error getting mtime"
							<< trackPath
							<< e.what ();
				}

				if (paths.contains (trackPath))
				{
					try
					{
						storage.SetMTime (trackPath, mtime);
					}
					catch (const std::exception& e)
					{
						qWarning () << Q_FUNC_INFO
								<< "error setting mtime"
								<< trackPath
								<< e.what ();
					}
				}
				result.ChangedFiles_ << trackPath;
			}

			return result;
		};
		watcher->setFuture (QtConcurrent::run (worker));
	}

	void LocalCollection::Unscan (const QString& path)
	{
		if (!RootPaths_.contains (path))
			return;

		QStringList toRemove;
		auto pred = [&path] (const QString& subPath) { return subPath.startsWith (path); };
		std::copy_if (PresentPaths_.begin (), PresentPaths_.end (),
				std::back_inserter (toRemove), pred);
		PresentPaths_.subtract (QSet<QString>::fromList (toRemove));

		try
		{
			std::for_each (toRemove.begin (), toRemove.end (),
					[this] (const QString& path) { RemoveTrack (path); });
		}
		catch (const std::runtime_error& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "error unscanning"
					<< path
					<< e.what ();
			return;
		}

		RemoveRootPaths (QStringList (path));
	}

	void LocalCollection::Rescan ()
	{
		auto paths = RootPaths_;
		Clear ();

		for (const auto& path : paths)
			Scan (path, true);
	}

	LocalCollection::DirStatus LocalCollection::GetDirStatus (const QString& dir) const
	{
		if (RootPaths_.contains (dir))
			return DirStatus::RootPath;

		auto pos = std::find_if (RootPaths_.begin (), RootPaths_.end (),
				[&dir] (const auto& root) { return dir.startsWith (root); });
		return pos == RootPaths_.end () ?
				DirStatus::None :
				DirStatus::SubPath;
	}

	QStringList LocalCollection::GetDirs () const
	{
		return RootPaths_;
	}

	int LocalCollection::FindArtist (const QString& artist) const
	{
		auto artistPos = std::find_if (Artists_.begin (), Artists_.end (),
				[&artist] (const auto& item) { return !QString::compare (item.Name_, artist, Qt::CaseInsensitive); });
		return artistPos == Artists_.end () ?
			-1 :
			artistPos->ID_;
	}

	int LocalCollection::FindAlbum (const QString& artist, const QString& album) const
	{
		auto artistPos = std::find_if (Artists_.begin (), Artists_.end (),
				[&artist] (const auto& item) { return !QString::compare (item.Name_, artist, Qt::CaseInsensitive); });
		if (artistPos == Artists_.end ())
		{
			qWarning () << Q_FUNC_INFO
					<< "artist not found:"
					<< artist
					<< album;
			return -1;
		}

		const auto& albums = artistPos->Albums_;
		auto albumPos = std::find_if (albums.begin (), albums.end (),
				[&album] (const auto& item) { return !QString::compare (item->Name_, album, Qt::CaseInsensitive); });
		if (albumPos == albums.end ())
		{
			qWarning () << Q_FUNC_INFO
					<< "album not found:"
					<< artist
					<< album;
			return -1;
		}

		return (*albumPos)->ID_;
	}

	void LocalCollection::SetAlbumArt (int id, const QString& path)
	{
		CollectionModel_->SetAlbumArt (id, path);

		if (AlbumID2Album_.contains (id))
			AlbumID2Album_ [id]->CoverPath_ = path;

		Storage_->SetAlbumArt (id, path);
	}

	Collection::Album_ptr LocalCollection::GetAlbum (int albumId) const
	{
		return AlbumID2Album_ [albumId];
	}

	int LocalCollection::FindTrack (const QString& path) const
	{
		return Path2Track_.value (path, -1);
	}

	int LocalCollection::GetTrackAlbumId (int trackId) const
	{
		return Track2Album_ [trackId];
	}

	Collection::Album_ptr LocalCollection::GetTrackAlbum (int trackId) const
	{
		return AlbumID2Album_ [Track2Album_ [trackId]];
	}

	QList<int> LocalCollection::GetDynamicPlaylist (DynamicPlaylist type) const
	{
		QList<int> result;
		switch (type)
		{
		case DynamicPlaylist::Random50:
		{
			const auto& keys = Track2Path_.keys ();
			if (keys.isEmpty ())
				return {};
			for (int i = 0; i < 50; ++i)
				result << keys [qrand () % keys.size ()];
			break;
		}
		case DynamicPlaylist::LovedTracks:
			result = Storage_->GetLovedTracks ();
			break;
		case DynamicPlaylist::BannedTracks:
			result = Storage_->GetBannedTracks ();
			break;
		}
		return result;
	}

	QStringList LocalCollection::TrackList2PathList (const QList<int>& tracks) const
	{
		QStringList result;
		std::transform (tracks.begin (), tracks.end (), std::back_inserter (result),
				[this] (int id) { return Track2Path_ [id]; });
		result.removeAll (QString ());
		return result;
	}

	void LocalCollection::AddTrackTo (int trackId, StaticRating rating)
	{
		switch (rating)
		{
		case StaticRating::Loved:
			Storage_->SetTrackLoved (trackId);
			break;
		case StaticRating::Banned:
			Storage_->SetTrackBanned (trackId);
			break;
		}
	}

	Collection::TrackStats LocalCollection::GetTrackStats (const QString& path) const
	{
		if (!Path2Track_.contains (path))
			return Collection::TrackStats ();

		try
		{
			return Storage_->GetTrackStats (Path2Track_ [path]);
		}
		catch (const std::runtime_error& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "error fetching stats for track"
					<< path
					<< Path2Track_ [path]
					<< e.what ();
			return Collection::TrackStats ();
		}
	}

	QList<int> LocalCollection::GetAlbumArtists (int albumId) const
	{
		QList<int> result;
		for (const auto& artist : Artists_)
		{
			if (std::any_of (artist.Albums_.begin (), artist.Albums_.end (),
					[albumId] (const auto& album) { return album->ID_ == albumId; }))
				result << artist.ID_;
		}
		return result;
	}

	Collection::Artist LocalCollection::GetArtist (int id) const
	{
		auto pos = std::find_if (Artists_.begin (), Artists_.end (),
				[id] (const auto& artist) { return artist.ID_ == id; });
		return pos != Artists_.end () ?
				*pos :
				Collection::Artist ();
	}

	Collection::Artists_t LocalCollection::GetAllArtists () const
	{
		return Artists_;
	}

	void LocalCollection::HandleExistingInfos (const QList<MediaInfo>& infos)
	{
		for (const auto& info : infos)
		{
			const auto& path = info.LocalPath_;
			const auto trackIdx = FindTrack (path);
			const auto trackAlbum = GetTrackAlbum (trackIdx);
			if (!trackAlbum)
			{
				qWarning () << Q_FUNC_INFO
						<< "no album for track"
						<< path;
				continue;
			}

			const auto pos = std::find_if (trackAlbum->Tracks_.begin (), trackAlbum->Tracks_.end (),
					[trackIdx] (const auto& track) { return track.ID_ == trackIdx; });
			const auto& track = pos != trackAlbum->Tracks_.end () ?
					*pos :
					Collection::Track ();
			const auto& artist = GetArtist (AlbumID2ArtistID_ [trackAlbum->ID_]);
			if (artist.Name_ == info.Artist_ &&
					trackAlbum->Name_ == info.Album_ &&
					trackAlbum->Year_ == info.Year_ &&
					track.Number_ == info.TrackNumber_ &&
					track.Name_ == info.Title_ &&
					track.Genres_ == info.Genres_)
				continue;

			auto stats = GetTrackStats (path);
			RemoveTrack (path);

			const auto& newArts = Storage_->AddToCollection ({ info });
			HandleNewArtists (newArts);

			const auto newTrackIdx = FindTrack (path);
			stats.TrackID_ = newTrackIdx;
			Storage_->SetTrackStats (stats);
		}
	}

	void LocalCollection::HandleNewArtists (const Collection::Artists_t& artists)
	{
		int albumCount = 0;
		int trackCount = 0;
		const bool shouldEmit = !Artists_.isEmpty ();

		for (const auto& artist : artists)
		{
			const auto pos = std::find_if (Artists_.begin (), Artists_.end (),
					[&artist] (const auto& present) { return present.ID_ == artist.ID_; });
			if (pos == Artists_.end ())
			{
				const auto pos = std::lower_bound (Artists_.begin (), Artists_.end (), artist,
						[] (const Collection::Artist& a1, const Collection::Artist& a2)
						{
							return CompareArtists (a1.Name_, a2.Name_,
									!XmlSettingsManager::Instance ()
										.property ("SortWithThe").toBool ());
						});
				Artists_.insert (pos, artist);
			}
			else
				pos->Albums_ << artist.Albums_;

			for (const auto& album : artist.Albums_)
				for (const auto& track : album->Tracks_)
					PresentPaths_ << track.FilePath_;
		}

		const auto autoFetchAA = XmlSettingsManager::Instance ()
				.property ("AutoFetchAlbumArt").toBool ();
		for (const auto& artist : artists)
		{
			albumCount += artist.Albums_.size ();
			for (auto album : artist.Albums_)
			{
				trackCount += album->Tracks_.size ();

				if (autoFetchAA)
					AlbumArtMgr_->CheckAlbumArt (artist, album);

				if (AlbumID2Album_.contains (album->ID_))
					AlbumID2Album_ [album->ID_]->Tracks_ << album->Tracks_;
				else
				{
					AlbumID2Album_ [album->ID_] = album;
					AlbumID2ArtistID_ [album->ID_] = artist.ID_;
				}

				for (const auto& track : album->Tracks_)
				{
					Path2Track_ [track.FilePath_] = track.ID_;
					Track2Path_ [track.ID_] = track.FilePath_;

					Track2Album_ [track.ID_] = album->ID_;
				}
			}
		}

		CollectionModel_->AddArtists (artists);

		if (shouldEmit &&
				trackCount)
		{
			UpdateNewArtists_ += artists.size ();
			UpdateNewAlbums_ += albumCount;
			UpdateNewTracks_ += trackCount;
		}
	}

	void LocalCollection::RemoveTrack (const QString& path)
	{
		const int id = FindTrack (path);
		if (id == -1)
			return;

		auto album = GetTrackAlbum (id);
		try
		{
			Storage_->RemoveTrack (id);
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "error removing track:"
					<< e.what ();
			throw;
		}

		CollectionModel_->RemoveTrack (id);

		Path2Track_.remove (path);
		Track2Path_.remove (id);
		Track2Album_.remove (id);
		PresentPaths_.remove (path);

		if (!album)
			return;

		auto pos = std::remove_if (album->Tracks_.begin (), album->Tracks_.end (),
				[id] (const auto& item) { return item.ID_ == id; });
		album->Tracks_.erase (pos, album->Tracks_.end ());

		if (album->Tracks_.isEmpty ())
			RemoveAlbum (album->ID_);
	}

	void LocalCollection::RemoveAlbum (int id)
	{
		try
		{
			Storage_->RemoveAlbum (id);
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "error removing album:"
					<< e.what ();
			throw;
		}

		AlbumID2Album_.remove (id);
		AlbumID2ArtistID_.remove (id);

		CollectionModel_->RemoveAlbum (id);

		for (auto i = Artists_.begin (); i != Artists_.end (); )
		{
			auto& artist = *i;

			auto pos = std::find_if (artist.Albums_.begin (), artist.Albums_.end (),
					[id] (const auto& album) { return album->ID_ == id; });
			if (pos == artist.Albums_.end ())
			{
				++i;
				continue;
			}

			artist.Albums_.erase (pos);
			if (artist.Albums_.isEmpty ())
				i = RemoveArtist (i);
			else
				++i;
		}
	}

	Collection::Artists_t::iterator LocalCollection::RemoveArtist (Collection::Artists_t::iterator pos)
	{
		const int id = pos->ID_;
		try
		{
			Storage_->RemoveArtist (id);
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "error removing artist:"
					<< e.what ();
			throw;
		}

		CollectionModel_->RemoveArtist (id);
		return Artists_.erase (pos);
	}

	void LocalCollection::AddRootPaths (QStringList paths)
	{
		for (const auto& path : RootPaths_)
			paths.removeAll (path);
		if (paths.isEmpty ())
			return;

		RootPaths_ << paths;
		emit rootPathsChanged (RootPaths_);

		for (const auto& path : paths)
			FilesWatcher_->AddPath (path);
	}

	void LocalCollection::RemoveRootPaths (const QStringList& paths)
	{
		int removed = 0;
		for (const auto& str : paths)
		{
			removed += RootPaths_.removeAll (str);
			FilesWatcher_->RemovePath (str);
		}

		if (removed)
			emit rootPathsChanged (RootPaths_);
	}

	void LocalCollection::CheckRemovedFiles (const QSet<QString>& scanned, const QString& rootPath)
	{
		auto toRemove = PresentPaths_;
		toRemove.subtract (scanned);

		for (auto pos = toRemove.begin (); pos != toRemove.end (); )
		{
			if (pos->startsWith (rootPath))
				++pos;
			else
				pos = toRemove.erase (pos);
		}

		for (const auto& path : toRemove)
			RemoveTrack (path);
	}

	void LocalCollection::InitiateScan (const QSet<QString>& newPaths)
	{
		auto resolver = Core::Instance ().GetLocalFileResolver ();

		emit scanStarted (newPaths.size ());
		auto worker = [resolver] (const QString& path)
		{
			return resolver->ResolveInfo (path).ToRight ([] (const ResolveError& error)
					{
						qWarning () << Q_FUNC_INFO
								<< "error resolving media info for"
								<< error.FilePath_
								<< error.ReasonString_;
						return MediaInfo {};
					});
		};
		const auto& future = QtConcurrent::mapped (newPaths,
				std::function<MediaInfo (QString)> (worker));
		Watcher_->setFuture (future);
	}

	void LocalCollection::RecordPlayedTrack (const QString& path)
	{
		if (Path2Track_.contains (path))
			RecordPlayedTrack (Path2Track_ [path]);
	}

	void LocalCollection::RecordPlayedTrack (int trackId)
	{
		try
		{
			Storage_->RecordTrackPlayed (trackId);
		}
		catch (const std::runtime_error& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "error recording played info for track"
					<< e.what ();
		}
	}

	void LocalCollection::rescanOnLoad ()
	{
		for (const auto& rootPath : RootPaths_)
			Scan (rootPath, true);
	}

	void LocalCollection::handleLoadFinished ()
	{
		auto watcher = dynamic_cast<QFutureWatcher<LocalCollectionStorage::LoadResult>*> (sender ());
		watcher->deleteLater ();
		const auto& result = watcher->result ();
		Storage_->Load (result);

		HandleNewArtists (result.Artists_);

		IsReady_ = true;

		emit collectionReady ();

		QTimer::singleShot (5000,
				this,
				SLOT (rescanOnLoad ()));
	}

	void LocalCollection::handleIterateFinished ()
	{
		sender ()->deleteLater ();

		const auto& path = sender ()->property ("Path").toString ();

		auto watcher = dynamic_cast<QFutureWatcher<IterateResult>*> (sender ());
		const auto& result = watcher->result ();

		CheckRemovedFiles (result.ChangedFiles_ + result.UnchangedFiles_, path);

		if (Watcher_->isRunning ())
			NewPathsQueue_ << result.ChangedFiles_;
		else
			InitiateScan (result.ChangedFiles_);
	}

	void LocalCollection::handleScanFinished ()
	{
		auto future = Watcher_->future ();
		QList<MediaInfo> newInfos, existingInfos;
		for (const auto& info : future)
		{
			const auto& path = info.LocalPath_;
			if (path.isEmpty ())
				continue;

			if (PresentPaths_.contains (path))
				existingInfos << info;
			else
			{
				newInfos << info;
				PresentPaths_ += path;
			}
		}

		emit scanFinished ();

		auto newArts = Storage_->AddToCollection (newInfos);
		HandleNewArtists (newArts);

		if (!NewPathsQueue_.isEmpty ())
			InitiateScan (NewPathsQueue_.takeFirst ());
		else if (UpdateNewTracks_)
		{
			const auto& artistsMsg = tr ("%n new artist(s)", 0, UpdateNewArtists_);
			const auto& albumsMsg = tr ("%n new album(s)", 0, UpdateNewAlbums_);
			const auto& tracksMsg = tr ("%n new track(s)", 0, UpdateNewTracks_);
			const auto& msg = tr ("Local collection updated: %1, %2, %3.")
					.arg (artistsMsg)
					.arg (albumsMsg)
					.arg (tracksMsg);
			Core::Instance ().SendEntity (Util::MakeNotification ("LMP", msg, PInfo_));

			UpdateNewArtists_ = UpdateNewAlbums_ = UpdateNewTracks_ = 0;
		}

		HandleExistingInfos (existingInfos);
	}

	void LocalCollection::saveRootPaths ()
	{
		XmlSettingsManager::Instance ().setProperty ("RootCollectionPaths", RootPaths_);
	}
}
}
