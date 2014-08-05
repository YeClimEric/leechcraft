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

#include <atomic>
#include <memory>
#include <functional>
#include <QThread>
#include <QMutex>
#include <QFuture>
#include <interfaces/azoth/iclentry.h>
#include "threadexceptions.h"

typedef struct Tox Tox;

namespace LeechCraft
{
namespace Azoth
{
namespace Sarin
{
	class ToxThread : public QThread
	{
		Q_OBJECT

		std::atomic_bool ShouldStop_ { false };

		const QString Name_;

		QByteArray ToxState_;

		EntryStatus Status_;

		QList<std::function<void (Tox*)>> FQueue_;
		QMutex FQueueMutex_;

		std::shared_ptr<Tox> Tox_;
	public:
		ToxThread (const QString& name, const QByteArray& toxState);
		~ToxThread ();

		EntryStatus GetStatus () const;
		void SetStatus (const EntryStatus&);

		void Stop ();
		bool IsStoppable () const;

		QFuture<QByteArray> GetToxId ();

		enum class AddFriendResult
		{
			Added,
			InvalidId,
			TooLong,
			NoMessage,
			OwnKey,
			AlreadySent,
			BadChecksum,
			NoSpam,
			NoMem,
			Unknown
		};

		struct FriendInfo
		{
			QByteArray Pubkey_;
			QString Name_;
			EntryStatus Status_;
		};

		QFuture<AddFriendResult> AddFriend (QByteArray, QString);
		void AddFriend (QByteArray);

		QFuture<FriendInfo> ResolveFriend (qint32);
	private:
		void ScheduleFunction (const std::function<void (Tox*)>&);

		template<typename F>
		auto ScheduleFunction (const F& func) -> typename std::enable_if<!std::is_same<decltype (func ({})), void>::value, QFuture<decltype (func ({}))>>::type
		{
			QFutureInterface<decltype (func ({}))> iface;
			ScheduleFunction ([iface, func] (Tox *tox) mutable
					{
						iface.reportStarted ();
						try
						{
							const auto result = func (tox);
							iface.reportFinished (&result);
						}
						catch (const std::exception& e)
						{
							iface.reportException (ToxException { e });
							iface.reportFinished ();
						}
					});
			return iface.future ();
		}

		void SaveState ();

		void LoadFriends ();

		void HandleFriendRequest (const uint8_t*, const uint8_t*, uint16_t);
		void HandleNameChange (int32_t, const uint8_t*, uint16_t);
		void UpdateFriendStatus (int32_t);
	protected:
		virtual void run ();
	signals:
		void statusChanged (const EntryStatus&);

		void toxStateChanged (const QByteArray&);

		void gotFriend (qint32);
		void gotFriendRequest (const QByteArray& toxId, const QString& msg);

		void friendNameChanged (const QByteArray& toxId, const QString&);

		void friendStatusChanged (const QByteArray& pubkey, const EntryStatus& status);
	};
}
}
}
