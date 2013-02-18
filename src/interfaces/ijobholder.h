/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2012  Georg Rudoy
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

#ifndef INTERFACES_IJOBHOLDER_H
#define INTERFACES_IJOBHOLDER_H
#include <QtPlugin>
#include "interfaces/structures.h"

class QModelIndex;
class QAbstractItemModel;
class QWidget;

namespace LeechCraft
{
	enum JobHolderColumn
	{
		JobName,
		JobStatus,
		JobProgress
	};

	/** Values of this enum are used to describe the semantics of rows
	 * in the representation models.
	 *
	 * Values of this enum are expected to be obtained via the
	 * CustomDataRoles::RoleJobHolderRow role.
	 */
	enum JobHolderRow
	{
		/** This row corresponds to something that cannot be described
		 * by other enum members.
		 */
		Other,

		/** This row corresponds to a news item, say, in an RSS reader
		 * or a Twitter client.
		 */
		News,

		/** This row corresponds to a pending download like in a
		 * BitTorrent client or an HTTP downloader.
		 *
		 * If a row has this type, then it also has to have meaningful
		 * values for ProcessState::Done and ProcessState::Total roles.
		 * These values are expected to be contained in the Progress
		 * column (the third column).
		 */
		DownloadProgress,

		/** This row corresponds to some process like sending a file in
		 * IM, unpacking an archive or checking for new mail.
		 *
		 * If a row has this type, then it also has to have meaningful
		 * values for ProcessState::Done and ProcessState::Total roles.
		 * These values are expected to be contained in the Progress
		 * column (the third column).
		 */
		ProcessProgress
	};

	/** This enum contains roles that are used to query against process
	 * or download completion.
	 *
	 * Rows of types JobHolderRow::DownloadProgress and
	 * JobHolderRow::ProcessProgress are expected to return meaningful
	 * values for roles of this enum.
	 */
	enum ProcessState
	{
		/** This role is expected to contain a qlonglong meaning how much
		 * of the task is completed. For example, how much bytes are
		 * transferred for a download task or how much files are unpacked
		 * in an archive.
		 */
		Done = CustomDataRoles::RoleMAX + 1,

		/** This role is expected to contain a qlonglong meaning the total
		 * size of the task. For example, how big is the file to be
		 * downloaded, or how many files an archive contains.
		 */
		Total
	};
}

/** @brief Interface for plugins holding jobs or persistent notifications.
 *
 * If a plugin can have some long-performing jobs (like a BitTorrent
 * download, or file transfer in an IM client, or mail checking status)
 * or persistent notifications (like unread messages in an IM client,
 * unread news in an RSS feed reader or weather forecast), it may want
 * to implement this interface to display itself in plugins like
 * Summary.
 *
 * Model with jobs and notifications is obtained via GetRepresentation(),
 * and various roles are used to retrieve controls and information pane
 * of the plugin, as well as some metadata like job progress (see
 * CustomDataRoles and ProcessState for example).
 * Returned model should have three columns: one for name, state and
 * status. Controls and additional information pane are only visible
 * when a job handled by the plugin is selected.
 *
 * @sa IDownloader
 * @sa CustomDataRoles
 * @sa ProcessState
 */
class Q_DECL_EXPORT IJobHolder
{
public:
	/** @brief Returns the item representation model.
	 *
	 * The returned model should have three columns, each for name,
	 * state and progress with speed. Inside of LeechCraft it would be
	 * merged with other models from other plugins.
	 *
	 * This model is also used to retrieve controls and additional info
	 * for a given index via the CustomDataRoles::RoleControls and
	 * CustomDataRoles::RoleAdditionalInfo respectively.
	 *
	 * Returned controls widget would be placed above the view with the
	 * jobs, so usually it has some actions controlling the job, but in
	 * fact it can have anything you want. It is only visible when a job
	 * from your plugin is selected. If a job from other plugin is
	 * selected, then other plugin's controls would be placed, and if no
	 * jobs are selected at all then all controls are hidden.
	 *
	 * Widget with the additional information is placed to the right of
	 * the view with the jobs, so usually it has additional information
	 * about the job like transfer log for FTP client, but in fact it
	 * can have anything you want. The same rules regarding its
	 * visibility apply as for controls widget.
	 *
	 * @return Representation model.
	 *
	 * @sa CustomDataRoles
	 */
	virtual QAbstractItemModel* GetRepresentation () const = 0;

	/** @brief Virtual destructor.
	 */
	virtual ~IJobHolder () {}
};

Q_DECLARE_METATYPE (LeechCraft::JobHolderRow);
Q_DECLARE_METATYPE (QAbstractItemModel*);
Q_DECLARE_METATYPE (QList<QModelIndex>);
Q_DECLARE_INTERFACE (IJobHolder, "org.Deviant.LeechCraft.IJobHolder/1.0");

#endif

