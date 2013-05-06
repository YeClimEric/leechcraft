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

#include "storage.h"
#include <stdexcept>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <util/util.h>
#include <util/dblock.h>
#include "structures.h"
#include "oral.h"

namespace LeechCraft
{
namespace Poleemery
{
	Storage::Storage (QObject *parent)
	: QObject (parent)
	, DB_ (QSqlDatabase::addDatabase ("QSQLITE", "Poleemery_Connection"))
	{
		const auto& dir = Util::CreateIfNotExists ("poleemeery");
		DB_.setDatabaseName (dir.absoluteFilePath ("database.db"));

		if (!DB_.open ())
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open database:"
					<< DB_.lastError ().text ();
			throw std::runtime_error ("Poleemery database creation failed");
		}

		{
			QSqlQuery query (DB_);
			query.exec ("PRAGMA foreign_keys = ON;");
		}

		InitializeTables ();
		LoadCategories ();
	}

	QList<Account> Storage::GetAccounts () const
	{
		try
		{
			return AccountInfo_.DoSelectAll_ ();
		}
		catch (const oral::QueryException& e)
		{
			qWarning () << Q_FUNC_INFO;
			Util::DBLock::DumpError (e.GetQuery ());
			throw;
		}
	}

	void Storage::AddAccount (Account& acc)
	{
		try
		{
			AccountInfo_.DoInsert_ (acc);
		}
		catch (const oral::QueryException& e)
		{
			qWarning () << Q_FUNC_INFO;
			Util::DBLock::DumpError (e.GetQuery ());
			throw;
		}
	}

	void Storage::UpdateAccount (const Account& acc)
	{
		try
		{
			AccountInfo_.DoUpdate_ (acc);
		}
		catch (const oral::QueryException& e)
		{
			qWarning () << Q_FUNC_INFO;
			Util::DBLock::DumpError (e.GetQuery ());
			throw;
		}
	}

	QList<ExpenseEntry> Storage::GetExpenseEntries (const Account& parent) const
	{
		QList<ExpenseEntry> entries;

		try
		{
			for (const auto& naked : NakedExpenseEntryInfo_.SelectByFKeysActor_ (boost::fusion::make_vector (parent)))
			{
				ExpenseEntry entry { naked };

				const auto& cats = boost::fusion::at_c<1> (CategoryLinkInfo_.SingleFKeySelectors_) (naked);
				for (const auto& cat : cats)
					entry.Categories_ << CatIDCache_ [cat.Category_].Name_;

				entries << entry;
			}
		}
		catch (const oral::QueryException& e)
		{
			qWarning () << Q_FUNC_INFO;
			Util::DBLock::DumpError (e.GetQuery ());
			throw;
		}

		return entries;
	}

	void Storage::AddExpenseEntry (ExpenseEntry& entry)
	{
		Util::DBLock lock (DB_);
		lock.Init ();

		try
		{
			NakedExpenseEntryInfo_.DoInsert_ (entry);

			for (const auto& cat : entry.Categories_)
			{
				if (!CatCache_.contains (cat))
					AddCategory (cat);
				LinkEntry2Cat (entry, CatCache_ [cat]);
			}
		}
		catch (const oral::QueryException& e)
		{
			qWarning () << Q_FUNC_INFO;
			Util::DBLock::DumpError (e.GetQuery ());
			throw;
		}

		lock.Good ();
	}

	Category Storage::AddCategory (const QString& name)
	{
		Category cat { name };
		CategoryInfo_.DoInsert_ (cat);
		CatCache_ [name] = cat;
		CatIDCache_ [cat.ID_] = cat;
		return cat;
	}

	void Storage::LinkEntry2Cat (const ExpenseEntry& entry, const Category& category)
	{
		CategoryLink link (category, entry);
		CategoryLinkInfo_.DoInsert_ (link);
	}

	namespace oral
	{
		template<>
		struct Type2Name<AccType>
		{
			QString operator() () const { return "TEXT"; }
		};

		template<>
		struct ToVariant<AccType>
		{
			QVariant operator() (const AccType& type) const
			{
				switch (type)
				{
				case AccType::BankAccount:
					return "BankAccount";
				case AccType::Cash:
					return "Cash";
				}
			}
		};

		template<>
		struct FromVariant<AccType>
		{
			AccType operator() (const QVariant& var) const
			{
				const auto& str = var.toString ();
				if (str == "BankAccount")
					return AccType::BankAccount;
				else
					return AccType::Cash;
			}
		};
	}

	void Storage::InitializeTables ()
	{
		AccountInfo_ = oral::Adapt<Account> (DB_);
		NakedExpenseEntryInfo_ = oral::Adapt<NakedExpenseEntry> (DB_);
		ReceiptEntryInfo_ = oral::Adapt<ReceiptEntry> (DB_);
		CategoryInfo_ = oral::Adapt<Category> (DB_);
		CategoryLinkInfo_ = oral::Adapt<CategoryLink> (DB_);

		const auto& tables = DB_.tables ();

		QMap<QString, QString> queryCreates;
		queryCreates [Account::ClassName ()] = AccountInfo_.CreateTable_;
		queryCreates [NakedExpenseEntry::ClassName ()] = NakedExpenseEntryInfo_.CreateTable_;
		queryCreates [ReceiptEntry::ClassName ()] = ReceiptEntryInfo_.CreateTable_;
		queryCreates [Category::ClassName ()] = CategoryInfo_.CreateTable_;
		queryCreates [CategoryLink::ClassName ()] = CategoryLinkInfo_.CreateTable_;

		Util::DBLock lock (DB_);
		lock.Init ();

		QSqlQuery query (DB_);

		bool tablesCreated = false;
		for (const auto& key : queryCreates.keys ())
			if (!tables.contains (key))
			{
				tablesCreated = true;
				if (!query.exec (queryCreates [key]))
					{
						Util::DBLock::DumpError (query);
						throw std::runtime_error ("cannot create tables");
					}
			}

		lock.Good ();

		// Otherwise queries created by oral::Adapt() don't work.
		if (tablesCreated)
			InitializeTables ();
	}

	void Storage::LoadCategories ()
	{
		try
		{
			for (const auto& cat : CategoryInfo_.DoSelectAll_ ())
			{
				CatCache_ [cat.Name_] = cat;
				CatIDCache_ [cat.ID_] = cat;
			}
		}
		catch (const oral::QueryException& e)
		{
			qWarning () << Q_FUNC_INFO;
			Util::DBLock::DumpError (e.GetQuery ());
			throw;
		}
	}
}
}
