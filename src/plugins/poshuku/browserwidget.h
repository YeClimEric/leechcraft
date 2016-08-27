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

#include <memory>
#include <QWidget>
#include <QTime>
#include <QMenu>
#include <qwebpage.h>
#include <interfaces/ihavetabs.h>
#include <interfaces/idndtab.h>
#include <interfaces/iwebbrowser.h>
#include <interfaces/ihaveshortcuts.h>
#include <interfaces/structures.h>
#include <interfaces/ihaverecoverabletabs.h>
#include <interfaces/iwkfontssettable.h>
#include <interfaces/core/ihookproxy.h>
#include <interfaces/poshuku/poshukutypes.h>
#include "interfaces/poshuku/ibrowserwidget.h"
#include "ui_browserwidget.h"

class QToolBar;
class QDataStream;
class QShortcut;
class QWebFrame;
class QLabel;
class QWebInspector;

namespace LeechCraft
{
namespace Poshuku
{
	class FindDialog;
	class PasswordRemember;
	struct BrowserWidgetSettings;
	class CustomWebView;

	class BrowserWidget : public QWidget
						, public IBrowserWidget
						, public IWebWidget
						, public ITabWidget
						, public IDNDTab
						, public IRecoverableTab
						, public IWkFontsSettable
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Poshuku::IBrowserWidget
				IWebWidget
				ITabWidget
				IDNDTab
				IRecoverableTab
				IWkFontsSettable)

		Ui::BrowserWidget Ui_;

		QToolBar *ToolBar_;
		QAction *Add2Favorites_;
		QAction *Find_;
		QAction *Print_;
		QAction *PrintPreview_;
		QAction *ScreenSave_;
		QAction *ViewSources_;
		QAction *SavePage_;

		QAction *ZoomIn_;
		QAction *ZoomOut_;
		QAction *ZoomReset_;

		QAction *TextZoomIn_;
		QAction *TextZoomOut_;
		QAction *TextZoomReset_;

		QAction *Cut_;
		QAction *Copy_;
		QAction *Paste_;
		QAction *Back_;
		QMenu *BackMenu_;
		QAction *Forward_;
		QMenu *ForwardMenu_;
		QAction *Reload_;
		QAction *Stop_;
		QAction *ReloadStop_;
		QAction *ReloadPeriodically_;
		QAction *NotifyWhenFinished_;
		QAction *HistoryAction_;
		QAction *BookmarksAction_;
		QPoint OnLoadPos_;
		QMenu *ChangeEncoding_;
		FindDialog *FindDialog_;
		PasswordRemember *RememberDialog_;
		QTimer *ReloadTimer_;
		QString PreviousFindText_;
		bool HtmlMode_ = false;
		bool Own_ = true;
		QMap<QString, QList<QAction*>> WindowMenus_;

		CustomWebView *WebView_;
		QLabel *LinkTextItem_;

		static QObject* S_MultiTabsParent_;
	public:
		BrowserWidget (QWidget* = nullptr);
		virtual ~BrowserWidget ();
		static void SetParentMultiTabs (QObject*);

		void Deown ();
		void FinalizeInit ();

		QLineEdit* GetURLEdit () const override;
		IWebView* GetWebView () const override;
		void InsertFindAction (QMenu*, const QString&) override;
		void AddStandardActions (QMenu*) override;

		QObject* GetQObject () override;

		BrowserWidgetSettings GetWidgetSettings () const;
		void SetWidgetSettings (const BrowserWidgetSettings&);
		void SetURL (const QUrl&);

		void Load (const QString&) override;
		void SetHtml (const QString&, const QUrl& = QUrl ()) override;
		void SetNavBarVisible (bool) override;
		void SetEverythingElseVisible (bool) override;
		QWidget* Widget () override;

		void SetShortcut (const QString&, const QKeySequences_t&);
		QMap<QString, ActionInfo> GetActionInfo () const;

		void Remove () override;
		QToolBar* GetToolBar () const override;
		QList<QAction*> GetTabBarContextMenuActions () const override;
		QMap<QString, QList<QAction*>> GetWindowMenus () const override;
		QObject* ParentMultiTabs () override;
		TabClassInfo GetTabClassInfo () const override;

		void FillMimeData (QMimeData*) override;
		void HandleDragEnter (QDragMoveEvent*) override;
		void HandleDrop (QDropEvent*) override;

		void SetTabRecoverData (const QByteArray&);
		QByteArray GetTabRecoverData () const override;
		QString GetTabRecoverName () const override;
		QIcon GetTabRecoverIcon () const override;

		void SetFontFamily (QWebSettings::FontFamily family, const QFont& font) override;
		void SetFontSize (QWebSettings::FontSize type, int size) override;
		void SetFontSizeMultiplier (qreal factor) override;

		void SetOnLoadScrollPoint (const QPoint&);
	protected:
		void keyReleaseEvent (QKeyEvent*) override;
	private:
		void SetActualReloadInterval (const QTime&);
		void SetSplitterSizes (int);
	public slots:
		void focusLineEdit ();
		void handleShortcutHistory ();
		void handleShortcutBookmarks ();
		void loadURL (const QUrl&);
		QWebView* getWebView () const;
		QLineEdit* getAddressBar () const;
		QWidget* getSideBar () const;
	private slots:
		void handleIconChanged ();
		void handleStatusBarMessage (const QString&);
		void handleURLFrameLoad (QString);
		void handleReloadPeriodically ();
		void handleAdd2Favorites ();
		void handleFind ();
		void handleScreenSave ();
		void handleViewSources ();
		void handleSavePage ();
		void enableActions ();

		void updateTitle (const QString&);

		void updateNavHistory ();
		void handleBackHistoryAction ();
		void handleForwardHistoryAction ();

		void checkLoadedDocument ();

		void handleContextMenu (const QPoint&, const ContextMenuInfo&);

		void setScrollPosition ();
		void pageFocus ();
		void handleLoadProgress (int);
		void notifyLoadFinished (bool);
		void handleChangeEncodingAboutToShow ();
		void handleChangeEncodingTriggered (QAction*);
		void updateLogicalPath ();
		void handleUrlChanged (const QString&);
	signals:
		void titleChanged (const QString&);
		void urlChanged (const QString&);
		void iconChanged (const QIcon&);
		void needToClose ();
		void tooltipChanged (QWidget*);
		void addToFavorites (const QString&, const QString&);
		void statusBarChanged (const QString&);
		void gotEntity (const LeechCraft::Entity&);
		void delegateEntity (const LeechCraft::Entity&, int*, QObject**);
		void couldHandle (const LeechCraft::Entity&, bool*);
		void raiseTab (QWidget*);
		void tabRecoverDataChanged () override;

		// Hook support
		void hookBrowserWidgetInitialized (LeechCraft::IHookProxy_ptr proxy,
				QObject *browserWidget);
		void hookIconChanged (LeechCraft::IHookProxy_ptr proxy,
				QWebPage *page,
				QObject *browserWidget);
		void hookLoadProgress (LeechCraft::IHookProxy_ptr proxy,
				QWebPage *page,
				QObject *browserWidget,
				int progress);
		void hookMoreMenuFillBegin (LeechCraft::IHookProxy_ptr proxy,
				QMenu *menu,
				QObject *browserWidget);
		void hookMoreMenuFillEnd (LeechCraft::IHookProxy_ptr proxy,
				QMenu *menu,
				QObject *browserWidget);
		void hookNotifyLoadFinished (LeechCraft::IHookProxy_ptr proxy,
				QWebView *view,
				QObject *browserWidget,
				bool ok,
				bool notifyWhenFinished,
				bool own,
				bool htmlMode);
		void hookSetURL (LeechCraft::IHookProxy_ptr proxy,
				QObject *browserWidget,
				QUrl url);
		void hookStatusBarMessage (LeechCraft::IHookProxy_ptr proxy,
				QObject *browserWidget,
				QString message);
		void hookTabBarContextMenuActions (LeechCraft::IHookProxy_ptr proxy,
				const QObject *browserWidget) const;
		void hookTabRemoveRequested (LeechCraft::IHookProxy_ptr proxy,
				QObject *browserWidget);
		void hookUpdateLogicalPath (LeechCraft::IHookProxy_ptr proxy,
				QObject *browserWidget);
		void hookURLEditReturnPressed (LeechCraft::IHookProxy_ptr proxy,
				QObject *browserWidget);
		void hookWebViewContextMenu (LeechCraft::IHookProxy_ptr,
				LeechCraft::Poshuku::IWebView*,
				const LeechCraft::Poshuku::ContextMenuInfo& hitTestResult,
				QMenu*,
				WebViewCtxMenuStage);
	};
}
}
