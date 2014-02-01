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

#include "imagecollectiondialog.h"
#include <map>
#include <functional>
#include <QtDebug>

namespace LeechCraft
{
namespace LHTR
{
	namespace
	{
		class InfosModel : public QAbstractItemModel
		{
			RemoteImageInfos_t& Infos_;
			const QStringList Columns_;

			enum Column
			{
				CImage,
				CSize,
				CAlt
			};
		public:
			InfosModel (RemoteImageInfos_t& infos, QObject *parent);

			QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex()) const;
			QModelIndex parent (const QModelIndex& child) const;
			int rowCount (const QModelIndex& parent = QModelIndex()) const;
			int columnCount (const QModelIndex& parent = QModelIndex()) const;

			QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

			Qt::ItemFlags flags (const QModelIndex& index) const;
			QVariant data (const QModelIndex& index, int role = Qt::DisplayRole) const;
			bool setData (const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
		};

		InfosModel::InfosModel (RemoteImageInfos_t& infos, QObject* parent)
		: QAbstractItemModel { parent }
		, Infos_ (infos)
		, Columns_ { tr ("Image"), tr ("Size"), tr ("Alt") }
		{
		}

		QModelIndex InfosModel::index (int row, int column, const QModelIndex& parent) const
		{
			if (parent.isValid () || !hasIndex (row, column, parent))
				return {};

			return createIndex (row, column);
		}

		QModelIndex InfosModel::parent (const QModelIndex&) const
		{
			return {};
		}

		int InfosModel::rowCount (const QModelIndex& parent) const
		{
			return parent.isValid () ? 0 : Infos_.size ();
		}

		int InfosModel::columnCount (const QModelIndex& parent) const
		{
			return Columns_.size ();
		}

		QVariant InfosModel::headerData (int section, Qt::Orientation orientation, int role) const
		{
			return orientation != Qt::Horizontal || role != Qt::DisplayRole ?
					QVariant {} :
					Columns_.at (section);
		}

		Qt::ItemFlags InfosModel::flags (const QModelIndex& index) const
		{
			auto flags = QAbstractItemModel::flags (index);
			if (index.column () == Column::CAlt)
				flags |= Qt::ItemFlag::ItemIsEditable;
			return flags;
		}

		QVariant InfosModel::data (const QModelIndex& index, int role) const
		{
			const auto& info = Infos_.at (index.row ());

			const std::map<Column, std::map<int, std::function<QVariant ()>>> map
			{
				{
					CImage,
					{
						{
							Qt::DecorationRole,
							[this] () -> QVariant
							{
								return {};
							}
						}
					}
				},
				{
					CSize,
					{
						{
							Qt::DisplayRole,
							[&info]
							{
								return QString::fromUtf8 ("%1×%2")
										.arg (info.FullSize_.width ())
										.arg (info.FullSize_.height ());
							}
						}
					}
				},
				{
					CAlt,
					{
						{
							Qt::DisplayRole,
							[&info] { return info.Title_; }
						},
						{
							Qt::EditRole,
							[&info] { return info.Title_; }
						}
					}
				}
			};

			const auto& roleMap = map.find (static_cast<Column> (index.column ()))->second;
			const auto& roleIt = roleMap.find (role);
			return roleIt == roleMap.end () ? QVariant {} : roleIt->second ();
		}

		bool InfosModel::setData (const QModelIndex& index, const QVariant& value, int)
		{
			if (index.column () != Column::CAlt)
				return false;

			Infos_ [index.row ()].Title_ = value.toString ();
			emit dataChanged (index, index);
			return true;
		}
	}

	ImageCollectionDialog::ImageCollectionDialog (const RemoteImageInfos_t& infos, QWidget *parent)
	: QDialog { parent }
	, Infos_ { infos }
	{
		Ui_.setupUi (this);

		auto model = new InfosModel (Infos_, this);
		Ui_.Images_->setModel (model);
	}

	RemoteImageInfos_t ImageCollectionDialog::GetInfos () const
	{
		return Infos_;
	}

	ImageCollectionDialog::Placement ImageCollectionDialog::GetPlacement () const
	{
		switch (Ui_.ImagePlacement_->currentIndex ())
		{
		case 0:
			return Placement::Next;
		case 1:
			return Placement::Under;
		}

		qWarning () << Q_FUNC_INFO
				<< "unknown placement index"
				<< Ui_.ImagePlacement_->currentIndex ();
		return Placement::Next;
	}

	ImageCollectionDialog::Wrapping ImageCollectionDialog::GetWrapping () const
	{
		switch (Ui_.TextWrapping_->currentIndex ())
		{
		case 0:
			return Wrapping::None;
		case 1:
			return Wrapping::Left;
		case 2:
			return Wrapping::Right;
		}

		qWarning () << Q_FUNC_INFO
				<< "unknown text wrapping"
				<< Ui_.TextWrapping_->currentIndex ();
		return Wrapping::None;
	}
}
}
