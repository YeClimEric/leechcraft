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

#include "todoitem.h"
#include <QUuid>
#include <QDataStream>
#include <QtDebug>

namespace LeechCraft
{
namespace Otlozhu
{
	TodoItem::TodoItem ()
	: ID_ (QUuid::createUuid ().toString ())
	, Created_ (QDateTime::currentDateTime ())
	, Percentage_ (0)
	{
	}

	TodoItem::TodoItem (const QString& id)
	: ID_ (id)
	, Created_ (QDateTime::currentDateTime ())
	, Percentage_ (0)
	{
	}

	TodoItem_ptr TodoItem::Clone () const
	{
		TodoItem_ptr clone (new TodoItem (GetID ()));
		clone->Title_ = Title_;
		clone->Comment_ = Comment_;
		clone->TagIDs_ = TagIDs_;
		clone->Created_ = Created_;
		clone->Due_ = Due_;
		clone->Percentage_ = Percentage_;
		clone->Deps_ = Deps_;
		return clone;
	}

	void TodoItem::CopyFrom (const TodoItem_ptr item)
	{
		Title_ = item->Title_;
		Comment_ = item->Comment_;
		TagIDs_ = item->TagIDs_;
		Created_ = item->Created_;
		Due_ = item->Due_;
		Percentage_ = item->Percentage_;
		Deps_ = item->Deps_;
	}

	namespace
	{
		class Checker
		{
			const TodoItem *This_;
			const TodoItem *That_;

			QVariantMap Result_;
		public:
			Checker (const TodoItem *our, const TodoItem *that)
			: This_ (our)
			, That_ (that)
			{
			}

			template<typename T>
			Checker& operator() (const QString& name, T TodoItem::* g)
			{
				if (This_->*g != That_->*g)
					Result_ [name] = This_->*g;
				return *this;
			}

			operator QVariantMap () const
			{
				return Result_;
			}
		};
	}

	QVariantMap TodoItem::DiffWith (const TodoItem_ptr item) const
	{
		return Checker (this, item.get ())
				("Title", &TodoItem::Title_)
				("Comment", &TodoItem::Comment_)
				("Tags", &TodoItem::TagIDs_)
				("Deps", &TodoItem::Deps_)
				("Created", &TodoItem::Created_)
				("Due", &TodoItem::Due_)
				("Percentage", &TodoItem::Percentage_);
	}

	namespace
	{
		class Applier
		{
			TodoItem *Item_;

			const QVariantMap& Map_;
		public:
			Applier (TodoItem *item, const QVariantMap& map)
			: Item_ (item)
			, Map_ (map)
			{
			}

			template<typename T>
			Applier& operator() (const QString& name, T TodoItem::* g)
			{
				if (Map_.contains (name))
					Item_->*g = Map_ [name].value<T> ();
				return *this;
			}
		};
	}

	void TodoItem::ApplyDiff (const QVariantMap& map)
	{
		Applier (this, map)
				("Title", &TodoItem::Title_)
				("Comment", &TodoItem::Comment_)
				("Tags", &TodoItem::TagIDs_)
				("Deps", &TodoItem::Deps_)
				("Created", &TodoItem::Created_)
				("Due", &TodoItem::Due_)
				("Percentage", &TodoItem::Percentage_);
	}

	TodoItem_ptr TodoItem::Deserialize (const QByteArray& data)
	{
		QDataStream str (data);
		quint8 version = 0;
		str >> version;
		if (version != 1)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown version"
					<< version;
			return TodoItem_ptr ();
		}

		TodoItem_ptr item (new TodoItem);
		str >> item->ID_
			>> item->Title_
			>> item->Comment_
			>> item->TagIDs_
			>> item->Created_
			>> item->Due_
			>> item->Percentage_
			>> item->Deps_;
		return item;
	}

	QByteArray TodoItem::Serialize () const
	{
		QByteArray res;
		QDataStream out (&res, QIODevice::WriteOnly);

		out << static_cast<quint8> (1)
			<< ID_
			<< Title_
			<< Comment_
			<< TagIDs_
			<< Created_
			<< Due_
			<< Percentage_
			<< Deps_;

		return res;
	}

	QString TodoItem::GetID () const
	{
		return ID_;
	}

	QString TodoItem::GetTitle () const
	{
		return Title_;
	}

	void TodoItem::SetTitle (const QString& title)
	{
		Title_ = title;
	}

	QString TodoItem::GetComment () const
	{
		return Comment_;
	}

	void TodoItem::SetComment (const QString& comment)
	{
		Comment_ = comment;
	}

	QStringList TodoItem::GetTagIDs () const
	{
		return TagIDs_;
	}

	void TodoItem::SetTagIDs (const QStringList& tagIds)
	{
		TagIDs_ = tagIds;
	}

	QDateTime TodoItem::GetCreatedDate () const
	{
		return Created_;
	}

	void TodoItem::SetCreatedDate (const QDateTime& created)
	{
		Created_ = created;
	}

	QDateTime TodoItem::GetDueDate () const
	{
		return Due_;
	}

	void TodoItem::SetDueDate (const QDateTime& due)
	{
		Due_ = due;
	}

	int TodoItem::GetPercentage () const
	{
		return Percentage_;
	}

	void TodoItem::SetPercentage (int p)
	{
		Percentage_ = p;
	}

	QStringList TodoItem::GetDeps () const
	{
		return Deps_;
	}

	void TodoItem::SetDeps (const QStringList& deps)
	{
		Deps_ = deps;
	}

	void TodoItem::AddDep (const QString& dep)
	{
		Deps_ << dep;
	}
}
}
