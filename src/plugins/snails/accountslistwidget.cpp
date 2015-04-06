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

#include "accountslistwidget.h"
#include "account.h"
#include "accountsmanager.h"

namespace LeechCraft
{
namespace Snails
{
	AccountsListWidget::AccountsListWidget (AccountsManager *accsMgr, QWidget *parent)
	: QWidget { parent }
	, AccsMgr_ { accsMgr }
	{
		Ui_.setupUi (this);

		Ui_.AccountsTree_->setModel (AccsMgr_->GetAccountsModel ());
	}

	void AccountsListWidget::on_AddButton__released ()
	{
		Account_ptr acc (new Account);

		acc->OpenConfigDialog ();

		if (acc->IsNull ())
			return;

		AccsMgr_->AddAccount (acc);
	}

	void AccountsListWidget::on_ModifyButton__released ()
	{
		const QModelIndex& current = Ui_.AccountsTree_->currentIndex ();
		if (!current.isValid ())
			return;

		Account_ptr acc = AccsMgr_->GetAccount (current);
		if (!acc)
			return;

		acc->OpenConfigDialog ();
	}

	void AccountsListWidget::on_RemoveButton__released ()
	{
	}
}
}
