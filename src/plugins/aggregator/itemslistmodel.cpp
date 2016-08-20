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

#include "itemslistmodel.h"
#include <algorithm>
#include <QApplication>
#include <QPalette>
#include <QTextDocument>
#include <QtDebug>
#include <interfaces/core/iiconthememanager.h>
#include "core.h"
#include "xmlsettingsmanager.h"
#include "storagebackendmanager.h"

namespace LeechCraft
{
namespace Aggregator
{
	ItemsListModel::ItemsListModel (QObject *parent)
	: QAbstractItemModel (parent)
	, StarredIcon_ (Core::Instance ().GetProxy ()->GetIconThemeManager ()->GetIcon ("mail-mark-important"))
	, UnreadIcon_ (Core::Instance ().GetProxy ()->GetIconThemeManager ()->GetIcon ("mail-mark-unread"))
	, ReadIcon_ (Core::Instance ().GetProxy ()->GetIconThemeManager ()->GetIcon ("mail-mark-read"))
	{
		ItemHeaders_ << tr ("Name") << tr ("Date");

		connect (&Core::Instance (),
				SIGNAL (channelRemoved (IDType_t)),
				this,
				SLOT (handleChannelRemoved (IDType_t)));

		connect (&StorageBackendManager::Instance (),
				SIGNAL (itemsRemoved (QSet<IDType_t>)),
				this,
				SLOT (handleItemsRemoved (QSet<IDType_t>)));
		connect (&StorageBackendManager::Instance (),
				SIGNAL (itemDataUpdated (Item_ptr, Channel_ptr)),
				this,
				SLOT (handleItemDataUpdated (Item_ptr, Channel_ptr)));
	}

	int ItemsListModel::GetSelectedRow () const
	{
		return CurrentRow_;
	}

	const IDType_t& ItemsListModel::GetCurrentChannel () const
	{
		return CurrentChannel_;
	}

	void ItemsListModel::SetCurrentChannel (const IDType_t& channel)
	{
		beginResetModel ();
		CurrentChannel_ = channel;
		endResetModel ();
	}

	void ItemsListModel::Selected (const QModelIndex& index)
	{
		CurrentRow_ = index.row ();
		if (!index.isValid ())
			return;

		auto item = CurrentItems_ [CurrentRow_];
		if (!item.Unread_)
			return;

		item.Unread_ = false;
		Core::Instance ().GetStorageBackend ()->UpdateItem (item);
	}

	const ItemShort& ItemsListModel::GetItem (const QModelIndex& index) const
	{
		return CurrentItems_ [index.row ()];
	}

	const items_shorts_t& ItemsListModel::GetAllItems () const
	{
		return CurrentItems_;
	}

	bool ItemsListModel::IsItemRead (int item) const
	{
		return !CurrentItems_ [item].Unread_;
	}

	QStringList ItemsListModel::GetCategories (int item) const
	{
		return CurrentItems_ [item].Categories_;
	}

	void ItemsListModel::Reset (const IDType_t& channel)
	{
		beginResetModel ();

		CurrentChannel_ = channel;
		CurrentRow_ = -1;
		CurrentItems_.clear ();
		if (channel != static_cast<IDType_t> (-1))
			Core::Instance ().GetStorageBackend ()->GetItems (CurrentItems_, channel);

		endResetModel ();
	}

	void ItemsListModel::Reset (const QList<IDType_t>& items)
	{
		beginResetModel ();

		CurrentChannel_ = -1;
		CurrentRow_ = -1;
		CurrentItems_.clear ();

		StorageBackend *sb = Core::Instance ().GetStorageBackend ();
		for (const IDType_t& itemId : items)
			CurrentItems_.push_back (sb->GetItem (itemId)->ToShort ());

		endResetModel ();
	}

	void ItemsListModel::RemoveItems (const QSet<IDType_t>& ids)
	{
		if (ids.isEmpty ())
			return;

		const bool shouldReset = ids.size () > 10;

		if (shouldReset)
			beginResetModel ();

		int remainingCount = ids.size ();

		for (auto i = CurrentItems_.begin ();
				i != CurrentItems_.end () && remainingCount; )
		{
			if (!ids.contains (i->ItemID_))
			{
				++i;
				continue;
			}

			if (!shouldReset)
			{
				const size_t dist = std::distance (CurrentItems_.begin (), i);
				beginRemoveRows (QModelIndex (), dist, dist);
			}

			i = CurrentItems_.erase (i);
			--remainingCount;

			if (!shouldReset)
			{
				endRemoveRows ();
				qApp->processEvents (QEventLoop::ExcludeUserInputEvents);
			}
		}

		if (shouldReset)
			endResetModel ();
	}

	void ItemsListModel::ItemDataUpdated (Item_ptr item)
	{
		const auto& is = item->ToShort ();

		const auto pos = std::find_if (CurrentItems_.begin (), CurrentItems_.end (),
				[&item] (const ItemShort& itemShort)
				{
					return item->ItemID_ == itemShort.ItemID_ ||
							(item->Title_ == itemShort.Title_ && item->Link_ == itemShort.URL_);
				});

		// Item is new
		if (pos == CurrentItems_.end ())
		{
			auto insertPos = std::find_if (CurrentItems_.begin (), CurrentItems_.end (),
						[item] (const ItemShort& is) { return item->PubDate_ > is.PubDate_; });

			int shift = std::distance (CurrentItems_.begin (), insertPos);

			beginInsertRows (QModelIndex (), shift, shift);
			CurrentItems_.insert (insertPos, is);
			endInsertRows ();
		}
		// Item exists already
		else
		{
			*pos = is;

			int distance = std::distance (CurrentItems_.begin (), pos);
			emit dataChanged (index (distance, 0), index (distance, 1));
		}
	}

	int ItemsListModel::columnCount (const QModelIndex&) const
	{
		return ItemHeaders_.size ();
	}

	namespace
	{
		void RemoveTag (const QString& name, QString& str)
		{
			int startPos = 0;
			while ((startPos = str.indexOf ("<" + name, startPos, Qt::CaseInsensitive)) >= 0)
			{
				const int end = str.indexOf ('>', startPos);
				if (end < 0)
					return;

				str.remove (startPos, end - startPos + 1);
			}
		}

		void RemovePair (const QString& name, QString& str)
		{
			RemoveTag (name, str);
			RemoveTag ('/' + name, str);
		}
	}

	QVariant ItemsListModel::data (const QModelIndex& index, int role) const
	{
		if (!index.isValid () || index.row () >= rowCount ())
			return QVariant ();

		if (role == Qt::DisplayRole)
		{
			switch (index.column ())
			{
				case 0:
					{
						auto title = CurrentItems_ [index.row ()].Title_;
						auto pos = 0;
						while ((pos = title.indexOf ('<', pos)) != -1)
						{
							auto end = title.indexOf ('>', pos);
							if (end > 0)
								title.remove (pos, end - pos + 1);
							else
								break;
						}

						title.replace ("&laquo;", QString::fromUtf8 ("«"));
						title.replace ("&raquo;", QString::fromUtf8 ("»"));
						title.replace ("&quot;", "\"");
						title.replace ("&ndash;", "-");
						title.replace ("&mdash;", QString::fromUtf8 ("—"));

						return title;
					}
				case 1:
					return CurrentItems_ [index.row ()].PubDate_;
				default:
					return QVariant ();
			}
		}
		//Color mark an items as read/unread
		else if (role == Qt::ForegroundRole)
		{
			bool palette = XmlSettingsManager::Instance ()->
					property ("UsePaletteColors").toBool ();
			if (CurrentItems_ [index.row ()].Unread_)
			{
				if (XmlSettingsManager::Instance ()->
						property ("UnreadCustomColor").toBool ())
					return XmlSettingsManager::Instance ()->
							property ("UnreadItemsColor").value<QColor> ();
				else
					return palette ?
						QApplication::palette ().link ().color () :
						QVariant ();
			}
			else
				return palette ?
					QApplication::palette ().linkVisited ().color () :
					QVariant ();
		}
		else if (role == Qt::FontRole &&
				CurrentItems_ [index.row ()].Unread_)
			return XmlSettingsManager::Instance ()->
				property ("UnreadItemsFont");
		else if (role == Qt::ToolTipRole &&
				XmlSettingsManager::Instance ()->property ("ShowItemsTooltips").toBool ())
		{
			IDType_t id = CurrentItems_ [index.row ()].ItemID_;
			Item_ptr item = Core::Instance ()
					.GetStorageBackend ()->GetItem (id);
			QString result = QString ("<qt><strong>%1</strong><br />").arg (item->Title_);
			if (item->Author_.size ())
			{
				result += tr ("<b>Author</b>: %1").arg (item->Author_);
				result += "<br />";
			}
			if (item->Categories_.size ())
			{
				result += tr ("<b>Categories</b>: %1").arg (item->Categories_.join ("; "));
				result += "<br />";
			}
			if (item->NumComments_ > 0)
			{
				result += tr ("%n comment(s)", "", item->NumComments_);
				result += "<br />";
			}
			if (item->Enclosures_.size () > 0)
			{
				result += tr ("%n enclosure(s)", "", item->Enclosures_.size ());
				result += "<br />";
			}
			if (item->MRSSEntries_.size () > 0)
			{
				result += tr ("%n MediaRSS entry(s)", "", item->MRSSEntries_.size ());
				result += "<br />";
			}
			if (item->CommentsLink_.size ())
			{
				result += tr ("RSS with comments is available");
				result += "<br />";
			}
			result += "<br />";

			const int maxDescriptionSize = 1000;
			auto descr = item->Description_;
			RemoveTag ("img", descr);
			RemovePair ("font", descr);
			RemovePair ("span", descr);
			RemovePair ("p", descr);
			RemovePair ("div", descr);
			for (auto i : { 1, 2, 3, 4, 5, 6 })
				RemovePair ("h" + QString::number (i), descr);
			result += descr.left (maxDescriptionSize);
			if (descr.size () > maxDescriptionSize)
				result += "...";

			return result;
		}
		else if (role == Qt::BackgroundRole)
		{
			const QPalette& p = QApplication::palette ();
			QLinearGradient grad (0, 0, 0, 10);
			grad.setColorAt (0, p.color (QPalette::AlternateBase));
			grad.setColorAt (1, p.color (QPalette::Base));
			return QBrush (grad);
		}
		else if (role == Qt::DecorationRole)
		{
			if (index.column ())
				return QVariant ();

			const auto& item = CurrentItems_ [index.row ()];
			if (Core::Instance ().GetStorageBackend ()->
					GetItemTags (item.ItemID_).contains ("_important"))
				return StarredIcon_;

			return item.Unread_ ? UnreadIcon_ : ReadIcon_;
		}
		else if (role == ItemRole::IsRead)
			return !CurrentItems_ [index.row ()].Unread_;
		else if (role == ItemRole::ItemId)
			return CurrentItems_ [index.row ()].ItemID_;
		else if (role == ItemRole::ItemShortDescr)
			return QVariant::fromValue (CurrentItems_ [index.row ()]);
		else
			return QVariant ();
	}

	Qt::ItemFlags ItemsListModel::flags (const QModelIndex&) const
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	QVariant ItemsListModel::headerData (int column, Qt::Orientation orient, int role) const
	{
		if (orient == Qt::Horizontal && role == Qt::DisplayRole)
			return ItemHeaders_.at (column);
		else
			return QVariant ();
	}

	QModelIndex ItemsListModel::index (int row, int column, const QModelIndex& parent) const
	{
		if (!hasIndex (row, column, parent))
			return QModelIndex ();

		return createIndex (row, column);
	}

	QModelIndex ItemsListModel::parent (const QModelIndex&) const
	{
		return QModelIndex ();
	}

	int ItemsListModel::rowCount (const QModelIndex& parent) const
	{
		return parent.isValid () ? 0 : CurrentItems_.size ();
	}

	void ItemsListModel::reset (const IDType_t& type)
	{
		Reset (type);
	}

	void ItemsListModel::selected (const QModelIndex& index)
	{
		Selected (index);
	}

	void ItemsListModel::handleChannelRemoved (IDType_t id)
	{
		if (id != CurrentChannel_)
			return;
		Reset (-1);
	}

	void ItemsListModel::handleItemsRemoved (const QSet<IDType_t>& items)
	{
		RemoveItems (items);
	}

	void ItemsListModel::handleItemDataUpdated (const Item_ptr& item, const Channel_ptr& channel)
	{
		if (channel->ChannelID_ != CurrentChannel_)
			return;

		ItemDataUpdated (item);
	}
}
}
