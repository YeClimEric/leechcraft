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

#include <functional>
#include <QObject>
#include <QDateTime>
#include <QUrl>
#include <interfaces/core/icoreproxy.h>
#include "svcauthconfig.h"

class QTimer;

namespace LeechCraft
{
namespace Util
{
class QueueManager;
enum class QueuePriority;

class CustomCookieJar;

namespace SvcAuth
{
	class UTIL_SVCAUTH_API VkAuthManager : public QObject
	{
		Q_OBJECT

		ICoreProxy_ptr Proxy_;

		const QString AccountHR_;

		QNetworkAccessManager *AuthNAM_;
		Util::CustomCookieJar *Cookies_;

		QueueManager * const Queue_;

		QString Token_;
		QDateTime ReceivedAt_;
		qint32 ValidFor_;

		bool IsRequesting_;

		const QString ID_;
		QUrl URL_;

		bool IsRequestScheduled_;
		QTimer *ScheduleTimer_;

		bool SilentMode_ = false;
	public:
		typedef QList<std::function<void (QString)>> RequestQueue_t;
		typedef RequestQueue_t* RequestQueue_ptr;

		typedef QList<QPair<std::function<void (QString)>, QueuePriority>> PrioRequestQueue_t;
		typedef PrioRequestQueue_t* PrioRequestQueue_ptr;
	private:
		QList<RequestQueue_ptr> ManagedQueues_;
		QList<PrioRequestQueue_ptr> PrioManagedQueues_;
	public:
		VkAuthManager (const QString& accountName, const QString& clientId,
				const QStringList& scope, const QByteArray& cookies,
				ICoreProxy_ptr, QueueManager* = nullptr, QObject* = nullptr);

		bool IsAuthenticated () const;
		bool HadAuthentication () const;

		void UpdateScope (const QStringList&);

		void GetAuthKey ();

		void ManageQueue (RequestQueue_ptr);
		void UnmanageQueue (RequestQueue_ptr);

		void ManageQueue (PrioRequestQueue_ptr);
		void UnmanageQueue (PrioRequestQueue_ptr);

		void SetSilentMode (bool);
	private:
		void InvokeQueues (const QString&);

		void RequestURL (const QUrl&);
		void RequestAuthKey ();
		bool CheckIsBlank (QUrl);
	public slots:
		void clearAuthData ();
		void reauth ();
	private slots:
		void execScheduledRequest ();
		void handleGotForm ();
		void handleViewUrlChanged (const QUrl&);
	signals:
		void gotAuthKey (const QString&);
		void cookiesChanged (const QByteArray&);
		void authCanceled ();
		void justAuthenticated ();
	};
}
}
}
