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

#include "itemhandlerpath.h"
#include <QDir>
#include <QLabel>
#include <QGridLayout>
#include <QtDebug>
#include <QDesktopServices>
#include "../filepicker.h"

namespace LeechCraft
{
	ItemHandlerPath::ItemHandlerPath ()
	{
	}

	ItemHandlerPath::~ItemHandlerPath ()
	{
	}

	bool ItemHandlerPath::CanHandle (const QDomElement& element) const
	{
		return element.attribute ("type") == "path";
	}

	void ItemHandlerPath::Handle (const QDomElement& item,
			QWidget *pwidget)
	{
		QGridLayout *lay = qobject_cast<QGridLayout*> (pwidget->layout ());
		QLabel *label = new QLabel (XSD_->GetLabel (item));
		label->setWordWrap (false);

		FilePicker::Type type = FilePicker::Type::ExistingDirectory;
		if (item.attribute ("pickerType") == "openFileName")
			type = FilePicker::Type::OpenFileName;
		else if (item.attribute ("pickerType") == "saveFileName")
			type = FilePicker::Type::SaveFileName;

		FilePicker *picker = new FilePicker (type, XSD_);
		const QVariant& value = XSD_->GetValue (item);
		picker->SetText (value.toString ());
		picker->setObjectName (item.attribute ("property"));
		if (item.attribute ("onCancel") == "clear")
			picker->SetClearOnCancel (true);
		if (item.hasAttribute ("filter"))
			picker->SetFilter (item.attribute ("filter"));

		connect (picker,
				SIGNAL (textChanged (const QString&)),
				this,
				SLOT (updatePreferences ()));

		picker->setProperty ("ItemHandler", QVariant::fromValue<QObject*> (this));
		picker->setProperty ("SearchTerms", label->text ());

		int row = lay->rowCount ();
		lay->addWidget (label, row, 0);
		lay->addWidget (picker, row, 1);
	}

	QVariant ItemHandlerPath::GetValue (const QDomElement& item,
			QVariant value) const
	{
		if (value.isNull () ||
				value.toString ().isEmpty ())
		{
			if (item.hasAttribute ("defaultHomePath") &&
					item.attribute ("defaultHomePath") == "true")
				value = QDir::homePath ();
			else if (item.hasAttribute ("default"))
			{
				QString text = item.attribute ("default");
				QMap<QString, QDesktopServices::StandardLocation> str2loc;
				str2loc ["DOCUMENTS"] = QDesktopServices::DocumentsLocation;
				str2loc ["DESKTOP"] = QDesktopServices::DocumentsLocation;
				str2loc ["MUSIC"] = QDesktopServices::DocumentsLocation;
				str2loc ["MOVIES"] = QDesktopServices::DocumentsLocation;
				Q_FOREACH (const QString& key, str2loc.keys ())
					if (text.startsWith ("{" + key + "}"))
					{
						text.replace (0, key.length () + 2, QDesktopServices::storageLocation (str2loc [key]));
						break;
					}

				value = text;
			}
		}
		return value;
	}

	void ItemHandlerPath::SetValue (QWidget *widget,
			const QVariant& value) const
	{
		FilePicker *picker = qobject_cast<FilePicker*> (widget);
		if (!picker)
		{
			qWarning () << Q_FUNC_INFO
				<< "not a FilePicker"
				<< widget;
			return;
		}
		picker->SetText (value.toString ());
	}

	QVariant ItemHandlerPath::GetObjectValue (QObject *object) const
	{
		FilePicker *picker = qobject_cast<FilePicker*> (object);
		if (!picker)
		{
			qWarning () << Q_FUNC_INFO
				<< "not a FilePicker"
				<< object;
			return QVariant ();
		}
		return picker->GetText ();
	}
};
