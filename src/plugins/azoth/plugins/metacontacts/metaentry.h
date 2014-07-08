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

#ifndef PLUGINS_AZOTH_PLUGINS_METACONTACTS_METAENTRY_H
#define PLUGINS_AZOTH_PLUGINS_METACONTACTS_METAENTRY_H
#include <boost/function.hpp>
#include <QObject>
#include <QPair>
#include <QStringList>
#include <interfaces/azoth/iclentry.h>
#include <interfaces/azoth/iadvancedclentry.h>

namespace LeechCraft
{
namespace Azoth
{
namespace Metacontacts
{
	class MetaAccount;

	class MetaEntry : public QObject
					, public ICLEntry
					, public IAdvancedCLEntry
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Azoth::ICLEntry
				LeechCraft::Azoth::IAdvancedCLEntry)

		MetaAccount *Account_;
		QString ID_;
		QString Name_;
		QStringList Groups_;

		QStringList UnavailableRealEntries_;
		QList<QObject*> AvailableRealEntries_;
		QMap<QString, QPair<QObject*, QString>> Variant2RealVariant_;

		QList<QObject*> Messages_;

		QAction *ActionMCSep_;
		QAction *ActionManageContacts_;
	public:
		MetaEntry (const QString&, MetaAccount*);

		QObjectList GetAvailEntryObjs () const;
		QStringList GetRealEntries () const;
		void SetRealEntries (const QStringList&);
		void AddRealObject (ICLEntry*);

		QString GetMetaVariant (QObject*, const QString&) const;

		// ICLEntry
		QObject* GetQObject ();
		QObject* GetParentAccount () const;
		Features GetEntryFeatures () const;
		EntryType GetEntryType () const;
		QString GetEntryName () const;
		void SetEntryName (const QString& name);
		QString GetEntryID () const;
		QString GetHumanReadableID () const;
		QStringList Groups () const;
		void SetGroups (const QStringList&);
		QStringList Variants () const;
		QObject* CreateMessage (IMessage::MessageType, const QString&, const QString&);
		QList<QObject*> GetAllMessages () const;
		void PurgeMessages (const QDateTime&);
		void SetChatPartState (ChatPartState, const QString&);
		EntryStatus GetStatus (const QString&) const;
		QImage GetAvatar () const;
		QString GetRawInfo () const;
		void ShowInfo ();
		QList<QAction*> GetActions () const;
		QMap<QString, QVariant> GetClientInfo (const QString&) const;
		void MarkMsgsRead ();
		void ChatTabClosed ();

		// IAdvancedCLEntry
		AdvancedFeatures GetAdvancedFeatures () const;
		void DrawAttention (const QString&, const QString&);
	private:
		template<typename T, typename U>
		T ActWithVariant (boost::function<T (U, const QString&)>, const QString&) const;

		void ConnectStandardSignals (QObject*);
		void ConnectAdvancedSiganls (QObject*);
	private:
		void PerformRemoval (QObject*);
		void SetNewEntryList (const QList<QObject*>&, bool readdRemoved);
	private slots:
		void handleRealGotMessage (QObject*);
		void handleRealStatusChanged (const EntryStatus&, const QString&);
		void handleRealVariantsChanged (QStringList, QObject* = 0);
		void handleRealNameChanged (const QString&);
		void handleRealCPSChanged (const ChatPartState&, const QString&);

		void handleRealAttentionDrawn (const QString&, const QString&);
		void handleRealMoodChanged (const QString&);
		void handleRealActivityChanged (const QString&);
		void handleRealTuneChanged (const QString&);
		void handleRealLocationChanged (const QString&);

		void checkRemovedCLItems (const QList<QObject*>&);

		void handleManageContacts ();
	signals:
		// ICLEntry
		void gotMessage (QObject*);
		void statusChanged (const EntryStatus&, const QString&);
		void availableVariantsChanged (const QStringList&);
		void nameChanged (const QString&);
		void groupsChanged (const QStringList&);
		void avatarChanged (const QImage&);
		void chatPartStateChanged (const ChatPartState&, const QString&);
		void permsChanged ();
		void entryGenerallyChanged ();

		// IAdvancedCLEntry
		void attentionDrawn (const QString&, const QString&);
		void moodChanged (const QString&);
		void activityChanged (const QString&);
		void tuneChanged (const QString&);
		void locationChanged (const QString&);

		// Own
		void shouldRemoveThis ();
	};
}
}
}

#endif
