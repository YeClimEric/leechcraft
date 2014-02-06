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

#include "cpuloadproxyobj.h"
#include <QtDebug>
#include "structures.h"

namespace LeechCraft
{
namespace CpuLoad
{
	const auto HistCount = 350;

	CpuLoadProxyObj::CpuLoadProxyObj (const QMap<LoadPriority, LoadTypeInfo>& infos)
	{
		Set (infos);
	}

	void CpuLoadProxyObj::Set (const QMap<LoadPriority, LoadTypeInfo>& infos)
	{
		Infos_ = infos;
		emit percentagesChanged ();

		for (auto i = infos.begin (), end = infos.end (); i != end; ++i)
		{
			auto& arr = History_ [i.key ()];
			arr << i.value ().LoadPercentage_ * 100;
			if (arr.size () > HistCount)
				arr.removeFirst ();
		}
		emit histChanged ();
	}

	double CpuLoadProxyObj::GetIoPercentage () const
	{
		return Infos_ [LoadPriority::IO].LoadPercentage_;
	}

	double CpuLoadProxyObj::GetLowPercentage () const
	{
		return Infos_ [LoadPriority::Low].LoadPercentage_;
	}

	double CpuLoadProxyObj::GetMediumPercentage () const
	{
		return Infos_ [LoadPriority::Medium].LoadPercentage_;
	}

	double CpuLoadProxyObj::GetHighPercentage () const
	{
		return Infos_ [LoadPriority::High].LoadPercentage_;
	}

	QList<QPointF> CpuLoadProxyObj::GetIoHist () const
	{
		return GetHist (LoadPriority::IO);
	}

	QList<QPointF> CpuLoadProxyObj::GetLowHist () const
	{
		return GetHist (LoadPriority::Low);
	}

	QList<QPointF> CpuLoadProxyObj::GetMediumHist () const
	{
		return GetHist (LoadPriority::Medium);
	}

	QList<QPointF> CpuLoadProxyObj::GetHighHist () const
	{
		return GetHist (LoadPriority::High);
	}

	QList<QPointF> CpuLoadProxyObj::GetHist (LoadPriority key) const
	{
		QList<QPointF> result;
		int i = 0;
		for (const auto pt : History_ [key])
			result.push_back ({ static_cast<double> (i++), pt });
		return result;
	}
}
}
