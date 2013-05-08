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

#include "tagslineedit.h"
#include <QtDebug>
#include <QTimer>
#include <QCompleter>
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QPushButton>
#include "tagscompletionmodel.h"
#include "tagscompleter.h"

using namespace LeechCraft::Util;

TagsLineEdit::TagsLineEdit (QWidget *parent)
: QLineEdit (parent)
, Completer_ (0)
, Separator_ ("; ")
{
}

void TagsLineEdit::AddSelector ()
{
	CategorySelector_.reset (new CategorySelector (parentWidget ()));
	CategorySelector_->SetSeparator (Separator_);
	CategorySelector_->hide ();

	QAbstractItemModel *model = Completer_->model ();

	if (model->metaObject ()->indexOfSignal (QMetaObject::normalizedSignature ("tagsUpdated (QStringList)")) >= 0)
		connect (model,
				SIGNAL (tagsUpdated (QStringList)),
				this,
				SLOT (handleTagsUpdated (QStringList)));

	QStringList initialTags;
	for (int i = 0; i < model->rowCount (); ++i)
		initialTags << model->data (model->index (i, 0)).toString ();
	handleTagsUpdated (initialTags);

	connect (CategorySelector_.get (),
			SIGNAL (tagsSelectionChanged (const QStringList&)),
			this,
			SLOT (handleSelectionChanged (const QStringList&)));

	connect (this,
			SIGNAL (textChanged (const QString&)),
			CategorySelector_.get (),
			SLOT (lineTextChanged (const QString&)));
}

QString TagsLineEdit::GetSeparator () const
{
	return Separator_;
}

void TagsLineEdit::SetSeparator (const QString& sep)
{
	Separator_ = sep;
	if (CategorySelector_)
		CategorySelector_->SetSeparator (sep);
}

void TagsLineEdit::insertTag (const QString& completion)
{
	if (Completer_->widget () != this)
		return;

	QString wtext = text ();
	if (completion.startsWith (wtext))
		wtext.clear ();
	int pos = wtext.lastIndexOf (Separator_);
	if (pos >= 0)
		wtext = wtext.left (pos).append (Separator_);
	else
		wtext.clear ();
	wtext.append (completion);
	wtext = wtext.simplified ();
	setText (wtext);

	emit tagsChosen ();
}

void TagsLineEdit::handleTagsUpdated (const QStringList& tags)
{
	CategorySelector_->setPossibleSelections (tags);
}

void TagsLineEdit::setTags (const QStringList& tags)
{
	setText (tags.join (Separator_));
	if (CategorySelector_.get ())
		CategorySelector_->SetSelections (tags);
}

void TagsLineEdit::handleSelectionChanged (const QStringList& tags)
{
	setText (tags.join (Separator_));

	emit tagsChosen ();
}

void TagsLineEdit::keyPressEvent (QKeyEvent *e)
{
	if (Completer_ && Completer_->popup ()->isVisible ())
		switch (e->key ())
		{
			case Qt::Key_Enter:
			case Qt::Key_Return:
			case Qt::Key_Escape:
			case Qt::Key_Tab:
			case Qt::Key_Backtab:
				e->ignore ();
				return;
			default:
				break;
		}

	QLineEdit::keyPressEvent (e);

	bool cos = e->modifiers () & (Qt::ControlModifier |
			Qt::ShiftModifier |
			Qt::AltModifier |
			Qt::MetaModifier);
	bool isShortcut = e->modifiers () & (Qt::ControlModifier |
			Qt::AltModifier |
			Qt::ShiftModifier);
	if (!Completer_ ||
			(cos && e->text ().isEmpty ()) ||
			isShortcut)
		return;

	QString completionPrefix = textUnderCursor ();
	Completer_->setCompletionPrefix (completionPrefix);
	Completer_->popup ()->
		setCurrentIndex (Completer_->completionModel ()->index (0, 0));
	Completer_->complete ();
}

void TagsLineEdit::focusInEvent (QFocusEvent *e)
{
	if (Completer_)
		Completer_->setWidget (this);
	QLineEdit::focusInEvent (e);
}

void TagsLineEdit::contextMenuEvent (QContextMenuEvent *e)
{
	if (!CategorySelector_.get ())
	{
		QLineEdit::contextMenuEvent (e);
		return;
	}

	CategorySelector_->move (e->globalPos ());
	CategorySelector_->show ();
}

void TagsLineEdit::SetCompleter (TagsCompleter *c)
{
	if (Completer_)
		disconnect (Completer_,
				0,
				this,
				0);

	Completer_ = c;

	if (!Completer_)
		return;

	Completer_->setWidget (this);
	Completer_->setCompletionMode (QCompleter::PopupCompletion);
	connect (Completer_,
			SIGNAL (activated (const QString&)),
			this,
			SLOT (insertTag (const QString&)));
}

QString TagsLineEdit::textUnderCursor () const
{
	auto rxStr = Separator_;
	rxStr.replace (' ', "\\s*");

	QRegExp rx (rxStr);

	QString wtext = text ();
	int pos = cursorPosition () - 1;
	int last = wtext.indexOf (rx, pos);
	int first = wtext.lastIndexOf (rx, pos);
	if (first == -1)
		first = 0;
	if (last == -1)
		last = wtext.size ();
	return wtext.mid (first, last - first);
}

