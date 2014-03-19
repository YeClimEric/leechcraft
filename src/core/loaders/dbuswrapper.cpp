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

#include "dbuswrapper.h"
#include <QDBusInterface>
#include <QDBusReply>
#include <interfaces/core/icoreproxy.h>

namespace LeechCraft
{
namespace Loaders
{
	DBusWrapper::DBusWrapper (const QString& service)
	: Service_ { service }
	, IFace_ { new QDBusInterface { service, "/org/LeechCraft/Plugin" } }
	, Info_ { new QDBusInterface { service, "/org/LeechCraft/Info" } }
	{
	}

	void DBusWrapper::Init (ICoreProxy_ptr proxy)
	{
		qDebug () << Q_FUNC_INFO;
		Info_->call ("Init", QVariant::fromValue (proxy));
		qDebug () << "done";
	}

	void DBusWrapper::SecondInit ()
	{
		Info_->call ("SecondInit");
	}

	void DBusWrapper::Release ()
	{
		Info_->call ("Release");
	}

	QByteArray DBusWrapper::GetUniqueID () const
	{
		return QDBusReply<QByteArray> (Info_->call ("GetUniqueID")).value ();
	}

	QString DBusWrapper::GetName () const
	{
		return QDBusReply<QString> (Info_->call ("GetName")).value ();
	}

	QString DBusWrapper::GetInfo () const
	{
		return QDBusReply<QString> (Info_->call ("GetInfo")).value ();
	}

	QIcon DBusWrapper::GetIcon () const
	{
		return QDBusReply<QIcon> (Info_->call ("GetIcon")).value ();
	}
}
}
