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

#include "pubsubmanager.h"
#include <QDomElement>
#include <QtDebug>
#include <QXmppClient.h>

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	const QString NsPubSub = "http://jabber.org/protocol/pubsub";
	const QString NsPubSubEvent = "http://jabber.org/protocol/pubsub#event";

	void PubSubManager::RegisterCreator (const QString& node,
			boost::function<PEPEventBase* ()> creator)
	{
		Node2Creator_ [node] = creator;
		SetAutosubscribe (node, false);
	}

	void PubSubManager::SetAutosubscribe (const QString& node, bool enabled)
	{
		AutosubscribeNodes_ [node] = enabled;
	}

	void PubSubManager::PublishEvent (PEPEventBase *event)
	{
		QXmppElement publish;
		publish.setTagName ("publish");
		publish.setAttribute ("node", event->Node ());
		publish.appendChild (event->ToXML ());

		QXmppElement pubsub;
		pubsub.setTagName ("pubsub");
		pubsub.setAttribute ("xmlns", NsPubSub);
		pubsub.appendChild (publish);

		QXmppIq iq (QXmppIq::Set);
		iq.setExtensions (QXmppElementList () << pubsub);
		client ()->sendPacket (iq);
	}

	void PubSubManager::RequestItem (const QString& jid,
			const QString& node, const QString& id)
	{
		QXmppElement item;
		item.setTagName ("item");
		item.setAttribute ("id", id);

		QXmppElement items;
		items.setTagName ("items");
		items.setAttribute ("node", node);
		items.appendChild (item);

		QXmppElement pubsub;
		pubsub.setTagName ("pubsub");
		pubsub.setAttribute ("xmlns", NsPubSub);
		pubsub.appendChild (items);

		QXmppIq iq (QXmppIq::Get);
		iq.setTo (jid);
		iq.setExtensions (QXmppElementList () << pubsub);
		client ()->sendPacket (iq);
	}

	QStringList PubSubManager::discoveryFeatures () const
	{
		QStringList result;
		result << NsPubSub;
		Q_FOREACH (const QString& node, Node2Creator_.keys ())
		{
			result << node;
			if (AutosubscribeNodes_ [node])
				result << node + "+notify";
		}
		return result;
	}

	bool PubSubManager::handleStanza (const QDomElement& elem)
	{
		if (elem.tagName () == "message")
			return HandleMessage (elem);
		else if (elem.tagName () == "iq")
			return HandleIq (elem);
		else
			return false;
	}

	bool PubSubManager::HandleIq (const QDomElement& elem)
	{
		const QDomElement& pubsub = elem.firstChildElement ("pubsub");
		if (pubsub.namespaceURI () != NsPubSub)
			return false;

		ParseItems (pubsub.firstChildElement ("items"), elem.attribute ("from"));

		return true;
	}

	bool PubSubManager::HandleMessage (const QDomElement& elem)
	{
		if (elem.tagName () != "message" || elem.attribute ("type") != "headline")
			return false;

		const QDomElement& event = elem.firstChildElement ("event");
		if (event.namespaceURI () != NsPubSubEvent)
			return false;

		ParseItems (event.firstChildElement ("items"), elem.attribute ("from"));

		return true;
	}

	void PubSubManager::ParseItems (QDomElement items, const QString& from)
	{
		while (!items.isNull ())
		{
			const QString& node = items.attribute ("node");
			if (!Node2Creator_.contains (node))
			{
				items = items.nextSiblingElement ("items");
				continue;
			}

			QDomElement item = items.firstChildElement ("item");
			while (!item.isNull ())
			{
				PEPEventBase *eventObj = Node2Creator_ [node] ();
				eventObj->Parse (item);

				emit gotEvent (from, eventObj);

				delete eventObj;

				item = item.nextSiblingElement ("item");
			}

			items = items.nextSiblingElement ("items");
		}
	}
}
}
}
