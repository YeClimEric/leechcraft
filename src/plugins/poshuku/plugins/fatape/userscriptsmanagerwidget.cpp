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

#include "userscriptsmanagerwidget.h"
#include <QDebug>
#include <QStandardItemModel>
#include "fatape.h"

namespace LeechCraft
{
namespace Poshuku
{
namespace FatApe
{
	UserScriptsManagerWidget::UserScriptsManagerWidget (QStandardItemModel *model, Plugin *plugin)
	: Model_ (model)
	, Plugin_ (plugin)
	{
		Ui_.setupUi (this);
		Ui_.Items_->setModel (model);
		connect (Ui_.Items_->selectionModel (),
				SIGNAL (currentChanged (const QModelIndex&, const QModelIndex&)),
				this,
				SLOT (currentItemChanged (const QModelIndex&, const QModelIndex&)));
	}

	void UserScriptsManagerWidget::on_Edit__released ()
	{
		const QModelIndex& selected = Ui_.Items_->currentIndex ();

		if (selected.isValid ())
			Plugin_->EditScript (selected.row ());
	}

	void UserScriptsManagerWidget::on_Disable__released ()
	{
		const QModelIndex& selected = Ui_.Items_->currentIndex ();

		if (!selected.isValid ())
			return;

		QStandardItemModel *model = qobject_cast<QStandardItemModel*> (Ui_.Items_->model ());


		if (!model)
		{
			qWarning () << Q_FUNC_INFO
				<< "unable cast "
				<< Ui_.Items_->model ()
				<< "to QStandardItemModel";
			return;
		}

		bool enabled = selected.data (EnabledRole).toBool ();

		Plugin_->SetScriptEnabled (selected.row (), !enabled);

		for (int column = 0; column < model->columnCount (); column++)
			model->item (selected.row (), column)->setData (!enabled, EnabledRole);

		Ui_.Disable_->setText (!enabled ? tr ("Disable") : tr ("Enable"));
	}

	void UserScriptsManagerWidget::on_Remove__released ()
	{
		const QModelIndex& selected = Ui_.Items_->currentIndex ();

		if (selected.isValid ())
		{
			Ui_.Items_->model ()->removeRow (selected.row ());
			Plugin_->DeleteScript (selected.row ());
		}
	}

	void UserScriptsManagerWidget::currentItemChanged (const QModelIndex& current,
			const QModelIndex& previous)
	{
		bool currentEnabled = current.data (EnabledRole).toBool ();
		bool previousEnabled = previous.data (EnabledRole).toBool ();

		if ((previous.isValid () && currentEnabled != previousEnabled) || !previous.isValid ())
			Ui_.Disable_->setText (currentEnabled ? tr ("Disable") : tr ("Enable"));
	}
}
}
}

