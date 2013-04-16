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

#ifndef INTERFACES_IMWPROXY_H
#define INTERFACES_IMWPROXY_H
#include <Qt>

class QDockWidget;
class QToolBar;
class QWidget;
class QKeySequence;
class QMenu;

/** @brief This interface is used for manipulating the main window.
 *
 * All the interaction with LeechCraft main window should be done
 * through this interface.
 */
class Q_DECL_EXPORT IMWProxy
{
public:
	enum WidgetArea
	{
		WALeft,
		WARight,
		WABottom
	};

	virtual ~IMWProxy () {}

	/** @brief Adds the given dock widget to the given area.
	 *
	 * This function merely calls QMainWindow::addDockWidget().
	 *
	 * The action for toggling the visibility of this dock widget is
	 * also added to the corresponding menus by default. The
	 * ToggleViewActionVisiblity() method could be used to change that.
	 *
	 * @param[in] area The area to add widget to.
	 * @param[in] widget The dock widget to add.
	 *
	 * @sa AssociateDockWidget(), ToggleViewActionVisiblity()
	 */
	virtual void AddDockWidget (Qt::DockWidgetArea area, QDockWidget *widget) = 0;

	/** @brief Connects the given dock widget with the given tab.
	 *
	 * This function associates the given dock widget with the given tab
	 * widget so that the dock widget is only visible when the tab is
	 * current tab.
	 *
	 * A dock widget may be associated with only one tab widget. Calling
	 * this function repeatedly will override older associations.
	 *
	 * @param[in] dock The dock widget to associate.
	 * @param[in] tab The tab widget for which the dock widget should be
	 * active.
	 *
	 * @sa AddDockWidget()
	 */
	virtual void AssociateDockWidget (QDockWidget *dock, QWidget *tab) = 0;

	/** @brief Toggles the visibility of the toggle view action.
	 *
	 * By default all newly added dock widgets have their toggle view
	 * actions shown.
	 *
	 * @param[in] widget The widget for which to update the toggle
	 * action visibility.
	 * @param[in] visible Whether the corresponding action should be
	 * visible.
	 */
	virtual void ToggleViewActionVisiblity (QDockWidget *widget, bool visible) = 0;

	/** @brief Sets the visibility action shortcut of the given widget.
	 *
	 * @param[in] widget The widget for which the visibility action
	 * shortcut.
	 * @param[in] seq The widget's visibility action shortcut sequence.
	 */
	virtual void SetViewActionShortcut (QDockWidget *widget, const QKeySequence& seq) = 0;

	/** @brief Adds the given toolbar at the given area.
	 *
	 * If the toolbar is already added, it will be just moved to the
	 * area.
	 *
	 * @param[in] toolbar The toolbar to add.
	 * @param[in] area The area where the toolbar should be added.
	 */
	virtual void AddToolbar (QToolBar *toolbar, Qt::ToolBarArea area = Qt::TopToolBarArea) = 0;

	/** @brief Adds the given widget at the given area.
	 *
	 * @param[in] widget The widget to add.
	 * @param[in] area The area where the widget should be added.
	 */
	virtual void AddSideWidget (QWidget *widget, WidgetArea area = WALeft) = 0;

	/** @brief Toggles the visibility of the main window.
	 */
	virtual void ToggleVisibility () = 0;

	virtual QMenu* GetMainMenu () = 0;

	virtual void HideMainMenu () = 0;
};

Q_DECLARE_INTERFACE (IMWProxy, "org.Deviant.LeechCraft.IMWProxy/1.0");

#endif
