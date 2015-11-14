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

#include "addtoblockedrunner.h"
#include <util/sll/functional.h>
#include <util/sll/prelude.h>
#include <util/sll/util.h>
#include "clientconnection.h"
#include "privacylistsmanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	AddToBlockedRunner::AddToBlockedRunner (const QStringList& ids,
			const ClientConnection_ptr& conn, QObject *parent)
	: QObject { parent }
	, Ids_ { ids }
	, Conn_ { conn }
	{
		Conn_->GetPrivacyListsManager ()->QueryLists ({
					[this] (const QXmppIq&) { deleteLater (); },
					Util::BindMemFn (&AddToBlockedRunner::HandleGotLists, this)
				});
	}

	void AddToBlockedRunner::HandleGotLists (const QStringList&,
			const QString& active, const QString& def)
	{
		bool activate = false;
		QString listName;
		if (!active.isEmpty ())
			listName = active;
		else if (!def.isEmpty ())
			listName = def;
		else
		{
			listName = "default";
			activate = true;
		}

		FetchList (listName, activate);
	}

	void AddToBlockedRunner::FetchList (const QString& listName, bool activate)
	{
		Conn_->GetPrivacyListsManager ()->QueryList (listName,
				{
					[=] (const QXmppIq&) { AddToList (listName, {}, activate); },
					[=] (const PrivacyList& list) { AddToList (listName, list, activate); }
				});
	}

	void AddToBlockedRunner::AddToList (const QString& name, PrivacyList list, bool activate)
	{
		deleteLater ();

		if (list.GetName ().isEmpty ())
			list.SetName (name);

		auto items = list.GetItems ();

		const auto& presentIds = QSet<QString>::fromList (Util::Map (items, &PrivacyListItem::GetValue));

		bool modified = false;
		for (const auto& id : Ids_)
		{
			if (presentIds.contains (id))
				continue;

			items.prepend ({ id, PrivacyListItem::Type::Jid });
			modified = true;
		}

		if (!modified)
			return;

		list.SetItems (items);

		const auto plm = Conn_->GetPrivacyListsManager ();
		plm->SetList (list);
		if (activate)
			plm->ActivateList (list.GetName (), PrivacyListsManager::ListType::Default);
	}
}
}
}
