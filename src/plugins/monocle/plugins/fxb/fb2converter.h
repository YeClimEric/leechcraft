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

#pragma once

#include <functional>
#include <QString>
#include <QObject>
#include <QHash>
#include <QStack>
#include <interfaces/monocle/idocument.h>
#include <interfaces/monocle/ihavetoc.h>

class QTextCharFormat;
class QTextBlockFormat;
class QTextCursor;
class QDomElement;
class QDomDocument;
class QTextDocument;

namespace LeechCraft
{
namespace Monocle
{
namespace FXB
{
	class Document;

	class CursorCacher;

	class FB2Converter : public QObject
	{
		Document *ParentDoc_;

		const QDomDocument& FB2_;

		QTextDocument *Result_;
		DocumentInfo DocInfo_;
		TOCEntryLevel_t TOC_;

		QStack<TOCEntry*> CurrentTOCStack_;

		QTextCursor *Cursor_;
		CursorCacher *CursorCacher_;

		int SectionLevel_ = 0;

		typedef std::function<void (QDomElement)> Handler_f;
		QHash<QString, Handler_f> Handlers_;

		QString Error_;
	public:
		FB2Converter (Document*, const QDomDocument&);
		~FB2Converter ();

		QString GetError () const;
		QTextDocument* GetResult () const;
		DocumentInfo GetDocumentInfo () const;
		TOCEntryLevel_t GetTOC () const;
	private:
		QDomElement FindBinary (const QString&) const;

		void HandleDescription (const QDomElement&);
		void HandleBody (const QDomElement&);

		void HandleSection (const QDomElement&);
		void HandleTitle (const QDomElement&, int = 0);
		void HandleEpigraph (const QDomElement&);
		void HandleImage (const QDomElement&);

		void HandlePara (const QDomElement&);
		void HandleParaWONL (const QDomElement&);
		void HandlePoem (const QDomElement&);
		void HandleStanza (const QDomElement&);
		void HandleEmptyLine (const QDomElement&);

		void HandleChildren (const QDomElement&);
		void Handle (const QDomElement&);

		void HandleMangleBlockFormat (const QDomElement&,
				std::function<void (QTextBlockFormat&)>, Handler_f);
		void HandleMangleCharFormat (const QDomElement&,
				std::function<void (QTextCharFormat&)>, Handler_f);

		void FillPreamble ();
	};
}
}
}
