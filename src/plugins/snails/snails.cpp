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

#include "snails.h"
#include <QIcon>
#include <util/util.h>
#include <util/xsd/wkfontswidget.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include <interfaces/core/iiconthememanager.h>
#include "mailtab.h"
#include "xmlsettingsmanager.h"
#include "accountslistwidget.h"
#include "core.h"
#include "progressmanager.h"
#include "composemessagetab.h"
#include "composemessagetabfactory.h"

namespace LeechCraft
{
namespace Snails
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("snails");

		Proxy_ = proxy;

		MailTabClass_ =
		{
			"mail",
			tr ("Mail"),
			tr ("Mail tab."),
			GetIcon (),
			65,
			TFOpenableByRequest
		};
		ComposeTabClass_ =
		{
			"compose",
			tr ("Compose mail"),
			tr ("Allows one to compose outgoing mail messages."),
			proxy->GetIconThemeManager ()->GetIcon ("mail-message-new"),
			60,
			TFOpenableByRequest
		};

		ComposeMessageTab::SetParentPlugin (this);
		ComposeMessageTab::SetTabClassInfo (ComposeTabClass_);

		Core::Instance ().SetProxy (proxy);

		ComposeTabFactory_ = new ComposeMessageTabFactory;

		connect (ComposeTabFactory_,
				SIGNAL (gotTab (QString, QWidget*)),
				this,
				SLOT (handleNewTab (QString, QWidget*)));

		XSD_.reset (new Util::XmlSettingsDialog);
		XSD_->RegisterObject (&XmlSettingsManager::Instance (), "snailssettings.xml");

		XSD_->SetCustomWidget ("AccountsWidget", new AccountsListWidget);

		WkFontsWidget_ = new Util::WkFontsWidget { &XmlSettingsManager::Instance () };
		XSD_->SetCustomWidget ("FontsSelector", WkFontsWidget_);
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Snails";
	}

	void Plugin::Release ()
	{
		Core::Instance ().Release ();
	}

	QString Plugin::GetName () const
	{
		return "Snails";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("LeechCraft mail client.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/resources/images/snails.svg");
		return icon;
	}

	TabClasses_t Plugin::GetTabClasses () const
	{
		return
		{
			MailTabClass_,
			ComposeTabClass_
		};
	}

	void Plugin::TabOpenRequested (const QByteArray& tabClass)
	{
		if (tabClass == "mail")
		{
			const auto mt = new MailTab { Proxy_, ComposeTabFactory_, MailTabClass_, this };
			handleNewTab (MailTabClass_.VisibleName_, mt);
			WkFontsWidget_->RegisterSettable (mt);
		}
		else if (tabClass == "compose")
		{
			const auto ct = ComposeTabFactory_->MakeTab ();
			handleNewTab (ct->GetTabClassInfo ().VisibleName_, ct);
		}
		else
			qWarning () << Q_FUNC_INFO
					<< "unknown tab class"
					<< tabClass;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XSD_;
	}

	QAbstractItemModel* Plugin::GetRepresentation () const
	{
		return Core::Instance ().GetProgressManager ()->GetRepresentation ();
	}

	void Plugin::handleNewTab (const QString& name, QWidget *mt)
	{
		connect (mt,
				SIGNAL (removeTab (QWidget*)),
				this,
				SIGNAL (removeTab (QWidget*)));

		emit addNewTab (name, mt);
		emit raiseTab (mt);
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_snails, LeechCraft::Snails::Plugin);

