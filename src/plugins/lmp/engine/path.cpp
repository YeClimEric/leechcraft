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

#include "path.h"
#include <QtDebug>
#include <gst/gst.h>
#include "sourceobject.h"
#include "output.h"

namespace LeechCraft
{
namespace LMP
{
	struct CallbackData
	{
		Path * const Path_;
		GstElement * const Elem_;
		guint ID_;

		const Path::Action Action_;
	};

	Path::Path (SourceObject *source, Output *output, QObject *parent)
	: QObject (parent)
	, SrcObj_ (source)
	, WholeBin_ (gst_bin_new ("whole_bin"))
	, Identity_ (gst_element_factory_make ("identity", "effect_placeholder"))
	, Pipeline_ (nullptr)
	, OutputBin_ (nullptr)
	{
		NextWholeElems_ << Identity_;

		source->AddToPath (this);
		output->AddToPathExposed (this);

		gst_bin_add_many (GST_BIN (WholeBin_), Identity_, OutputBin_, nullptr);
		gst_element_link (Identity_, OutputBin_);

		auto pad = gst_element_get_static_pad (Identity_, "sink");
		auto ghostPad = gst_ghost_pad_new ("sink", pad);
		gst_pad_set_active (ghostPad, TRUE);
		gst_element_add_pad (WholeBin_, ghostPad);
		gst_object_unref (pad);

		source->SetSink (WholeBin_);
		output->PostAddExposed (this);
	}

	Path::~Path ()
	{
	}

	GstElement* Path::GetPipeline () const
	{
		return Pipeline_;
	}

	void Path::SetPipeline (GstElement *pipeline)
	{
		Pipeline_ = pipeline;
	}

	GstElement* Path::GetOutPlaceholder () const
	{
		return Identity_;
	}

	GstElement* Path::GetWholeOut () const
	{
		return WholeBin_;
	}

	GstElement* Path::GetOutputBin () const
	{
		return OutputBin_;
	}

	void Path::SetOutputBin (GstElement *bin)
	{
		if (OutputBin_)
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot change output bin now";
		}

		OutputBin_ = bin;
		NextWholeElems_ << OutputBin_;
	}

	SourceObject* Path::GetSourceObject () const
	{
		return SrcObj_;
	}

	namespace
	{
#if GST_VERSION_MAJOR < 1
		gboolean EventProbeHandler (GstPad *pad, GstEvent *event, CallbackData *cbData)
		{
			if (GST_EVENT_TYPE (event) != GST_EVENT_EOS)
				return TRUE;
#else
		GstPadProbeReturn EventProbeHandler (GstPad *pad, GstPadProbeInfo *info, gpointer cbDataPtr)
		{
			if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS)
				return GST_PAD_PROBE_OK;

			const auto cbData = static_cast<CallbackData*> (cbDataPtr);
#endif
			qDebug () << Q_FUNC_INFO << "eos";
#if GST_VERSION_MAJOR < 1
			gst_pad_remove_event_probe (pad, cbData->ID_);
#else
			gst_pad_remove_probe (pad, cbData->ID_);
#endif

			cbData->Path_->FinalizeAction (cbData);

#if GST_VERSION_MAJOR < 1
			return FALSE;
#else
			return GST_PAD_PROBE_DROP;
#endif
		}

#if GST_VERSION_MAJOR < 1
		gboolean ProbeHandler (GstPad *pad, GstMiniObject*, CallbackData *cbData)
		{
#else
		GstPadProbeReturn ProbeHandler (GstPad *pad, GstPadProbeInfo*, gpointer cbDataPtr)
		{
			const auto cbData = static_cast<CallbackData*> (cbDataPtr);
#endif
			qDebug () << Q_FUNC_INFO;
#if GST_VERSION_MAJOR < 1
			gst_pad_remove_data_probe (pad, cbData->ID_);
#else
			gst_pad_remove_probe (pad, cbData->ID_);
#endif

#if GST_VERSION_MAJOR < 1
			cbData->ID_ = gst_pad_add_event_probe (pad, G_CALLBACK (EventProbeHandler), cbData);
#else
			cbData->ID_ = gst_pad_add_probe (pad,
					static_cast<GstPadProbeType> (GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM),
					EventProbeHandler, cbData, nullptr);
#endif

			const auto sinkpad = gst_element_get_static_pad (cbData->Path_->GetOutPlaceholder (), "sink");
			gst_pad_send_event (sinkpad, gst_event_new_eos ());
			gst_object_unref (sinkpad);

#if GST_VERSION_MAJOR < 1
			return TRUE;
#else
			return GST_PAD_PROBE_OK;
#endif
		}
	}

	void Path::InsertElement (GstElement *elem)
	{
		Perform (elem, Action::Add);
	}

	void Path::RemoveElement (GstElement *elem)
	{
		Perform (elem, Action::Remove);
	}

	void Path::FinalizeAction (CallbackData *cbData)
	{
		const auto elem = cbData->Elem_;

		auto deleteGuard = std::shared_ptr<CallbackData> (cbData);

		switch (cbData->Action_)
		{
		case Path::Action::Add:
		{
			gst_bin_add (GST_BIN (GetWholeOut ()), elem);

			qDebug () << "unlinking...";
			gst_element_unlink (NextWholeElems_.at (0), NextWholeElems_.at (1));

			qDebug () << "linking...";
			gst_element_link_many (NextWholeElems_.at (0), elem, NextWholeElems_.at (1), nullptr);

			GstState wholeCurrent, pending;
			gst_element_get_state (GetWholeOut (), &wholeCurrent, &pending, GST_SECOND);
			gst_element_set_state (elem, std::max (wholeCurrent, pending));

			NextWholeElems_.insert (1, elem);

			break;
		}
		case Path::Action::Remove:
		{
			const auto idx = NextWholeElems_.indexOf (elem);
			if (idx == -1)
			{
				qWarning () << Q_FUNC_INFO
						<< "element not found";
				return;
			}
			if (!idx || idx == NextWholeElems_.size () - 1)
			{
				qWarning () << Q_FUNC_INFO
						<< "cannot remove side element";
				return;
			}

			const auto prev = NextWholeElems_.at (idx - 1);
			const auto next = NextWholeElems_.at (idx + 1);

			gst_element_unlink_many (prev, elem, next, nullptr);
			gst_element_link (prev, next);

			gst_element_set_state (elem, GST_STATE_NULL);

			gst_bin_remove (GST_BIN (GetWholeOut ()), elem);

			NextWholeElems_.removeAt (idx);
			break;
		}
		}
	}

	void Path::Perform (GstElement *elem, Action action)
	{
		auto srcpad = gst_element_get_static_pad (GetOutPlaceholder (), "src");
		qDebug () << Q_FUNC_INFO << elem << srcpad;

		auto data = new CallbackData { this, elem, 0, action };
#if GST_VERSION_MAJOR < 1
		data->ID_ = gst_pad_add_data_probe (srcpad, G_CALLBACK (ProbeHandler), data);
#else
		data->ID_ = gst_pad_add_probe (srcpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
				ProbeHandler, data, nullptr);
#endif
	}
}
}
