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

#pragma once

#include <QObject>
#include <interfaces/iinfo.h>
#include <interfaces/iplugin2.h>
#include <interfaces/iactionsexporter.h>
#include <interfaces/ishutdownlistener.h>
#include <interfaces/core/ihookproxy.h>

namespace LeechCraft
{
namespace TabSessManager
{
	class SessionMenuManager;
	class SessionsManager;
	class UncloseManager;
	class TabsPropsManager;

	class Plugin : public QObject
				 , public IInfo
				 , public IPlugin2
				 , public IActionsExporter
				 , public IShutdownListener
	{
		Q_OBJECT
		Q_INTERFACES (IInfo IPlugin2 IActionsExporter IShutdownListener)

		LC_PLUGIN_METADATA ("org.LeechCraft.TabSessManager")

		ICoreProxy_ptr Proxy_;

		std::shared_ptr<TabsPropsManager> TabsPropsMgr_;

		UncloseManager *UncloseMgr_;

		SessionsManager *SessionsMgr_;
		SessionMenuManager *SessionMenuMgr_;
	public:
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		QByteArray GetUniqueID () const;
		void Release ();
		QString GetName () const;
		QString GetInfo () const;
		QIcon GetIcon () const;

		QSet<QByteArray> GetPluginClasses () const;

		QList<QAction*> GetActions (ActionsEmbedPlace) const;

		void HandleShutdownInitiated ();
	public slots:
		void hookTabIsRemoving (LeechCraft::IHookProxy_ptr proxy,
				int index,
				int windowId);
		void hookTabAdding (LeechCraft::IHookProxy_ptr proxy,
				QWidget *widget);
		void hookGetPreferredWindowIndex (LeechCraft::IHookProxy_ptr proxy,
				const QWidget *widget) const;
	signals:
		void gotActions (QList<QAction*>, LeechCraft::ActionsEmbedPlace);
	};
}
}
