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

#include "acceptlangwidget.h"
#include <QStandardItemModel>
#include <QMessageBox>
#include <util/sll/prelude.h>
#include <util/util.h>
#include "xmlsettingsmanager.h"

Q_DECLARE_METATYPE (QList<QLocale>)

namespace LeechCraft
{
namespace Intermutko
{
	AcceptLangWidget::AcceptLangWidget (QWidget *parent)
	: QWidget { parent }
	, Model_ { new QStandardItemModel { this } }
	{
		Ui_.setupUi (this);
		Ui_.LangsTree_->setModel (Model_);

		Model_->setHorizontalHeaderLabels ({ tr ("Language"), tr ("Country"), tr ("Code") });

		LoadSettings ();

		reject ();

		for (int i = 0; i < QLocale::LastCountry; ++i)
			Ui_.Country_->addItem (QLocale::countryToString (static_cast<QLocale::Country> (i)), i);
		Ui_.Country_->model ()->sort (0);

		for (int i = 0; i < QLocale::LastLanguage; ++i)
		{
			if (i == QLocale::C)
				continue;

			Ui_.Language_->addItem (QLocale::languageToString (static_cast<QLocale::Language> (i)), i);
		}
		Ui_.Language_->model ()->sort (0);
		on_Language__currentIndexChanged (Ui_.Language_->currentIndex ());
	}

	const QString& AcceptLangWidget::GetLocaleString () const
	{
		return LocaleStr_;
	}

	namespace
	{
		QString GetCode (const LocaleEntry& entry)
		{
			const auto& name = Util::GetInternetLocaleName ({ entry.Language_, entry.Country_ });
			return entry.Country_ != QLocale::AnyCountry ?
					name :
					name.left (2);
		}

		QString GetCountryName (QLocale::Country c)
		{
			return c == QLocale::AnyCountry ?
					AcceptLangWidget::tr ("Any country") :
					QLocale::countryToString (c);
		}
	}

	void AcceptLangWidget::AddLocale (const LocaleEntry& entry)
	{
		if (GetModelLocales ().contains (entry))
			return;

		QList<QStandardItem*> items
		{
			new QStandardItem { QLocale::languageToString (entry.Language_) },
			new QStandardItem { GetCountryName (entry.Country_) },
			new QStandardItem { GetCode (entry) }
		};
		Model_->appendRow (items);
		items.first ()->setData (QVariant::fromValue (entry), Roles::LocaleObj);
	}

	QList<LocaleEntry> AcceptLangWidget::GetModelLocales () const
	{
		QList<LocaleEntry> result;
		for (int i = 0; i < Model_->rowCount (); ++i)
			result << Model_->item (i)->data (Roles::LocaleObj).value<LocaleEntry> ();
		return result;
	}

	void AcceptLangWidget::WriteSettings ()
	{
		XmlSettingsManager::Instance ().setProperty ("LocaleEntries", QVariant::fromValue (Locales_));
	}

	void AcceptLangWidget::LoadSettings ()
	{
		Locales_ = XmlSettingsManager::Instance ()
				.property ("LocaleEntries").value<QList<LocaleEntry>> ();

		RebuildLocaleStr ();
	}

	void AcceptLangWidget::RebuildLocaleStr ()
	{
		LocaleStr_ = QStringList { Util::Map (Locales_, &GetCode) }.join (", ");
	}

	void AcceptLangWidget::accept ()
	{
		Locales_ = GetModelLocales ();
		WriteSettings ();
		RebuildLocaleStr ();
	}

	void AcceptLangWidget::reject ()
	{
		if (const auto rc = Model_->rowCount ())
			Model_->removeRows (0, rc);

		for (const auto& locale : Locales_)
			AddLocale (locale);
	}

	namespace
	{
		template<typename T>
		T GetValue (const QComboBox *box)
		{
			const int idx = box->currentIndex ();
			if (idx <= 0)
				return T ();

			const int val = box->itemData (idx).toInt ();
			return static_cast<T> (val);
		}
	}

	void AcceptLangWidget::on_Add__released ()
	{
		const auto country = GetValue<QLocale::Country> (Ui_.Country_);
		const auto lang = GetValue<QLocale::Language> (Ui_.Language_);
		AddLocale ({ lang, country });

		if (!GetModelLocales ().contains ({ lang, QLocale::AnyCountry }) &&
				QMessageBox::question (this,
						"LeechCraft",
						tr ("Do you want to add an accepted language without "
							"any country specified as a fallback?"),
						QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			AddLocale ({ lang, QLocale::AnyCountry });
	}

	void AcceptLangWidget::on_Remove__released ()
	{
		const auto& idx = Ui_.LangsTree_->currentIndex ();
		if (!idx.isValid ())
			return;

		Model_->removeRow (idx.row ());
	}

	void AcceptLangWidget::on_MoveUp__released ()
	{
		const auto item = Model_->itemFromIndex (Ui_.LangsTree_->currentIndex ());
		if (!item || !item->row ())
			return;

		const int row = item->row ();
		Model_->insertRow (row - 1, Model_->takeRow (row));
	}

	void AcceptLangWidget::on_MoveDown__released ()
	{
		const auto item = Model_->itemFromIndex (Ui_.LangsTree_->currentIndex ());
		if (!item || item->row () == Model_->rowCount () - 1)
			return;

		const int row = item->row ();
		auto items = Model_->takeRow (row);
		Model_->insertRow (row + 1, items);
	}

	void AcceptLangWidget::on_Language__currentIndexChanged (int)
	{
		Ui_.Country_->clear ();

		const auto lang = GetValue<QLocale::Language> (Ui_.Language_);
		auto countries = QLocale::countriesForLanguage (lang);
		if (!countries.contains (QLocale::AnyCountry))
			countries << QLocale::AnyCountry;

		for (auto c : countries)
			Ui_.Country_->addItem (GetCountryName (c), c);

		Ui_.Country_->model ()->sort (0);
	}
}
}
