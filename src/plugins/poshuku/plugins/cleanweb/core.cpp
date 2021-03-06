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

#include "core.h"
#include <algorithm>
#include <functional>
#include <thread>
#include <QNetworkRequest>
#include <QRegExp>
#include <QFile>
#include <QSettings>
#include <QFileInfo>
#include <QTimer>
#include <QTextCodec>
#include <QMessageBox>
#include <QDir>
#include <QCoreApplication>
#include <QtConcurrentRun>
#include <QtConcurrentMap>
#include <QMenu>
#include <QMainWindow>
#include <QDir>
#include <QElapsedTimer>
#include <util/xpc/util.h>
#include <util/sys/paths.h>
#include <util/sll/slotclosure.h>
#include <util/sll/prelude.h>
#include <util/sll/delayedexecutor.h>
#include <util/sll/qstringwrappers.h>
#include <util/sll/urlaccessor.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/core/ipluginsmanager.h>
#include <interfaces/poshuku/ibrowserwidget.h>
#include <interfaces/poshuku/iwebview.h>
#include <interfaces/poshuku/iinterceptablerequests.h>
#include <util/threads/futures.h>
#include "xmlsettingsmanager.h"
#include "userfiltersmodel.h"
#include "lineparser.h"
#include "subscriptionsmodel.h"

Q_DECLARE_METATYPE (QNetworkReply*);

namespace LeechCraft
{
namespace Poshuku
{
namespace CleanWeb
{
	namespace
	{
		QList<Filter> ParseToFilters (const QStringList& paths)
		{
			QList<Filter> result;
			for (const auto& filePath : paths)
			{
				QFile file (filePath);
				if (!file.open (QIODevice::ReadOnly))
				{
					qWarning () << Q_FUNC_INFO
						<< "could not open file"
						<< filePath
						<< file.errorString ();
					result << Filter ();
					continue;
				}

				const auto& data = QString::fromUtf8 (file.readAll ());
				auto rawLines = data.split ('\n', QString::SkipEmptyParts);
				if (!rawLines.isEmpty ())
					rawLines.removeAt (0);
				const auto& lines = Util::Map (rawLines, Util::QStringTrimmed {});

				Filter f;
				std::for_each (lines.begin (), lines.end (), LineParser (&f));

				f.SD_.Filename_ = QFileInfo (filePath).fileName ();

				result << f;
			}
			return result;
		}
	}

	Core::Core (SubscriptionsModel *model, UserFiltersModel *ufm, const ICoreProxy_ptr& proxy)
	: UserFilters_ { ufm }
	, SubsModel_ { model }
	, Proxy_ { proxy }
	{
		connect (SubsModel_,
				SIGNAL (filtersListChanged ()),
				this,
				SLOT (regenFilterCaches ()));
		connect (UserFilters_,
				SIGNAL (filtersChanged ()),
				this,
				SLOT (regenFilterCaches ()));

		const auto& path = Util::CreateIfNotExists ("cleanweb");
		const auto& infos = path.entryInfoList (QDir::Files | QDir::Readable);
		const auto& paths = Util::Map (infos, &QFileInfo::absoluteFilePath);

		Util::Sequence (nullptr, QtConcurrent::run (ParseToFilters, paths)) >>
				[this] (const QList<Filter>& filters)
				{
					SubsModel_->SetInitialFilters (filters);

					QTimer::singleShot (0,
							this,
							SLOT (update ()));
				};
	}

	ICoreProxy_ptr Core::GetProxy () const
	{
		return Proxy_;
	}

	bool Core::CouldHandle (const Entity& e) const
	{
		const auto& url = e.Entity_.toUrl ();
		return url.scheme () == "abp" && url.path () == "subscribe";
	}

	void Core::Handle (Entity subscr)
	{
		QUrl subscrUrl = subscr.Entity_.toUrl ();

		Add (subscrUrl);
	}

	void Core::HandleBrowserWidget (IBrowserWidget *ibw)
	{
		const auto view = ibw->GetWebView ();
		new Util::SlotClosure<Util::NoDeletePolicy>
		{
			[view, this] { HandleViewLayout (view); },
			view->GetQWidget (),
			{ SIGNAL (earliestViewLayout ()), SIGNAL (loadFinished (bool)) },
			view->GetQWidget ()
		};
	}

	void Core::HandleContextMenu (const ContextMenuInfo& r,
			IWebView *view, QMenu *menu,
			WebViewCtxMenuStage stage)
	{
		const auto& iurl = r.ImageUrl_;
		if (stage == WVSAfterImage &&
				!iurl.isEmpty ())
		{
			const auto action = menu->addAction (tr ("Block image..."));
			new Util::SlotClosure<Util::DeleteLaterPolicy>
			{
				[=] { UserFilters_->BlockImage (iurl, view); },
				action,
				SIGNAL (triggered ()),
				action
			};
		}
	}

	namespace
	{
#if defined (Q_OS_WIN32) || defined (Q_OS_MAC)
		// Thanks for this goes to http://www.codeproject.com/KB/string/patmatch.aspx
		bool WildcardMatches (const char *pattern, const char *str)
		{
			enum State {
				Exact,        // exact match
				Any,        // ?
				AnyRepeat    // *
			};

			const char *s = str;
			const char *p = pattern;
			const char *q = 0;
			int state = 0;

			bool match = true;
			while (match && *p) {
				if (*p == '*') {
					state = AnyRepeat;
					q = p+1;
				} else if (*p == '?') state = Any;
				else state = Exact;

				if (*s == 0) break;

				switch (state) {
					case Exact:
						match = *s == *p;
						s++;
						p++;
						break;

					case Any:
						match = true;
						s++;
						p++;
						break;

					case AnyRepeat:
						match = true;
						s++;

						if (*s == *q) p++;
						break;
				}
			}

			if (state == AnyRepeat) return (*s == *q);
			else if (state == Any) return (*s == *p);
			else return match && (*s == *p);
		}
#else
#include <fnmatch.h>

		bool WildcardMatches (const char *pat, const char *str)
		{
			return !fnmatch (pat, str, 0);
		}
#endif

		bool Matches (const FilterItem_ptr& item,
				const QByteArray& urlUtf8, const QString& domain)
		{
			const auto& opt = item->Option_;
			if (opt.MatchObjects_ != FilterOption::MatchObject::All)
			{
				if (!(opt.MatchObjects_ & FilterOption::MatchObject::CSS) &&
						!(opt.MatchObjects_ & FilterOption::MatchObject::Image) &&
						!(opt.MatchObjects_ & FilterOption::MatchObject::Script) &&
						!(opt.MatchObjects_ & FilterOption::MatchObject::Object) &&
						!(opt.MatchObjects_ & FilterOption::MatchObject::ObjSubrequest))
					return false;
			}

			if (std::any_of (opt.NotDomains_.begin (), opt.NotDomains_.end (),
						[&domain, &opt] (const QString& notDomain)
							{ return domain.endsWith (notDomain, opt.Case_); }))
				return false;

			if (!opt.Domains_.isEmpty () &&
					std::none_of (opt.Domains_.begin (), opt.Domains_.end (),
							[&domain, &opt] (const QString& doDomain)
								{ return domain.endsWith (doDomain, opt.Case_); }))
				return false;

			switch (opt.MatchType_)
			{
			case FilterOption::MTRegexp:
				return item->RegExp_.Matches (urlUtf8);
			case FilterOption::MTWildcard:
				return WildcardMatches (item->PlainMatcher_.constData (), urlUtf8.constData ());
			case FilterOption::MTPlain:
				return urlUtf8.indexOf (item->PlainMatcher_) >= 0;
			case FilterOption::MTBegin:
				return urlUtf8.startsWith (item->PlainMatcher_);
			case FilterOption::MTEnd:
				return urlUtf8.endsWith (item->PlainMatcher_);
			}

			return false;
		}
	}

	namespace
	{
		void DumbReductor (bool& res, bool value)
		{
			if (value)
				res = true;
		}

		FilterOption::MatchObjects ResourceType2Objs (IInterceptableRequests::ResourceType type)
		{
			switch (type)
			{
			case IInterceptableRequests::ResourceType::Image:
				return FilterOption::MatchObject::Image;
			case IInterceptableRequests::ResourceType::SubFrame:
				return FilterOption::MatchObject::Subdocument;
			case IInterceptableRequests::ResourceType::Stylesheet:
				return FilterOption::MatchObject::CSS;
			default:
				return FilterOption::MatchObject::All;
			}
		}

		bool ShouldReject (const IInterceptableRequests::RequestInfo& req,
				const QList<QList<FilterItem_ptr>>& exceptions,
				const QList<QList<FilterItem_ptr>>& filters)
		{
			if (!XmlSettingsManager::Instance ()->property ("EnableFiltering").toBool ())
				return false;

			if (!req.PageUrl_.isValid ())
				return false;

			const auto objs = ResourceType2Objs (req.ResourceType_);

			const QUrl& url = req.RequestUrl_;
			const QString& urlStr = url.toString ();
			const auto& urlUtf8 = urlStr.toUtf8 ();
			const QString& cinUrlStr = urlStr.toLower ();
			const auto& cinUrlUtf8 = cinUrlStr.toUtf8 ();

			const QString& domain = req.PageUrl_.host ();
			const bool isThirdParty = !url.host ().endsWith (domain);

			auto matches = [=] (const QList<QList<FilterItem_ptr>>& chunks)
			{
				return QtConcurrent::blockingMappedReduced (chunks.begin (), chunks.end (),
						std::function<bool (const QList<FilterItem_ptr>&)>
						{
							[=] (const QList<FilterItem_ptr>& items)
							{
								for (const auto& item : items)
								{
									const auto& opt = item->Option_;
									if (opt.ThirdParty_ != FilterOption::ThirdParty::Unspecified)
										if ((opt.ThirdParty_ == FilterOption::ThirdParty::Yes) != isThirdParty)
											continue;

									if (opt.MatchObjects_ != FilterOption::MatchObject::All &&
											objs != FilterOption::MatchObject::All &&
											!(objs & opt.MatchObjects_))
										continue;

									const auto& utf8 = item->Option_.Case_ == Qt::CaseSensitive ? urlUtf8 : cinUrlUtf8;
									if (Matches (item, utf8, domain))
										return true;
								}

								return false;
							}
						}, DumbReductor);
			};
			if (matches (exceptions))
				return false;
			if (matches (filters))
				return true;

			return false;
		}
	}

	void Core::InstallInterceptor ()
	{
		auto interceptor = [this] (const IInterceptableRequests::RequestInfo& info)
				-> IInterceptableRequests::Result_t
		{
			if (!ShouldReject (info, ExceptionsCache_, FilterItemsCache_))
				return IInterceptableRequests::Allow {};

			if (info.View_)
			{
				const auto view = *info.View_;
				const auto reqUrl = info.RequestUrl_;
				auto exec = new Util::DelayedExecutor
				{
					[this, view, reqUrl] { DelayedRemoveElements (view, reqUrl); },
					0
				};
				exec->moveToThread (qApp->thread ());
			}

			return IInterceptableRequests::Block {};
		};

		const auto interceptables = Proxy_->GetPluginsManager ()->GetAllCastableTo<IInterceptableRequests*> ();
		for (const auto interceptable : interceptables)
			interceptable->AddInterceptor (interceptor);
	}

	void Core::HandleProvider (QObject *provider)
	{
		if (Downloaders_.contains (provider))
			return;

		Downloaders_ << provider;
		connect (provider,
				SIGNAL (jobFinished (int)),
				this,
				SLOT (handleJobFinished (int)));
		connect (provider,
				SIGNAL (jobError (int, IDownload::Error)),
				this,
				SLOT (handleJobError (int, IDownload::Error)));
	}

	void Core::Parse (const QString& filePath)
	{
		SubsModel_->AddFilter (ParseToFilters ({ filePath }).first ());
	}

	bool Core::Add (const QUrl& subscrUrl)
	{
		qDebug () << Q_FUNC_INFO << subscrUrl;
		QUrl url;

		const Util::UrlAccessor accessor { subscrUrl };
		const auto& location = accessor ["location"];

		if (location.contains ("%"))
			url.setUrl (QUrl::fromPercentEncoding (location.toLatin1 ()));
		else
			url.setUrl (location);

		const auto& subscrName = accessor ["title"];

		qDebug () << "adding" << url << "as" << subscrName;
		bool result = Load (url, subscrName);
		if (result)
		{
			QString str = tr ("The subscription %1 was successfully added.")
					.arg (subscrName);
			Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification ("Poshuku CleanWeb",
					str, PInfo_));
		}
		return result;
	}

	bool Core::Load (const QUrl& url, const QString& subscrName)
	{
		const auto& name = QFileInfo (url.path ()).fileName ();
		const auto& path = Util::CreateIfNotExists ("cleanweb").absoluteFilePath (name);

		const auto& e = Util::MakeEntity (url,
				path,
				Internal |
					DoNotNotifyUser |
					DoNotSaveInHistory |
					NotPersistent |
					DoNotAnnounceEntity);

		const auto iem = Proxy_->GetEntityManager ();
		const auto& result = iem->DelegateEntity (e);
		if (!result.Handler_)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to delegate"
					<< subscrName
					<< url.toString ();

			const auto& str = tr ("The subscription %1 wasn't delegated.")
					.arg (subscrName);
			iem->HandleEntity (Util::MakeNotification ("Poshuku CleanWeb",
					str, PCritical_));
			return false;
		}

		HandleProvider (result.Handler_);
		PendingJob pj
		{
			path,
			name,
			subscrName,
			url
		};
		PendingJobs_ [result.ID_] = pj;
		return true;
	}

	void Core::update ()
	{
		if (!XmlSettingsManager::Instance ()->
				property ("Autoupdate").toBool ())
			return;

		const auto& current = QDateTime::currentDateTime ();
		int days = XmlSettingsManager::Instance ()->
			property ("UpdateInterval").toInt ();
		for (const auto& f : SubsModel_->GetAllFilters ())
			if (f.SD_.LastDateTime_.daysTo (current) > days)
				Load (f.SD_.URL_, f.SD_.Name_);
	}

	void Core::handleJobFinished (int id)
	{
		if (!PendingJobs_.contains (id))
			return;

		PendingJob pj = PendingJobs_ [id];
		SubscriptionData sd
		{
			pj.URL_,
			pj.Subscr_,
			pj.FileName_,
			QDateTime::currentDateTime ()
		};
		Parse (pj.FullName_);
		PendingJobs_.remove (id);
		SubsModel_->SetSubData (sd);
	}

	void Core::handleJobError (int id, IDownload::Error)
	{
		if (!PendingJobs_.contains (id))
			return;

		PendingJobs_.remove (id);
	}

	void Core::HandleViewLayout (IWebView *view)
	{
		if (!XmlSettingsManager::Instance ()->property ("EnableElementHiding").toBool ())
			return;

		if (ScheduledHidings_.contains (view))
			return;

		qDebug () << Q_FUNC_INFO << view;
		ScheduledHidings_ << view;

		const auto& frameUrl = view->GetUrl ();
		const QString& urlStr = frameUrl.toString ();
		const auto& urlUtf8 = urlStr.toUtf8 ();
		const QString& cinUrlStr = urlStr.toLower ();
		const auto& cinUrlUtf8 = cinUrlStr.toUtf8 ();

		const QString& domain = frameUrl.host ();

		auto allFilters = SubsModel_->GetAllFilters ();
		allFilters << UserFilters_->GetFilter ();

		const auto future = QtConcurrent::run ([=]
				{
					QElapsedTimer timer;
					timer.start ();

					QStringList sels;
					for (const Filter& filter : allFilters)
						for (const auto& item : filter.Filters_)
						{
							if (item->Option_.HideSelector_.isEmpty ())
								continue;

							const auto& opt = item->Option_;
							const auto& utf8 = opt.Case_ == Qt::CaseSensitive ? urlUtf8 : cinUrlUtf8;
							if (!Matches (item, utf8, domain))
								continue;

							sels << item->Option_.HideSelector_;
						}

					const auto msecs = timer.nsecsElapsed () / 1000000;
					const auto remaining = 1000 - msecs;
					if (remaining > 0)
					{
						qDebug () << Q_FUNC_INFO
								<< "sleeping for"
								<< remaining
								<< "ms";
						std::this_thread::sleep_for (std::chrono::milliseconds { remaining });
					}

					return HidingWorkerResult { view, sels };
				});

		Util::Sequence (view->GetQWidget (), future) >>
				[this] (const HidingWorkerResult& result) { HideElementsChunk (result); };
	}

	void Core::HideElementsChunk (HidingWorkerResult result)
	{
		ScheduledHidings_.remove (result.View_);

		for (auto& selector : result.Selectors_)
			selector.replace ('\\', "\\\\")
					.replace ('\'', "\\'")
					;

		QString js = R"(
					(function(){
					var elems = document.querySelectorAll('__SELECTORS__');
					for (var i = 0; i < elems.length; ++i)
						elems[i].remove();
					return elems.length;
					})();
				)";
		js.replace ("__SELECTORS__", result.Selectors_.join (", "));

		result.View_->EvaluateJS (js,
				[result, js] (const QVariant& res)
				{
					if (const auto count = res.toInt ())
						qDebug () << "removed"
								<< count
								<< "elements on frame with URL"
								<< result.View_->GetUrl ();
					else if (!res.canConvert<int> ())
						qWarning () << Q_FUNC_INFO
								<< "failed to execute JS:"
								<< js;
				},
				IWebView::EvaluateJSFlag::RecurseSubframes);
	}

	namespace
	{
		template<typename Cont>
		void RemoveElements (IWebView *view, const QList<QUrl>& urls, Cont&& cont)
		{
			const auto& preparedUrls = Util::Map (urls,
					[] (const QUrl& url) { return QString { '"' + url.toEncoded () + '"' }; });

			QString js = R"(
					(function(){
					var urls = [ __URLS__ ];
					var elems = document.querySelectorAll('img,script,iframe,applet,object');
					if (elems.length == 0)
						return false;
					var removedCount = 0;
					for (var i = 0; i < elems.length; ++i){
						if (urls.indexOf(elems[i].src) != -1){
							elems[i].remove();
							++removedCount;
						}
					}
					return removedCount == urls.length;
					})();
				)";

			js.replace ("__URLS__", preparedUrls.join (", "));

			view->EvaluateJS (js,
					[cont = std::move (cont)] (const QVariant& res) { cont (res.toBool ()); },
					IWebView::EvaluateJSFlag::RecurseSubframes);
		}
	}

	void Core::DelayedRemoveElements (IWebView *view, const QUrl& url)
	{
		RemoveElements (view, { url },
				[this, view, url] (bool res)
				{
					if (res)
						return;

					const auto obj = view->GetQWidget ();
					connect (obj,
							SIGNAL (loadFinished (bool)),
							this,
							SLOT (moreDelayedRemoveElements ()),
							Qt::UniqueConnection);
					connect (obj,
							SIGNAL (destroyed (QObject*)),
							this,
							SLOT (handleViewDestroyed (QObject*)),
							Qt::UniqueConnection);
					MoreDelayedURLs_ [obj] << url;
				});
	}

	void Core::moreDelayedRemoveElements ()
	{
		const auto senderObj = sender ();
		auto view = qobject_cast<IWebView*> (senderObj);
		if (!view)
			return;

		const auto& urls = MoreDelayedURLs_.take (senderObj);
		if (!urls.isEmpty ())
			RemoveElements (view, urls.toList (),
					[view, urls] (bool res)
					{
						if (!res)
							qWarning () << Q_FUNC_INFO
									<< urls
									<< "not found for"
									<< view->GetUrl ();
					});
	}

	void Core::handleViewDestroyed (QObject *obj)
	{
		MoreDelayedURLs_.remove (obj);
	}

	void Core::regenFilterCaches ()
	{
		ExceptionsCache_.clear ();
		FilterItemsCache_.clear ();

		auto allFilters = SubsModel_->GetAllFilters ();
		allFilters << UserFilters_->GetFilter ();

		int exceptionsCount = 0;
		int filtersCount = 0;
		for (const Filter& filter : allFilters)
		{
			exceptionsCount += filter.Exceptions_.size ();
			filtersCount += filter.Filters_.size ();
		}

		auto idealThreads = std::max (QThread::idealThreadCount (), 2);

		const int exChunkSize = std::max (exceptionsCount / idealThreads / 4, 200);
		const int fChunkSize = std::max (filtersCount / idealThreads / 4, 200);
		qDebug () << Q_FUNC_INFO << exceptionsCount << filtersCount << exChunkSize << fChunkSize;

		QList<FilterItem_ptr> lastItemsChunk, lastExceptionsChunk;
		lastItemsChunk.reserve (fChunkSize);
		lastExceptionsChunk.reserve (exChunkSize);

		for (const Filter& filter : allFilters)
		{
			for (const auto& item : filter.Exceptions_)
			{
				if (!item->Option_.HideSelector_.isEmpty ())
					continue;

				lastExceptionsChunk << item;
				if (lastExceptionsChunk.size () >= exChunkSize)
				{
					ExceptionsCache_ << lastExceptionsChunk;
					lastExceptionsChunk.clear ();
				}
			}

			for (const auto& item : filter.Filters_)
			{
				if (!item->Option_.HideSelector_.isEmpty ())
					continue;

				lastItemsChunk << item;
				if (lastItemsChunk.size () >= fChunkSize)
				{
					FilterItemsCache_ << lastItemsChunk;
					lastItemsChunk.clear ();
				}
			}
		}

		if (!lastItemsChunk.isEmpty ())
			FilterItemsCache_ << lastItemsChunk;
		if (!lastExceptionsChunk.isEmpty ())
			ExceptionsCache_ << lastExceptionsChunk;
	}
}
}
}
