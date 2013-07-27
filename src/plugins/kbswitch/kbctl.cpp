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

#include "kbctl.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

namespace LeechCraft
{
namespace KBSwitch
{
	KBCtl::KBCtl (QObject *parent)
	: QObject (parent)
	{
		InitDisplay ();

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

		CheckExtWM ();

		if (!ExtWM_)
			SetupNonExtListeners ();
	}

	KBCtl::~KBCtl ()
	{
		XCloseDisplay (Display_);
	}

	void KBCtl::InitDisplay ()
	{
		int xkbError, xkbReason;
		int mjr = XkbMajorVersion, mnr = XkbMinorVersion;
		Display_ = XkbOpenDisplay (nullptr,
				&XkbEventType_,
				&xkbError,
				&mjr,
				&mnr,
				&xkbReason);
		Window_ = DefaultRootWindow (Display_);

		NetActiveWinAtom_ = XInternAtom (Display_, "_NET_ACTIVE_WINDOW", 0);
	}

	void KBCtl::CheckExtWM ()
	{
		Atom type;
		int format;
		uchar *prop = nullptr;
		ulong count, after;
		auto ret = XGetWindowProperty (Display_, Window_, NetActiveWinAtom_,
				0, sizeof (Window), 0, XA_WINDOW, &type, &format, &count, &after, &prop);
		if (ret == Success && prop)
		{
			XFree (prop);
			ExtWM_ = true;
		}
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

	void KBCtl::AssignWindow (Qt::HANDLE window)
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
}
}
