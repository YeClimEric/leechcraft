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

#pragma once

#include <QObject>
#include <QVariant>
#include <QStringList>
#include <QHash>
#include <QUrl>
#include <QDir>
#include <interfaces/iscriptloader.h>

namespace LeechCraft
{
namespace Aggregator
{
namespace BodyFetch
{
	class WorkerObject : public QObject
	{
		Q_OBJECT

		IScriptLoaderInstance *Inst_;
		QVariantList Items_;

		bool IsProcessing_;
		bool RecheckScheduled_;

		QStringList EnumeratedCache_;

		QHash<QString, QString> ChannelLink2ScriptID_;
		QHash<QUrl, IScript_ptr> URL2Script_;
		QHash<QUrl, quint64> URL2ItemID_;

		QHash<QString, IScript_ptr> CachedScripts_;

		QList<QPair<QUrl, QString>> FetchedQueue_;

		QDir StorageDir_;
	public:
		WorkerObject (QObject* = 0);

		void SetLoaderInstance (IScriptLoaderInstance*);
		bool IsOk () const;
		void AppendItems (const QVariantList&);
	private:
		void ProcessItems (const QVariantList&);
		IScript_ptr GetScriptForChannel (const QString&);
		QString FindScriptForChannel (const QString&);
		QString Parse (const QString&, IScript_ptr);
		void WriteFile (const QString&, quint64) const;
		QString Recode (const QByteArray&) const;
		void ScheduleRechecking ();
	private slots:
		void handleDownloadFinished (QUrl, QString);
		void recheckFinished ();
		void process ();
		void clearCaches ();
	signals:
		void downloadRequested (QUrl);
		void newBodyFetched (quint64);
	};
}
}
}
