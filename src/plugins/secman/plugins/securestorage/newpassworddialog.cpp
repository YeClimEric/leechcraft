/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2011-2012  Alexander Konovalov
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

#include "newpassworddialog.h"
#include <QString>
#include "securestorage.h"

namespace LeechCraft
{
namespace Plugins
{
namespace SecMan
{
namespace StoragePlugins
{
namespace SecureStorage
{
	NewPasswordDialog::NewPasswordDialog ()
	{
		Ui_.setupUi (this);

		connect (Ui_.PasswordEdit1_,
			SIGNAL (textChanged (const QString&)),
			this,
			SLOT (checkPasswords ()));
		connect (Ui_.PasswordEdit2_,
			SIGNAL (textChanged (const QString&)),
			this,
			SLOT (checkPasswords ()));

		connect (this,
			SIGNAL (accepted ()),
			this,
			SIGNAL (dialogFinished ()),
			Qt::QueuedConnection);
		connect (this,
			SIGNAL (rejected ()),
			this,
			SIGNAL (dialogFinished ()),
			Qt::QueuedConnection);
	}

	QString NewPasswordDialog::GetPassword () const
	{
		return ReturnIfEqual (Ui_.PasswordEdit1_->text (),  Ui_.PasswordEdit2_->text ());
	}

	void NewPasswordDialog::clear ()
	{
		Ui_.PasswordEdit1_->clear ();
		Ui_.PasswordEdit2_->clear ();
	}

	void NewPasswordDialog::checkPasswords ()
	{
		bool passwordsEqual = Ui_.PasswordEdit1_->text () == Ui_.PasswordEdit2_->text ();
		Ui_.DifferenceLabel_->setText (passwordsEqual ? "" : tr ("Passwords must be the same"));
		Ui_.ButtonBox_->button (QDialogButtonBox::Ok)->setEnabled (passwordsEqual);
	}

}
}
}
}
}
