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

#include "collectionsortermodel.h"
#include <functional>
#include "localcollection.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace LMP
{
	namespace
	{
		enum CompareFlag
		{
			None = 0x0,
			WithoutThe = 0x1
		};
		Q_DECLARE_FLAGS (CompareFlags, CompareFlag);

		template<typename T>
		bool VarCompare (const QVariant& left, const QVariant& right, CompareFlags)
		{
			return left.value<T> () < right.value<T> ();
		}

		template<>
		bool VarCompare<QString> (const QVariant& left, const QVariant& right, CompareFlags)
		{
			return QString::localeAwareCompare (left.toString (), right.toString ()) < 0;
		}

		bool NameCompare (const QVariant& left, const QVariant& right, CompareFlags flags)
		{
			auto leftStr = left.toString ();
			auto rightStr = right.toString ();

			if (flags & WithoutThe)
			{
				auto chopStr = [] (QString& str)
				{
					if (str.startsWith ("the ", Qt::CaseInsensitive))
						str = str.mid (4);
				};

				chopStr (leftStr);
				chopStr (rightStr);
			}

			return QString::localeAwareCompare (leftStr, rightStr) < 0;
		}

		struct Comparators
		{
			typedef std::function<bool (const QVariant&, const QVariant&, CompareFlags)> Comparator_t;
			QHash<LocalCollection::Role, Comparator_t> Role2Cmp_;

			Comparators ()
			{
				Role2Cmp_ [LocalCollection::Role::ArtistName] = NameCompare;
				Role2Cmp_ [LocalCollection::Role::AlbumName] = VarCompare<QString>;
				Role2Cmp_ [LocalCollection::Role::AlbumYear] = VarCompare<int>;
				Role2Cmp_ [LocalCollection::Role::TrackNumber] = VarCompare<int>;
				Role2Cmp_ [LocalCollection::Role::TrackTitle] = VarCompare<QString>;
				Role2Cmp_ [LocalCollection::Role::TrackPath] = VarCompare<QString>;
			}
		};

		bool RoleCompare (const QModelIndex& left, const QModelIndex& right,
				QList<LocalCollection::Role> roles, CompareFlags flags)
		{
			static Comparators comparators;
			while (!roles.isEmpty ())
			{
				auto role = roles.takeFirst ();
				const auto& lData = left.data (role);
				const auto& rData = right.data (role);
				if (lData != rData)
					return comparators.Role2Cmp_ [role] (lData, rData, flags);
			}
			return false;
		}
	}
	CollectionSorterModel::CollectionSorterModel (QObject *parent)
	: QSortFilterProxyModel (parent)
	, UseThe_ (true)
	{
		XmlSettingsManager::Instance ().RegisterObject ("SortWithThe",
				this,
				"handleUseTheChanged");

		handleUseTheChanged ();
	}

	bool CollectionSorterModel::lessThan (const QModelIndex& left, const QModelIndex& right) const
	{
		const auto type = left.data (LocalCollection::Role::Node).toInt ();
		QList<LocalCollection::Role> roles;
		switch (type)
		{
		case LocalCollection::NodeType::Artist:
			roles << LocalCollection::Role::ArtistName;
			break;
		case LocalCollection::NodeType::Album:
			roles << LocalCollection::Role::AlbumYear
					<< LocalCollection::Role::AlbumName;
			break;
		case LocalCollection::NodeType::Track:
			roles << LocalCollection::Role::TrackNumber
					<< LocalCollection::Role::TrackTitle
					<< LocalCollection::Role::TrackPath;
			break;
		default:
			return QSortFilterProxyModel::lessThan (left, right);
		}

		CompareFlags flags = CompareFlag::None;
		if (!UseThe_)
			flags |= CompareFlag::WithoutThe;

		return RoleCompare (left, right, roles, flags);
	}

	void CollectionSorterModel::handleUseTheChanged ()
	{
		UseThe_ = XmlSettingsManager::Instance ()
				.property ("SortWithThe").toBool ();
		invalidate ();
	}
}
}
