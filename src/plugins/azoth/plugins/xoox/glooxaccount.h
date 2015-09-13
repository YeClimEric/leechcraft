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

#ifndef PLUGINS_AZOTH_PLUGINS_XOOX_GLOOXACCOUNT_H
#define PLUGINS_AZOTH_PLUGINS_XOOX_GLOOXACCOUNT_H
#include <memory>
#include <QObject>
#include <QMap>
#include <QIcon>
#include <QXmppRosterIq.h>
#include <QXmppBookmarkSet.h>
#include <interfaces/azoth/iaccount.h>
#include <interfaces/azoth/iextselfinfoaccount.h>
#include <interfaces/azoth/ihaveservicediscovery.h>
#include <interfaces/azoth/ihavesearch.h>
#include <interfaces/azoth/imessage.h>
#include <interfaces/azoth/ihaveconsole.h>
#include <interfaces/azoth/isupporttune.h>
#include <interfaces/azoth/isupportmood.h>
#include <interfaces/azoth/isupportactivity.h>
#include <interfaces/azoth/isupportgeolocation.h>
#include <interfaces/azoth/isupportmediacalls.h>
#include <interfaces/azoth/isupportriex.h>
#include <interfaces/azoth/isupportbookmarks.h>
#include <interfaces/azoth/ihavemicroblogs.h>
#include <interfaces/azoth/iregmanagedaccount.h>
#include <interfaces/azoth/ihaveserverhistory.h>
#include <interfaces/azoth/isupportlastactivity.h>
#include <interfaces/azoth/ihaveblacklists.h>
#ifdef ENABLE_CRYPT
#include <interfaces/azoth/isupportpgp.h>
#endif
#include "glooxclentry.h"
#include "glooxprotocol.h"

class QXmppCall;

namespace LeechCraft
{
namespace Azoth
{
class IProtocol;

namespace Xoox
{
	class ClientConnection;
	class AccountSettingsHolder;

	struct GlooxAccountState
	{
		State State_;
		QString Status_;
		int Priority_;
	};

	bool operator== (const GlooxAccountState&, const GlooxAccountState&);

	class GlooxProtocol;
	class TransferManager;
	class Xep0313ModelManager;

	class GlooxAccount : public QObject
					   , public IAccount
					   , public IExtSelfInfoAccount
					   , public IHaveServiceDiscovery
					   , public IHaveSearch
					   , public IHaveConsole
					   , public IHaveMicroblogs
					   , public ISupportTune
					   , public ISupportMood
					   , public ISupportActivity
					   , public ISupportGeolocation
#ifdef ENABLE_MEDIACALLS
					   , public ISupportMediaCalls
#endif
					   , public ISupportRIEX
					   , public ISupportBookmarks
					   , public ISupportLastActivity
					   , public IRegManagedAccount
					   , public IHaveServerHistory
					   , public IHaveBlacklists
#ifdef ENABLE_CRYPT
					   , public ISupportPGP
#endif
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Azoth::IAccount
				LeechCraft::Azoth::IExtSelfInfoAccount
				LeechCraft::Azoth::IHaveServiceDiscovery
				LeechCraft::Azoth::IHaveSearch
				LeechCraft::Azoth::IHaveConsole
				LeechCraft::Azoth::IHaveMicroblogs
				LeechCraft::Azoth::ISupportTune
				LeechCraft::Azoth::ISupportMood
				LeechCraft::Azoth::ISupportActivity
				LeechCraft::Azoth::ISupportGeolocation
				LeechCraft::Azoth::ISupportRIEX
				LeechCraft::Azoth::ISupportBookmarks
				LeechCraft::Azoth::ISupportLastActivity
				LeechCraft::Azoth::IRegManagedAccount
				LeechCraft::Azoth::IHaveServerHistory
				LeechCraft::Azoth::IHaveBlacklists
			)

#ifdef ENABLE_MEDIACALLS
		Q_INTERFACES (LeechCraft::Azoth::ISupportMediaCalls)
#endif

#ifdef ENABLE_CRYPT
		Q_INTERFACES (LeechCraft::Azoth::ISupportPGP)
#endif

		QString Name_;
		GlooxProtocol *ParentProtocol_;

		AccountSettingsHolder *SettingsHolder_;

		QIcon AccountIcon_;

		std::shared_ptr<ClientConnection> ClientConnection_;
		std::shared_ptr<TransferManager> TransferManager_;

		QHash<QObject*, QPair<QString, QString>> ExistingEntry2JoinConflict_;

		QAction *SelfVCardAction_;
		QAction *PrivacyDialogAction_;
		QAction *CarbonsAction_;

		Xep0313ModelManager * const Xep0313ModelMgr_;
	public:
		GlooxAccount (const QString&, GlooxProtocol*, QObject*);

		void Init ();
		void Release ();

		AccountSettingsHolder* GetSettings () const;

		// IAccount
		QObject* GetQObject ();
		GlooxProtocol* GetParentProtocol () const;
		AccountFeatures GetAccountFeatures () const;
		QList<QObject*> GetCLEntries ();
		QString GetAccountName () const;
		QString GetOurNick () const;
		void RenameAccount (const QString&);
		QByteArray GetAccountID () const;
		QList<QAction*> GetActions () const;
		void OpenConfigurationDialog ();
		EntryStatus GetState () const;
		void ChangeState (const EntryStatus&);
		void Authorize (QObject*);
		void DenyAuth (QObject*);
		void AddEntry (const QString&,
				const QString&, const QStringList&);
		void RequestAuth (const QString&, const QString&,
				const QString&, const QStringList&);
		void RemoveEntry (QObject*);
		QObject* GetTransferManager () const;

		// IExtSelfInfoAccount
		QObject* GetSelfContact () const;
		QIcon GetAccountIcon () const;

		// IHaveServiceDiscovery
		QObject* CreateSDSession ();
		QString GetDefaultQuery () const;

		// IHaveSearch
		QObject* CreateSearchSession ();
		QString GetDefaultSearchServer () const;

		// IHaveConsole
		PacketFormat GetPacketFormat () const;
		void SetConsoleEnabled (bool);

		// IHaveMicroblogs
		void SubmitPost (const Post&);

		// ISupportTune, ISupportMood, ISupportActivity
		void PublishTune (const QMap<QString, QVariant>&);
		void SetMood (const QString&, const QString&);
		void SetActivity (const QString&, const QString&, const QString&);

		// ISupportGeolocation
		void SetGeolocationInfo (const GeolocationInfo_t&);
		GeolocationInfo_t GetUserGeolocationInfo (QObject*, const QString&) const;

#ifdef ENABLE_MEDIACALLS
		// ISupportMediaCalls
		MediaCallFeatures GetMediaCallFeatures () const;
		QObject* Call (const QString& id, const QString& variant);
#endif

		// ISupportRIEX
		void SuggestItems (QList<RIEXItem>, QObject*, QString);

		// ISupportBookmarks
		QWidget* GetMUCBookmarkEditorWidget ();
		QVariantList GetBookmarkedMUCs () const;
		void SetBookmarkedMUCs (const QVariantList&);

		// ISupportLastActivity
		QObject* RequestLastActivity (QObject*, const QString&);
		QObject* RequestLastActivity (const QString&);

		// IRegManagedAccount
		bool SupportsFeature (Feature) const;
		void UpdateServerPassword (const QString& newPass);
		void DeregisterAccount ();

		// IHaveServerHistory
		bool HasFeature (ServerHistoryFeature) const;
		void OpenServerHistoryConfiguration ();
		QAbstractItemModel* GetServerContactsModel () const;
		void FetchServerHistory (const QModelIndex&, const QByteArray&, int);
		DefaultSortParams GetSortParams () const;

		// IHaveBlacklists
		bool SupportsBlacklists () const;
		void SuggestToBlacklist (const QList<ICLEntry*>&);

#ifdef ENABLE_CRYPT
		// ISupportPGP
		void SetPrivateKey (const QCA::PGPKey&);
		QCA::PGPKey GetPrivateKey () const;
		void SetEntryKey (QObject*, const QCA::PGPKey&);
		QCA::PGPKey GetEntryKey (QObject* entry) const;
		void SetEncryptionEnabled (QObject*, bool);
#endif

		QString GetNick () const;
		void JoinRoom (const QString& jid, const QString& nick, const QString& password);
		void JoinRoom (const QString& room, const QString& server,
				const QString& nick, const QString& password);

		std::shared_ptr<ClientConnection> GetClientConnection () const;
		GlooxCLEntry* CreateFromODS (OfflineDataSource_ptr);
		QXmppBookmarkSet GetBookmarks () const;
		void SetBookmarks (const QXmppBookmarkSet&);

		void UpdateOurPhotoHash (const QByteArray&);

		void CreateSDForResource (const QString&);

		void RequestRosterSave ();

		QByteArray Serialize () const;
		static GlooxAccount* Deserialize (const QByteArray&, GlooxProtocol*);

		GlooxMessage* CreateMessage (IMessage::Type,
				const QString&, const QString&,
				const QString&);
	private:
		QString GetPassword (bool authFailure = false);
		QString GetDefaultReqHost () const;
	public slots:
		void handleEntryRemoved (QObject*);
		void handleGotRosterItems (const QList<QObject*>&);
	private slots:
		void regenAccountIcon (const QString&);
		void handleServerAuthFailed ();
		void feedClientPassword ();
		void showSelfVCard ();
		void showPrivacyDialog ();
		void handleCarbonsToggled (bool);
		void handleServerHistoryFetched (const QString&,
				const QString&, SrvHistMessages_t);
#ifdef ENABLE_MEDIACALLS
		void handleIncomingCall (QXmppCall*);
#endif
	signals:
		void gotCLItems (const QList<QObject*>&);
		void removedCLItems (const QList<QObject*>&);
		void accountRenamed (const QString&);
		void authorizationRequested (QObject*, const QString&);
		void itemSubscribed (QObject*, const QString&);
		void itemUnsubscribed (QObject*, const QString&);
		void itemUnsubscribed (const QString&, const QString&);
		void itemCancelledSubscription (QObject*, const QString&);
		void itemGrantedSubscription (QObject*, const QString&);
		void statusChanged (const EntryStatus&);
		void mucInvitationReceived (const QVariantMap&,
				const QString&, const QString&);

		void gotSDSession (QObject*);

		void bookmarksChanged ();

		void riexItemsSuggested (QList<LeechCraft::Azoth::RIEXItem> items,
				QObject*, QString);

		void gotConsolePacket (const QByteArray&, IHaveConsole::PacketDirection, const QString&);

		void geolocationInfoChanged (const QString&, QObject*);

		void serverPasswordUpdated (const QString&);

		void serverHistoryFetched (const QModelIndex&,
				const QByteArray&, const SrvHistMessages_t&);

#ifdef ENABLE_MEDIACALLS
		void called (QObject*);
#endif

#ifdef ENABLE_CRYPT
		void signatureVerified (QObject*, bool);
		void encryptionStateChanged (QObject*, bool);
#endif

		void rosterSaveRequested ();

		void accountSettingsChanged ();
	};

	typedef std::shared_ptr<GlooxAccount> GlooxAccount_ptr;
}
}
}

#endif
