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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QDialog>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QList>
#include <QModelIndex>
#include <QToolButton>
#include "ui_leechcraft.h"
#include "interfaces/core/ihookproxy.h"

class QLabel;
class QDockWidget;
class QModelIndex;
class QToolBar;
class IShortcutProxy;
class QToolButton;
class QShortcut;
class QSplashScreen;
class QSystemTrayIcon;
class QWidgetAction;

namespace LeechCraft
{
	namespace Util
	{
		class XmlSettingsDialog;
	};

	class Core;
	class PluginInfo;
	class PluginManagerDialog;
	class ShortcutManager;
	class MainWindowMenuManager;

	class MainWindow : public QMainWindow
	{
		Q_OBJECT

		Ui::LeechCraft Ui_;

		const bool IsPrimary_;
		const int WindowIdx_;

		QSystemTrayIcon *TrayIcon_ = nullptr;
		bool IsShown_ = true;
		bool WasMaximized_ = false;
		QShortcut *FullScreenShortcut_;
		QShortcut *CloseTabShortcut_;

		QToolBar *QLBar_;

		QToolButton *MenuButton_;
		QWidgetAction *MBAction_;
		MainWindowMenuManager *MenuManager_;

		QToolBar *LeftDockToolbar_;
		QToolBar *RightDockToolbar_;
		QToolBar *TopDockToolbar_;
		QToolBar *BottomDockToolbar_;
	public:
		MainWindow (int screen, bool isPrimary, int windowIdx);

		void Init ();

		SeparateTabWidget* GetTabWidget () const;
		QSplitter* GetMainSplitter () const;
		void SetAdditionalTitle (const QString&);

		QMenu* GetMainMenu () const;
		void HideMainMenu ();

		QToolBar* GetDockListWidget (Qt::DockWidgetArea) const;

		MainWindowMenuManager* GetMenuManager () const;

		QMenu* createPopupMenu () override;
	public slots:
		void catchError (QString);
		void showHideMain ();
		void showMain ();
		void showFirstTime ();

		void handleQuit ();
	protected:
		void closeEvent (QCloseEvent*) override;
		void keyPressEvent (QKeyEvent*) override;

		void dragEnterEvent (QDragEnterEvent*) override;
		void dropEvent (QDropEvent*) override;
	private:
		void InitializeInterface ();
		void SetStatusBar ();
		void ReadSettings ();
		void WriteSettings ();
	private slots:
		void on_ActionAddTask__triggered ();
		void on_ActionNewWindow__triggered ();
		void on_ActionCloseTab__triggered ();
		void handleCloseCurrentTab ();
		void on_ActionSettings__triggered ();
		void on_ActionAboutLeechCraft__triggered ();
		void on_ActionRestart__triggered ();
		void on_ActionQuit__triggered ();
		void on_ActionShowStatusBar__triggered ();
		void on_ActionFullscreenMode__triggered (bool);
		void handleShortcutFullscreenMode ();
		void handleToolButtonStyleChanged ();
		void handleShowTrayIconChanged ();
		void handleNewTabMenuRequested ();
		void handleCurrentTabChanged (int);
		void handleRestoreActionAdded (QAction*);
		void handleTrayIconActivated (QSystemTrayIcon::ActivationReason);
		void handleWorkAreaResized (int);
		void doDelayedInit ();
	private:
		void FillQuickLaunch ();
		void FillTray ();
		void FillToolMenu ();
		void InitializeShortcuts ();
		void ShowMenuAndBar (bool);
	signals:
		void hookGonnaFillMenu (LeechCraft::IHookProxy_ptr);
		void hookGonnaFillQuickLaunch (LeechCraft::IHookProxy_ptr);
		void hookGonnaShowStatusBar (LeechCraft::IHookProxy_ptr, bool);
		void hookTrayIconCreated (LeechCraft::IHookProxy_ptr, QSystemTrayIcon*);
		void hookTrayIconVisibilityChanged (LeechCraft::IHookProxy_ptr, QSystemTrayIcon*, bool);
	};
};

#endif

