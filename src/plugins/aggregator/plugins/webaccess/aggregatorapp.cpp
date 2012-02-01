/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2012  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "aggregatorapp.h"
#include <QObject>
#include <Wt/WText>
#include <Wt/WContainerWidget>
#include <Wt/WBoxLayout>
#include <Wt/WCheckBox>
#include <Wt/WTreeView>
#include <Wt/WStandardItemModel>
#include <Wt/WOverlayLoadingIndicator>
#include "readchannelsfilter.h"

namespace LeechCraft
{
namespace Aggregator
{
namespace WebAccess
{
	namespace
	{
		Wt::WString ToW (const QString& str)
		{
			return Wt::WString (str.toUtf8 ().constData (), Wt::CharEncoding::UTF8);
		}
	}

	AggregatorApp::AggregatorApp (const Wt::WEnvironment& environment)
	: WApplication (environment)
	, ChannelsModel_ (new Wt::WStandardItemModel (this))
	, ChannelsFilter_ (new ReadChannelsFilter (this))
	, ItemsModel_ (new Wt::WStandardItemModel (this))
	{
		ChannelsFilter_->setSourceModel (ChannelsModel_);

		setTitle ("Aggregator WebAccess");
		setLoadingIndicator (new Wt::WOverlayLoadingIndicator ());

		SetupUI ();
	}

	void AggregatorApp::HandleChannelClicked (const Wt::WModelIndex& idx)
	{
		auto realIdx = ChannelsFilter_->mapToSource (idx);

		ItemsModel_->clear ();
		ItemView_->setText (Wt::WString ());
	}

	void AggregatorApp::SetupUI ()
	{
		auto rootLay = new Wt::WBoxLayout (Wt::WBoxLayout::LeftToRight);
		root ()->setLayout (rootLay);

		auto leftPaneLay = new Wt::WBoxLayout (Wt::WBoxLayout::TopToBottom);

		auto showReadChannels = new Wt::WCheckBox (ToW (QObject::tr ("Include read channels")));
		showReadChannels->setToolTip (ToW (QObject::tr ("Also display channels that have no unread items.")));
		showReadChannels->setChecked (false);
		showReadChannels->checked ().connect ([ChannelsFilter_] (Wt::NoClass) { ChannelsFilter_->SetHideRead (false); });
		showReadChannels->unChecked ().connect ([ChannelsFilter_] (Wt::NoClass) { ChannelsFilter_->SetHideRead (true); });
		leftPaneLay->addWidget (showReadChannels);

		auto channelsTree = new Wt::WTreeView ();
		channelsTree->setModel (ChannelsModel_);
		leftPaneLay->addWidget (channelsTree);

		channelsTree->clicked ().connect (this, &AggregatorApp::HandleChannelClicked);

		rootLay->addLayout (leftPaneLay);

		auto rightPaneLay = new Wt::WBoxLayout (Wt::WBoxLayout::TopToBottom);

		auto itemsTree = new Wt::WTreeView ();
		itemsTree->setModel (ItemsModel_);
		rightPaneLay->addWidget (channelsTree);

		ItemView_ = new Wt::WText ();
		rightPaneLay->addWidget (ItemView_);
	}
}
}
}
