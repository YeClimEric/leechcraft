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

#include "otroid.h"
#include <QCoreApplication>
#include <QIcon>
#include <QAction>
#include <QTranslator>

extern "C"
{
#include <libotr/proto.h>
}

#include <interfaces/azoth/iprotocol.h>
#include <interfaces/azoth/iaccount.h>
#include <interfaces/azoth/iclentry.h>
#include <interfaces/azoth/imessage.h>
#include <util/util.h>
#include <util/sys/paths.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "xmlsettingsmanager.h"
#include "otrhandler.h"
#include "fpmanager.h"
#include "privkeymanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace OTRoid
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("azoth_otroid");

		XSD_.reset (new Util::XmlSettingsDialog);
		XSD_->RegisterObject (&XmlSettingsManager::Instance (), "azothotroidsettings.xml");

		CoreProxy_ = proxy;

		OTRL_INIT;
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Azoth.OTRoid";
	}

	void Plugin::Release ()
	{
	}

	QString Plugin::GetName () const
	{
		return "Azoth OTRoid";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Azoth OTRoid adds support for Off-the-Record deniable encryption system.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/plugins/azoth/plugins/otroid/resources/images/otroid.svg");
		return icon;
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Plugins.Azoth.Plugins.IGeneralPlugin";
		return result;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XSD_;
	}

	FPManager* Plugin::GetFPManager () const
	{
		return FPManager_;
	}

	void Plugin::initPlugin (QObject *obj)
	{
		AzothProxy_ = qobject_cast<IProxyObject*> (obj);

		OtrHandler_ = new OtrHandler (CoreProxy_, AzothProxy_);

		FPManager_ = new FPManager (OtrHandler_->GetUserState (), AzothProxy_);
		connect (FPManager_,
				SIGNAL (fingerprintsChanged ()),
				OtrHandler_,
				SLOT (writeFingerprints ()));
		XSD_->SetDataSource ("KnownFPs", FPManager_->GetModel ());

		PKManager_ = new PrivKeyManager (OtrHandler_->GetUserState (), AzothProxy_);
		XSD_->SetDataSource ("PrivKeys", PKManager_->GetModel ());
	}

	void Plugin::hookEntryActionAreasRequested (IHookProxy_ptr proxy,
			QObject *action, QObject*)
	{
		OtrHandler_->HandleEntryActionAreasRequested (proxy, action);
	}

	void Plugin::hookEntryActionsRemoved (IHookProxy_ptr,
			QObject *entry)
	{
		OtrHandler_->HandleEntryActionsRemoved (entry);
	}

	void Plugin::hookEntryActionsRequested (IHookProxy_ptr proxy, QObject *entry)
	{
		OtrHandler_->HandleEntryActionsRequested (proxy, entry);
	}

	void Plugin::hookGotMessage (IHookProxy_ptr proxy, QObject *msgObj)
	{
		OtrHandler_->HandleGotMessage (proxy, msgObj);
	}

	void Plugin::hookMessageCreated (IHookProxy_ptr proxy, QObject*, QObject *msgObj)
	{
		IMessage *msg = qobject_cast<IMessage*> (msgObj);
		if (!msg)
		{
			qWarning () << Q_FUNC_INFO
					<< msgObj
					<< "doesn't implement IMessage";
			return;
		}

		OtrHandler_->HandleMessageCreated (proxy, msg);
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_azoth_otroid, LeechCraft::Azoth::OTRoid::Plugin);
