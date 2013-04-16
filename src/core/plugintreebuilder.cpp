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

#include "plugintreebuilder.h"
#include <boost/graph/visitors.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/topological_sort.hpp>
#include "interfaces/iinfo.h"
#include "interfaces/iplugin2.h"
#include "interfaces/ipluginready.h"

namespace LeechCraft
{
	PluginTreeBuilder::VertexInfo::VertexInfo ()
	: IsFulfilled_ (false)
	, Object_ (0)
	{
	}

	PluginTreeBuilder::VertexInfo::VertexInfo (QObject *obj)
	: IsFulfilled_ (false)
	, Object_ (obj)
	{
		IInfo *info = qobject_cast<IInfo*> (obj);
		if (!info)
		{
			qWarning () << Q_FUNC_INFO
					<< obj
					<< "doesn't implement IInfo";
			throw std::runtime_error ("VertexInfo creation failed.");
		}

		AllFeatureDeps_ = QSet<QString>::fromList (info->Needs ());
		UnfulfilledFeatureDeps_ = AllFeatureDeps_;
		FeatureProvides_ = QSet<QString>::fromList (info->Provides ());

		IPluginReady *ipr = qobject_cast<IPluginReady*> (obj);
		if (ipr)
			P2PProvides_ = ipr->GetExpectedPluginClasses ();

		IPlugin2 *ip2 = qobject_cast<IPlugin2*> (obj);
		if (ip2)
		{
			AllP2PDeps_ = ip2->GetPluginClasses ();
			UnfulfilledP2PDeps_ = AllP2PDeps_;
		}
	}

	PluginTreeBuilder::PluginTreeBuilder ()
	{
	}

	void PluginTreeBuilder::AddObjects (const QObjectList& objs)
	{
		Instances_ << objs;
	}

	void PluginTreeBuilder::RemoveObject (QObject *obj)
	{
		Instances_.removeAll (obj);
	}

	template<typename Edge>
	struct CycleDetector : public boost::default_dfs_visitor
	{
		QList<Edge>& BackEdges_;

		CycleDetector (QList<Edge>& be)
		: BackEdges_ (be)
		{
		}

		template<typename Graph>
		void back_edge (Edge edge, Graph&)
		{
			BackEdges_ << edge;
		}
	};

	template<typename Graph, typename Vertex>
	struct FulfillableChecker : public boost::default_dfs_visitor
	{
		Graph& G_;
		const QList<Vertex>& BackVerts_;
		const QMap<Vertex, QList<Vertex>>& Reachable_;

		FulfillableChecker (Graph& g,
				const QList<Vertex>& backVerts,
				const QMap<Vertex, QList<Vertex>>& reachable)
		: G_ (g)
		, BackVerts_ (backVerts)
		, Reachable_ (reachable)
		{
		}

		void finish_vertex (Vertex u, const Graph&)
		{
			if (BackVerts_.contains (u))
			{
				qWarning () << G_ [u].Object_ << "is backedge";
				return;
			}

			G_ [u].IsFulfilled_ = G_ [u].UnfulfilledFeatureDeps_.isEmpty () &&
					G_ [u].UnfulfilledP2PDeps_.isEmpty ();
			if (!G_ [u].IsFulfilled_)
				qWarning () << qobject_cast<IInfo*> (G_ [u].Object_)->GetName ()
						<< "failed to initialize because of:"
						<< G_ [u].UnfulfilledFeatureDeps_
						<< G_ [u].UnfulfilledP2PDeps_;

			Q_FOREACH (const Vertex& v, Reachable_ [u])
				if (!G_ [v].IsFulfilled_)
				{
					G_ [u].IsFulfilled_ = false;
					break;
				}
		}
	};

	template<typename Graph>
	struct VertexPredicate
	{
		Graph *G_;

		VertexPredicate ()
		: G_ (0)
		{
		}

		VertexPredicate (Graph& g)
		: G_ (&g)
		{
		}

		template<typename Vertex>
		bool operator() (const Vertex& u) const
		{
			return (*G_) [u].IsFulfilled_;
		}
	};

	void PluginTreeBuilder::Calculate ()
	{
		Graph_.clear ();
		Object2Vertex_.clear ();
		Result_.clear ();

		CreateGraph ();
		QMap<Edge_t, QPair<Vertex_t, Vertex_t>> edge2vert = MakeEdges ();

		QMap<Vertex_t, QList<Vertex_t>> reachable;
		QPair<Vertex_t, Vertex_t> pair;
		Q_FOREACH (pair, edge2vert)
			reachable [pair.first] << pair.second;

		QList<Edge_t> backEdges;
		CycleDetector<Edge_t> cd (backEdges);
		boost::depth_first_search (Graph_, boost::visitor (cd));

		QList<Vertex_t> backVertices;
		Q_FOREACH (const Edge_t& backEdge, backEdges)
			backVertices << edge2vert [backEdge].first;

		FulfillableChecker<Graph_t, Vertex_t> checker (Graph_, backVertices, reachable);
		boost::depth_first_search (Graph_, boost::visitor (checker));

		typedef boost::filtered_graph<Graph_t, boost::keep_all, VertexPredicate<Graph_t>> fg_t;
		fg_t fg = fg_t (Graph_,
				boost::keep_all (),
				VertexPredicate<Graph_t> (Graph_));

		QList<Vertex_t> vertices;
		boost::topological_sort (fg,
				std::back_inserter (vertices));
		Q_FOREACH (const Vertex_t& vertex, vertices)
			Result_ << fg [vertex].Object_;
	}

	QObjectList PluginTreeBuilder::GetResult () const
	{
		return Result_;
	}

	void PluginTreeBuilder::CreateGraph ()
	{
		Q_FOREACH (QObject *object, Instances_)
		{
			try
			{
				VertexInfo vi (object);
				Vertex_t objVertex = boost::add_vertex (Graph_);
				Graph_ [objVertex] = vi;
				Object2Vertex_ [object] = objVertex;
			}
			catch (const std::exception& e)
			{
				qWarning () << Q_FUNC_INFO
						<< e.what ()
						<< "for"
						<< object
						<< "; skipping";
				continue;
			}
		}
	}

	QMap<PluginTreeBuilder::Edge_t, QPair<PluginTreeBuilder::Vertex_t, PluginTreeBuilder::Vertex_t>> PluginTreeBuilder::MakeEdges ()
	{
		QSet<QPair<Vertex_t, Vertex_t>> depVertices;
		boost::graph_traits<Graph_t>::vertex_iterator vi, vi_end;
		for (boost::tie (vi, vi_end) = boost::vertices (Graph_); vi != vi_end; ++vi)
		{
			const QSet<QString>& fdeps = Graph_ [*vi].UnfulfilledFeatureDeps_;
			const QSet<QByteArray>& pdeps = Graph_ [*vi].UnfulfilledP2PDeps_;
			if (!fdeps.size () && !pdeps.size ())
				continue;

			boost::graph_traits<Graph_t>::vertex_iterator vj, vj_end;
			for (boost::tie (vj, vj_end) = boost::vertices (Graph_); vj != vj_end; ++vj)
			{
				const QSet<QString> fInter = QSet<QString> (fdeps).intersect (Graph_ [*vj].FeatureProvides_);
				if (fInter.size ())
				{
					Graph_ [*vi].UnfulfilledFeatureDeps_.subtract (fInter);
					depVertices << qMakePair (*vi, *vj);
				}

				const QSet<QByteArray> pInter = QSet<QByteArray> (pdeps).intersect (Graph_ [*vj].P2PProvides_);
				if (pInter.size ())
				{
					Graph_ [*vi].UnfulfilledP2PDeps_.subtract (pInter);
					depVertices << qMakePair (*vi, *vj);
				}
			}
		}

		QMap<Edge_t, QPair<Vertex_t, Vertex_t>> result;
		QPair<Vertex_t, Vertex_t> pair;
		Q_FOREACH (pair, depVertices)
			result [boost::add_edge (pair.first, pair.second, Graph_).first] = pair;
		return result;
	}
}
