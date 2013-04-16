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

#ifndef PLUGINS_AZOTH_AZOTH_H
#define PLUGINS_AZOTH_AZOTH_H
#include <QObject>
#include <QTranslator>
#include <QModelIndex>
#include <interfaces/iinfo.h>
#include <interfaces/ipluginready.h>
#include <interfaces/ihavetabs.h>
#include <interfaces/ihaverecoverabletabs.h>
#include <interfaces/ihavesettings.h>
#include <interfaces/ijobholder.h>
#include <interfaces/iactionsexporter.h>
#include <interfaces/ientityhandler.h>
#include <interfaces/ihaveshortcuts.h>
#include <interfaces/ianemitter.h>

namespace LeechCraft
{
namespace Azoth
{
	class ServiceDiscoveryWidget;
	class MainWidget;
	class ConsoleWidget;
	class MicroblogsTab;

	class Plugin : public QObject
				 , public IInfo
				 , public IPluginReady
				 , public IHaveTabs
				 , public IHaveRecoverableTabs
				 , public IHaveSettings
				 , public IJobHolder
				 , public IActionsExporter
				 , public IEntityHandler
				 , public IHaveShortcuts
				 , public IANEmitter
	{
		Q_OBJECT
		Q_INTERFACES (IInfo
				IPluginReady
				IHaveTabs
				IHaveRecoverableTabs
				IHaveSettings
				IJobHolder
				IActionsExporter
				IEntityHandler
				IHaveShortcuts
				IANEmitter)

		MainWidget *MW_;
		Util::XmlSettingsDialog_ptr XmlSettingsDialog_;
		std::auto_ptr<QTranslator> Translator_;
		TabClasses_t TabClasses_;
	public:
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		void Release ();
		QByteArray GetUniqueID () const;
		QString GetName () const;
		QString GetInfo () const;
		QIcon GetIcon () const;
		QStringList Provides () const;

		QSet<QByteArray> GetExpectedPluginClasses () const;
		void AddPlugin (QObject*);

		Util::XmlSettingsDialog_ptr GetSettingsDialog () const;

		QAbstractItemModel* GetRepresentation () const;

		QList<QAction*> GetActions (ActionsEmbedPlace) const;
		QMap<QString, QList<QAction*>> GetMenuActions () const;

		EntityTestHandleResult CouldHandle (const Entity&) const;
		void Handle (Entity);

		TabClasses_t GetTabClasses () const;
		void TabOpenRequested (const QByteArray&);

		void RecoverTabs (const QList<TabRecoverInfo>&);

		void SetShortcut (const QString&, const QKeySequences_t&);
		QMap<QString, ActionInfo> GetActionInfo() const;

		QList<ANFieldData> GetANFields () const;
	private :
		void InitShortcuts ();
		void InitSettings ();
		void InitMW ();
		void InitSignals ();
		void InitTabClasses ();
	public slots:
		void handleSDWidget (ServiceDiscoveryWidget*);
		void handleMicroblogsTab (MicroblogsTab*);
		void handleTasksTreeSelectionCurrentRowChanged (const QModelIndex&, const QModelIndex&);
	private slots:
		void handleMWLocation (Qt::DockWidgetArea);
		void handleMWFloating (bool);
		void handleMoreThisStuff (const QString&);
		void handleConsoleWidget (ConsoleWidget*);
	signals:
		void gotEntity (const LeechCraft::Entity&);
		void delegateEntity (const LeechCraft::Entity&, int*, QObject**);

		void addNewTab (const QString&, QWidget*);
		void removeTab (QWidget*);
		void changeTabName (QWidget*, const QString&);
		void changeTabIcon (QWidget*, const QIcon&);
		void changeTooltip (QWidget*, QWidget*);
		void statusBarChanged (QWidget*, const QString&);
		void raiseTab (QWidget*);

		void gotActions (QList<QAction*>, LeechCraft::ActionsEmbedPlace);
	};
}
}

#endif

