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

#include <QModelIndex>
#include <QSpinBox>
#include <QLineEdit>
#include <QApplication>
#include <QTreeView>
#include <QtDebug>
#include <util/util.h>
#include "filesviewdelegate.h"
#include "torrentfilesmodel.h"

namespace LeechCraft
{
namespace BitTorrent
{
	FilesViewDelegate::FilesViewDelegate (QTreeView *parent)
	: QStyledItemDelegate (parent)
	, View_ (parent)
	{
	}

	FilesViewDelegate::~FilesViewDelegate ()
	{
	}

	QWidget* FilesViewDelegate::createEditor (QWidget *parent,
			const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		if (index.column () == TorrentFilesModel::ColumnPriority)
		{
			const auto box = new QSpinBox (parent);
			box->setRange (0, 7);
			return box;
		}
		else if (index.column () == TorrentFilesModel::ColumnPath)
			return new QLineEdit (parent);
		else
			return QStyledItemDelegate::createEditor (parent, option, index);
	}

	void FilesViewDelegate::paint (QPainter *painter,
			const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		if (index.column () != TorrentFilesModel::ColumnProgress)
		{
			QStyledItemDelegate::paint (painter, option, index);
			return;
		}
		else
		{
			QStyleOptionProgressBar progressBarOption;
			progressBarOption.state = QStyle::State_Enabled;
			progressBarOption.direction = QApplication::layoutDirection ();
			progressBarOption.rect = option.rect;
			progressBarOption.fontMetrics = QApplication::fontMetrics ();
			progressBarOption.minimum = 0;
			progressBarOption.maximum = 100;
			progressBarOption.textAlignment = Qt::AlignCenter;
			progressBarOption.textVisible = true;

			double progress = index.data (TorrentFilesModel::RoleProgress).toDouble ();
			qlonglong size = index.data (TorrentFilesModel::RoleSize).toLongLong ();
			qlonglong done = progress * size;
			progressBarOption.progress = progress < 0 ?
				0 :
				static_cast<int> (progress * 100);
			progressBarOption.text = QString (tr ("%1% (%2 of %3)")
					.arg (static_cast<int> (progress * 100))
					.arg (Util::MakePrettySize (done))
					.arg (Util::MakePrettySize (size)));

			QApplication::style ()->drawControl (QStyle::CE_ProgressBar,
					&progressBarOption, painter);
		}
	}

	void FilesViewDelegate::setEditorData (QWidget *editor, const QModelIndex& index) const
	{
		if (index.column () == TorrentFilesModel::ColumnPriority)
			qobject_cast<QSpinBox*> (editor)->
				setValue (index.data (TorrentFilesModel::RolePriority).toInt ());
		else if (index.column () == TorrentFilesModel::ColumnPath)
		{
			const auto& data = index.data (TorrentFilesModel::RoleFullPath);
			qobject_cast<QLineEdit*> (editor)->setText (data.toString ());
		}
		else
			QStyledItemDelegate::setEditorData (editor, index);
	}

	void FilesViewDelegate::setModelData (QWidget *editor, QAbstractItemModel *model,
			const QModelIndex& index) const
	{
		if (index.column () == TorrentFilesModel::ColumnPriority)
		{
			int value = qobject_cast<QSpinBox*> (editor)->value ();
			const auto& sindexes = View_->selectionModel ()->selectedRows ();
			for (const auto& selected : sindexes)
				model->setData (index.sibling (selected.row (), index.column ()), value);
		}
		else if (index.column () == TorrentFilesModel::ColumnPath)
		{
			const auto& oldData = index.data (TorrentFilesModel::RoleFullPath);
			const auto& newText = qobject_cast<QLineEdit*> (editor)->text ();
			if (oldData.toString () == newText)
				return;

			model->setData (index, newText);
		}
		else
			QStyledItemDelegate::setModelData (editor, model, index);
	}

	void FilesViewDelegate::updateEditorGeometry (QWidget *editor,
			const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		if (index.column () != TorrentFilesModel::ColumnPath)
		{
			QStyledItemDelegate::updateEditorGeometry (editor, option, index);
			return;
		}

		auto rect = option.rect;
		rect.setX (0);
		rect.setWidth (editor->parentWidget ()->width ());
		editor->setGeometry (rect);
	}
}
}
