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

#include "trackerschanger.h"
#include <QMessageBox>
#include <QMainWindow>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/irootwindowsmanager.h>
#include "core.h"
#include "singletrackerchanger.h"

namespace LeechCraft
{
	namespace Plugins
	{
		namespace BitTorrent
		{
			TrackersChanger::TrackersChanger (QWidget *parent)
			: QDialog (parent)
			{
				Ui_.setupUi (this);

				connect (Ui_.Trackers_,
						SIGNAL (currentItemChanged (QTreeWidgetItem*, QTreeWidgetItem*)),
						this,
						SLOT (currentItemChanged (QTreeWidgetItem*)));
				currentItemChanged (0);
			}

			void TrackersChanger::SetTrackers (const std::vector<libtorrent::announce_entry>& trackers)
			{
				Ui_.Trackers_->clear ();
				Q_FOREACH (const auto& tracker, trackers)
				{
					QStringList strings;
					bool torrent, client, magnet, tex;
					torrent = tracker.source & libtorrent::announce_entry::source_torrent;
					client = tracker.source & libtorrent::announce_entry::source_client;
					magnet = tracker.source & libtorrent::announce_entry::source_magnet_link;
					tex = tracker.source & libtorrent::announce_entry::source_tex;
					strings << QString::fromUtf8 (tracker.url.c_str ())
						<< QString::number (tracker.tier)
						<< tr ("%1 s").arg (tracker.next_announce_in ())
						<< QString::number (tracker.fails)
						<< QString::number (tracker.fail_limit)
						<< (tracker.verified ? tr ("true") : tr ("false"))
						<< (tracker.updating ? tr ("true") : tr ("false"))
						<< (tracker.start_sent ? tr ("true") : tr ("false"))
						<< (tracker.complete_sent ? tr ("true") : tr ("false"))
						<< (torrent ? tr ("true") : tr ("false"))
						<< (client ? tr ("true") : tr ("false"))
						<< (magnet ? tr ("true") : tr ("false"))
						<< (tex ? tr ("true") : tr ("false"));
					Ui_.Trackers_->addTopLevelItem (new QTreeWidgetItem (strings));
				}
				for (int i = 0; i < Ui_.Trackers_->columnCount (); ++i)
					Ui_.Trackers_->resizeColumnToContents (i);
			}

			std::vector<libtorrent::announce_entry> TrackersChanger::GetTrackers () const
			{
				const int count = Ui_.Trackers_->topLevelItemCount ();
				std::vector<libtorrent::announce_entry> result (count, std::string ());
				for (int i = 0; i < count; ++i)
				{
					QTreeWidgetItem *item = Ui_.Trackers_->topLevelItem (i);
					result [i].url = (item->text (0).toStdString ());
					result [i].tier = item->text (1).toInt ();
				}
				return result;
			}

			void TrackersChanger::currentItemChanged (QTreeWidgetItem *current)
			{
				Ui_.ButtonModify_->setEnabled (current);
				Ui_.ButtonRemove_->setEnabled (current);
			}

			void TrackersChanger::on_ButtonAdd__released ()
			{
				SingleTrackerChanger dia (this);
				if (dia.exec () != QDialog::Accepted)
					return;

				QStringList strings;
				strings << dia.GetTracker ()
					<< QString::number (dia.GetTier ());
				while (strings.size () < Ui_.Trackers_->columnCount ())
					strings << QString ();
				Ui_.Trackers_->addTopLevelItem (new QTreeWidgetItem (strings));
			}

			void TrackersChanger::on_ButtonModify__released ()
			{
				QTreeWidgetItem *current = Ui_.Trackers_->currentItem ();
				if (!current)
					return;

				SingleTrackerChanger dia (this);
				dia.SetTracker (current->text (0));
				dia.SetTier (current->text (1).toInt ());
				if (dia.exec () != QDialog::Accepted)
					return;

				QStringList strings;
				strings << dia.GetTracker ()
					<< QString::number (dia.GetTier ());
				while (strings.size () < Ui_.Trackers_->columnCount ())
					strings << QString ();

				int idx = Ui_.Trackers_->indexOfTopLevelItem (current);
				Ui_.Trackers_->insertTopLevelItem (idx, new QTreeWidgetItem (strings));
				delete Ui_.Trackers_->takeTopLevelItem (idx);
			}

			void TrackersChanger::on_ButtonRemove__released ()
			{
				QTreeWidgetItem *current = Ui_.Trackers_->currentItem ();
				if (!current)
					return;

				auto rootWM = Core::Instance ()->GetProxy ()->GetRootWindowsManager ();
				if (QMessageBox::question (rootWM->GetPreferredWindow (),
							tr ("Confirm tracker removal"),
							tr ("Are you sure you want to remove the "
								"following tracker:<br />%1")
								.arg (current->text (0)),
							QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
					delete current;
			}
		};
	};
};

