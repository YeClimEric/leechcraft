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

#include "dbuspluginloader.h"
#include <QProcess>
#include <QDir>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusConnectionInterface>
#include <QLocalServer>
#include <interfaces/iinfo.h>
#include "infoproxy.h"
#include "dbus/marshalling.h"

namespace LeechCraft
{
namespace Loaders
{
	DBusPluginLoader::DBusPluginLoader (const QString& filename)
	: Filename_ (filename)
	, IsLoaded_ (false)
	, Proc_ (new QProcess)
	{
		DBus::RegisterTypes ();

		auto sb = QDBusConnection::sessionBus ();
		const QString serviceName { "org.LeechCraft.MainInstance" };

		if (!sb.interface ()->isServiceRegistered (serviceName))
			qDebug () << "registering primary service..." << sb.registerService (serviceName);

		connect (Proc_.get (),
				SIGNAL (finished (int, QProcess::ExitStatus)),
				this,
				SLOT (handleProcFinished ()));
	}

	quint64 DBusPluginLoader::GetAPILevel ()
	{
		return GetLibAPILevel (Filename_);
	}

	bool DBusPluginLoader::Load ()
	{
		if (IsLoaded ())
			return true;

		Proc_->start ("lc_plugin_wrapper");

		QLocalServer srv;
		srv.listen (QString ("lc_waiter_%1").arg (Proc_->pid ()));

		if (!Proc_->waitForStarted ())
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot start proc";
			return false;
		}

		if (!srv.waitForNewConnection (1000))
		{
			qWarning () << Q_FUNC_INFO
					<< "time out waiting for connection"
					<< Filename_;
			return false;
		}

		const auto& serviceName = QString ("org.LeechCraft.Wrapper_%1").arg (Proc_->pid ());
		CtrlIface_.reset (new QDBusInterface (serviceName,
					"/org/LeechCraft/Control",
					"org.LeechCraft.Control"));

		QDBusReply<bool> reply = CtrlIface_->call ("Load", Filename_);
		IsLoaded_ = reply.value ();
		qDebug () << Q_FUNC_INFO
				<< GetFileName ()
				<< "is loaded?"
				<< IsLoaded_;
		if (!IsLoaded_)
			return false;

		Wrapper_.reset (new InfoProxy (serviceName));

		CtrlIface_->call ("SetLcIconsPaths", QDir::searchPaths ("lcicons"));

		return true;
	}

	bool DBusPluginLoader::Unload ()
	{
		if (!IsLoaded ())
			return true;

		QDBusReply<bool> reply = CtrlIface_->call ("Unload", Filename_);
		if (reply.value ())
		{
			CtrlIface_.reset ();
			IsLoaded_ = false;
		}

		return !IsLoaded_;
	}

	QObject* DBusPluginLoader::Instance ()
	{
		if (!IsLoaded () && !Load ())
		{
			qWarning () << Q_FUNC_INFO
					<< "null instance";
			return 0;
		}

		return Wrapper_.get ();
	}

	bool DBusPluginLoader::IsLoaded () const
	{
		return IsLoaded_;
	}

	QString DBusPluginLoader::GetFileName () const
	{
		return Filename_;
	}

	QString DBusPluginLoader::GetErrorString () const
	{
		return {};
	}

	QVariantMap DBusPluginLoader::GetManifest () const
	{
		// TODO
		return {};
	}

	void DBusPluginLoader::handleProcFinished ()
	{
		qDebug () << Q_FUNC_INFO << Proc_->exitCode () << Proc_->exitStatus ();
		qDebug () << Proc_->readAllStandardOutput ();
		qDebug () << Proc_->readAllStandardError ();
	}
}
}
