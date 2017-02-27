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

#include "kbctl.h"
#include <algorithm>
#include <QtDebug>
#include <QTimer>
#include <QProcess>
#include <QCoreApplication>
#include <QAbstractEventDispatcher>

#if QT_VERSION >= 0x050000
#include <QGuiApplication>
#endif

#include <util/x11/xwrapper.h>
#include "xmlsettingsmanager.h"
#include "rulesstorage.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#if QT_VERSION >= 0x050000
#include <xcb/xcb.h>

#define explicit xcb_authors_dont_care_about_cplusplus
#include <xcb/xkb.h>
#undef explicit

#endif

namespace LeechCraft
{
namespace KBSwitch
{
	namespace
	{
#if QT_VERSION < 0x050000
		bool EvFilter (void *msg)
		{
			return KBCtl::Instance ().Filter (static_cast<XEvent*> (msg));
		}
#endif
	}

	KBCtl::KBCtl ()
#if QT_VERSION < 0x050000
	: PrevFilter_ (QAbstractEventDispatcher::instance ()->setEventFilter (EvFilter))
#endif
	{
#if QT_VERSION >= 0x050000
		QAbstractEventDispatcher::instance ()->installNativeEventFilter (this);
#endif
		InitDisplay ();

		Rules_ = new RulesStorage (Display_);

#if QT_VERSION < 0x050000
		XWindowAttributes wa;
		XGetWindowAttributes (Display_, Window_, &wa);
		const auto rootEvents = StructureNotifyMask |
				SubstructureNotifyMask |
				PropertyChangeMask |
				FocusChangeMask |
				KeymapStateMask |
				LeaveWindowMask |
				EnterWindowMask;
		XSelectInput (Display_, Window_, wa.your_event_mask | rootEvents);

		XkbSelectEventDetails (Display_, XkbUseCoreKbd,
				XkbStateNotify, XkbAllStateComponentsMask, XkbGroupStateMask);
#else
		const auto conn = QX11Info::connection ();

		const uint32_t rootEvents [] =
		{
			XCB_EVENT_MASK_STRUCTURE_NOTIFY |
				XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
				XCB_EVENT_MASK_PROPERTY_CHANGE |
				XCB_EVENT_MASK_FOCUS_CHANGE |
				XCB_EVENT_MASK_KEYMAP_STATE |
				XCB_EVENT_MASK_LEAVE_WINDOW |
				XCB_EVENT_MASK_ENTER_WINDOW
		};
		xcb_change_window_attributes (conn,
				Window_, XCB_CW_EVENT_MASK, rootEvents);

		const uint16_t requiredMapParts = 0xffff;

		const uint16_t requiredEvents = XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
				XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
				XCB_XKB_EVENT_TYPE_STATE_NOTIFY;

		xcb_xkb_select_events (conn,
				XCB_XKB_ID_USE_CORE_KBD,
				requiredEvents,
				0,
				requiredEvents,
				requiredMapParts,
				requiredMapParts,
				nullptr);
#endif

		CheckExtWM ();

		if (!ExtWM_)
			SetupNonExtListeners ();

		UpdateGroupNames ();

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_KBSwitch");
		settings.beginGroup ("Groups");
		SetEnabledGroups (settings.value ("Groups").toStringList ());
		SetGroupVariants (settings.value ("Variants").toStringList ());
		settings.endGroup ();

		XmlSettingsManager::Instance ().RegisterObject ({
					"ManageSystemWide",
					"KeyboardModel",
					"ManageKeyRepeat",
					"RepeatRate",
					"RepeatTimeout"
				},
				this, "scheduleApply");
	}

	void KBCtl::InitDisplay ()
	{
#if QT_VERSION < 0x050000
		int xkbError, xkbReason;
		int mjr = XkbMajorVersion, mnr = XkbMinorVersion;
		Display_ = XkbOpenDisplay (nullptr,
				&XkbEventType_,
				&xkbError,
				&mjr,
				&mnr,
				&xkbReason);
#else
		Display_ = QX11Info::display ();

		const auto conn = QX11Info::connection ();
		const auto reply = xcb_get_extension_data (conn, &xcb_xkb_id);

		if (!reply || !reply->present)
		{
			qWarning () << Q_FUNC_INFO
					<< "XKB extension not present";
		}

		XkbEventType_ = reply->first_event;

		xcb_xkb_use_extension (conn, XCB_XKB_MAJOR_VERSION, XCB_XKB_MINOR_VERSION);
#endif

		Window_ = DefaultRootWindow (Display_);
		NetActiveWinAtom_ = Util::XWrapper::Instance ().GetAtom ("_NET_ACTIVE_WINDOW");
	}

	KBCtl& KBCtl::Instance ()
	{
		static KBCtl ctl;
		return ctl;
	}

	void KBCtl::Release ()
	{
		XCloseDisplay (Display_);
	}

	void KBCtl::SetSwitchPolicy (SwitchPolicy policy)
	{
		Policy_ = policy;
	}

	int KBCtl::GetCurrentGroup () const
	{
		XkbStateRec state;
		XkbGetState (Display_, XkbUseCoreKbd, &state);
		return state.group;
	}

	const QStringList& KBCtl::GetEnabledGroups () const
	{
		return Groups_;
	}

	void KBCtl::SetEnabledGroups (QStringList groups)
	{
		if (groups.isEmpty ())
			return;

		if (groups.contains ("us") && groups.at (0) != "us")
		{
			groups.removeAll ("us");
			groups.prepend ("us");
		}

		if (Groups_ == groups)
			return;

		Groups_ = groups;
		scheduleApply ();
	}

	QString KBCtl::GetGroupVariant (int groupIdx) const
	{
		return Variants_.value (groupIdx);
	}

	void KBCtl::SetGroupVariants (const QStringList& variants)
	{
		if (variants.isEmpty ())
			return;

		Variants_ = variants;
		scheduleApply ();
	}

	void KBCtl::EnableNextGroup ()
	{
		const int count = GetEnabledGroups ().count ();
		EnableGroup ((GetCurrentGroup () + 1) % count);
	}

	void KBCtl::EnableGroup (int group)
	{
		XkbLockGroup (Display_, XkbUseCoreKbd, group);

		/* What an utter crap X11 is actually. The group doesn't get
		 * updated by the line above until we make another request to
		 * the X server, which this line basically does.
		 *
		 * Dunno why I'm writing this as I don't write comments for code
		 * at all. Probably for easy grepping by "crap" or "X!1".
		 */
		GetCurrentGroup ();
	}

	int KBCtl::GetMaxEnabledGroups () const
	{
		return XkbNumKbdGroups;
	}

	QString KBCtl::GetLayoutName (int group) const
	{
		return Groups_.value (group);
	}

	QString KBCtl::GetLayoutDesc (int group) const
	{
		return Rules_->GetLayoutsN2D () [GetLayoutName (group)];
	}

	void KBCtl::SetOptions (const QStringList& opts)
	{
		if (Options_ == opts)
			return;

		Options_ = opts;
		Options_.sort ();
		scheduleApply ();
	}

	const RulesStorage* KBCtl::GetRulesStorage () const
	{
		return Rules_;
	}

#if QT_VERSION < 0x050000
	bool KBCtl::Filter (XEvent *event)
	{
		auto invokePrev = [this, event] { return PrevFilter_ ? PrevFilter_ (event) : false; };

		if (event->type == XkbEventType_)
		{
			HandleXkbEvent (event);
			return invokePrev ();
		}

		switch (event->type)
		{
		case FocusIn:
		case FocusOut:
		case PropertyNotify:
			SetWindowLayout (Util::XWrapper::Instance ().GetActiveApp ());
			break;
		case CreateNotify:
		{
			const auto window = event->xcreatewindow.window;
			AssignWindow (window);
			break;
		}
		case DestroyNotify:
			Win2Group_.remove (event->xdestroywindow.window);
			break;
		}

		return invokePrev ();
	}

	void KBCtl::HandleXkbEvent (XEvent *event)
	{
		XkbEvent xkbEv;
		xkbEv.core = *event;

		switch (xkbEv.any.xkb_type)
		{
		case XkbStateNotify:
			if (xkbEv.state.group == xkbEv.state.locked_group)
				Win2Group_ [Util::XWrapper::Instance ().GetActiveApp ()] = xkbEv.state.group;
			emit groupChanged (xkbEv.state.group);
			break;
		case XkbNewKeyboardNotify:
			Win2Group_.clear ();
			UpdateGroupNames ();
			break;
		default:
			break;
		}
	}
#else
	bool KBCtl::nativeEventFilter (const QByteArray& eventType, void *msg, long int*)
	{
		if (eventType != "xcb_generic_event_t")
			return false;

		const auto ev = static_cast<xcb_generic_event_t*> (msg);

		if ((ev->response_type & ~0x80) == XkbEventType_)
			HandleXkbEvent (msg);

		switch (ev->response_type & ~0x80)
		{
		case XCB_FOCUS_IN:
		case XCB_FOCUS_OUT:
		case XCB_PROPERTY_NOTIFY:
			SetWindowLayout (Util::XWrapper::Instance ().GetActiveApp ());
			break;
		case XCB_CREATE_NOTIFY:
			AssignWindow (static_cast<xcb_create_notify_event_t*> (msg)->window);
			break;
		case XCB_DESTROY_NOTIFY:
			Win2Group_.remove (static_cast<xcb_destroy_notify_event_t*> (msg)->window);
			break;
		}

		return false;
	}

	void KBCtl::HandleXkbEvent (void *msg)
	{
		const auto ev = static_cast<xcb_generic_event_t*> (msg);
		switch (ev->pad0)
		{
		case XCB_XKB_STATE_NOTIFY:
		{
			const auto stateEv = static_cast<xcb_xkb_state_notify_event_t*> (msg);
			if (stateEv->group == stateEv->lockedGroup)
				Win2Group_ [Util::XWrapper::Instance ().GetActiveApp ()] = stateEv->group;
			emit groupChanged (stateEv->group);
			break;
		}
		case XCB_XKB_NEW_KEYBOARD_NOTIFY:
			Win2Group_.clear ();
			UpdateGroupNames ();
			break;
		}
	}
#endif

	void KBCtl::SetWindowLayout (ulong window)
	{
		if (Policy_ != SwitchPolicy::PerWindow)
			return;

		if (window == None)
			return;

		if (!Win2Group_.contains (window))
			return;

		const auto group = Win2Group_ [window];
		XkbLockGroup (Display_, XkbUseCoreKbd, group);

		/* See comments in SetGroup() for details of X11 crappiness.
		 */
		GetCurrentGroup ();
	}

	void KBCtl::CheckExtWM ()
	{
		Atom type;
		int format;
		uchar *prop = nullptr;
		ulong count, after;
		const auto ret = XGetWindowProperty (Display_, Window_, NetActiveWinAtom_,
				0, sizeof (Window), 0, XA_WINDOW,
				&type, &format, &count, &after, &prop);
		if (ret == Success && prop)
		{
			XFree (prop);
			ExtWM_ = true;
		}
		else
			qWarning () << Q_FUNC_INFO
					<< "extended window manager hints support wasn't detected, this won't work";
	}

	void KBCtl::SetupNonExtListeners ()
	{
		uint count = 0;
		Window d1, d2;
		Window *windows = nullptr;

		if (!XQueryTree (Display_, Window_, &d1, &d2, &windows, &count))
			return;

		for (uint i = 0; i < count; ++i)
			AssignWindow (windows [i]);

		if (windows)
			XFree (windows);
	}

	void KBCtl::UpdateGroupNames ()
	{
		auto desc = XkbAllocKeyboard ();
		XkbGetControls (Display_, XkbAllControlsMask, desc);
		XkbGetNames (Display_, XkbSymbolsNameMask | XkbGroupNamesMask, desc);

		if (!desc->names)
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot get names";
			return;
		}

		Groups_.clear ();

		const auto group = desc->names->groups;
		size_t groupCount = 0;
		for (; groupCount < XkbNumKbdGroups && group [groupCount]; ++groupCount) ;

		char **result = new char* [groupCount];
		XGetAtomNames (Display_, group, groupCount, result);

		const auto& layoutsD2N = Rules_->GetLayoutsD2N ();
		const auto& varredLayouts = Rules_->GetVariantsD2Layouts ();
		for (size_t i = 0; i < groupCount; ++i)
		{
			const QString str (result [i]);
			XFree (result [i]);

			if (!layoutsD2N [str].isEmpty ())
			{
				const auto& grp = layoutsD2N [str];
				Groups_ << grp;
				Variants_ << QString ();
			}
			else if (!varredLayouts [str].first.isEmpty ())
			{
				const auto& grp = varredLayouts [str];
				Groups_ << grp.first;
				Variants_ << grp.second;
			}
			else
			{
				qWarning () << Q_FUNC_INFO
						<< str
						<< "not present anywhere";
				qWarning () << varredLayouts.contains (str);
				continue;
			}
		}
		delete [] result;

		XkbFreeNames (desc, XkbSymbolsNameMask | XkbGroupNamesMask, True);
	}

	void KBCtl::AssignWindow (ulong window)
	{
		if (ExtWM_)
			return;

		XWindowAttributes wa;
		if (!XGetWindowAttributes (Display_, window, &wa))
			return;

		const auto windowEvents = EnterWindowMask |
				FocusChangeMask |
				PropertyChangeMask |
				StructureNotifyMask;
		XSelectInput (Display_, window, windowEvents);
	}

	void KBCtl::scheduleApply ()
	{
		if (ApplyScheduled_)
			return;

		ApplyScheduled_ = true;
		QTimer::singleShot (100,
				this,
				SLOT (apply ()));
	}

	void KBCtl::ApplyKeyRepeat ()
	{
		if (!XmlSettingsManager::Instance ().property ("ManageKeyRepeat").toBool ())
			return;

		XkbChangeEnabledControls (Display_, XkbUseCoreKbd, XkbRepeatKeysMask, XkbRepeatKeysMask);

		auto timeout = XmlSettingsManager::Instance ().property ("RepeatTimeout").toUInt ();
		auto rate = XmlSettingsManager::Instance ().property ("RepeatRate").toUInt ();
		XkbSetAutoRepeatRate (Display_, XkbUseCoreKbd, timeout, 1000 / rate);

		// X11 is crap, XkbSetAutoRepeatRate() doesn't work next time if we don't call this.
		XkbGetAutoRepeatRate (Display_, XkbUseCoreKbd, &timeout, &rate);
	}

	void KBCtl::apply ()
	{
		ApplyScheduled_ = false;

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_KBSwitch");
		settings.beginGroup ("Groups");
		settings.remove ("");
		settings.setValue ("Groups", Groups_);
		settings.setValue ("Variants", Variants_);
		settings.endGroup ();

		if (!XmlSettingsManager::Instance ()
				.property ("ManageSystemWide").toBool ())
			return;

		auto kbModel = XmlSettingsManager::Instance ()
				.property ("KeyboardModel").toString ();
		const auto& kbCode = Rules_->GetKBModelCode (kbModel);

		QStringList args
		{
			"-layout",
			Groups_.join (","),
			"-model",
			kbCode,
			"-option"
		};

		if (!Options_.isEmpty ())
			args << "-option"
					<< Options_.join (",");

		if (std::any_of (Variants_.begin (), Variants_.end (),
				[] (const QString& str) { return !str.isEmpty (); }))
			args << "-variant"
					<< Variants_.join (",");

		qDebug () << Q_FUNC_INFO << args;
		QProcess::startDetached ("setxkbmap", args);

		ApplyKeyRepeat ();
	}
}
}
