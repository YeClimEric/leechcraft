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

#include "annitem.h"
#include <QBrush>
#include <QPen>
#include <QCursor>
#include <QtDebug>

namespace LeechCraft
{
namespace Monocle
{
	AnnBaseItem::AnnBaseItem (const IAnnotation_ptr& ann)
	: BaseAnn_ { ann }
	{
	}

	QGraphicsItem* AnnBaseItem::GetItem ()
	{
		return dynamic_cast<QGraphicsItem*> (this);
	}

	void AnnBaseItem::SetHandler (const Handler_f& handler)
	{
		Handler_ = handler;
	}

	AnnBaseItem* MakeItem (const IAnnotation_ptr& ann, QGraphicsItem *parent)
	{
		switch (ann->GetAnnotationType ())
		{
		case AnnotationType::Text:
			return new TextAnnItem (std::dynamic_pointer_cast<ITextAnnotation> (ann), parent);
		case AnnotationType::Highlight:
			return new HighAnnItem (std::dynamic_pointer_cast<IHighlightAnnotation> (ann), parent);
		case AnnotationType::Other:
			qWarning () << Q_FUNC_INFO
					<< "unknown annotation type with contents"
					<< ann->GetText ();
			return nullptr;
		}

		return nullptr;
	}

	TextAnnItem::TextAnnItem (const ITextAnnotation_ptr& ann, QGraphicsItem *parent)
	: AnnBaseGraphicsItem { ann, parent }
	{
	}

	void TextAnnItem::UpdateRect (const QRectF& rect)
	{
		setRect (rect);
	}

	HighAnnItem::HighAnnItem (const IHighlightAnnotation_ptr& ann, QGraphicsItem *parent)
	: AnnBaseGraphicsItem { ann, parent }
	, Ann_ { ann }
	, Polys_ { ToPolyData (Ann_->GetPolygons ()) }
	{
		for (const auto& data : Polys_)
		{
			addToGroup (data.Item_);
			data.Item_->setPen (Qt::NoPen);

			Bounding_ |= data.Poly_.boundingRect ();

			data.Item_->setCursor (Qt::PointingHandCursor);
		}
	}

	void HighAnnItem::UpdateRect (const QRectF& rect)
	{
		for (auto data : Polys_)
		{
			auto poly = data.Poly_;

			if (!Bounding_.width () || !Bounding_.height ())
				continue;

			const auto xScale = rect.width () / Bounding_.width ();
			const auto yScale = rect.height () / Bounding_.height ();
			const auto xTran = rect.x () - Bounding_.x () * xScale;
			const auto yTran = rect.y () - Bounding_.y () * yScale;

			data.Item_->setPolygon (poly * QMatrix { xScale, 0, 0, yScale, xTran, yTran });
		}
	}

	QList<HighAnnItem::PolyData> HighAnnItem::ToPolyData (const QList<QPolygonF>& polys)
	{
		QList<PolyData> result;
		for (const auto& poly : polys)
			result.append ({ poly, new QGraphicsPolygonItem });
		return result;
	}
}
}
