/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Tarjan.h
 * Author: harry
 *
 * Created on 17 October 2017, 13:04
 */

#ifndef TARJAN_H
#define TARJAN_H

#include <list>
#include <map>
#include <set>
#include <vector>
#include <iterator>
#include <functional>

#define DEBUGPRINT if(true) {} else std::cout

namespace gensim
{
	namespace util
	{

		// Implements Tarjan's algorithm for computing strongly connected
		// components. Currently quite inefficient, since it uses lots of
		// STL containers.
		template<typename NodeT> class Tarjan
		{
		public:
			typedef Tarjan<NodeT> this_t;
			typedef NodeT node_t;
			typedef std::pair<NodeT, NodeT> edge_t;
			typedef std::set<node_t> scc_t;
			typedef std::set<scc_t> output_t;

			typedef std::vector<node_t> node_list_t;
			typedef std::vector<edge_t> edge_list_t;


			template<typename NodeIterator, typename EdgeIterator> Tarjan(NodeIterator nodes_start, NodeIterator nodes_end, EdgeIterator edges_start, EdgeIterator edges_end) :
				nodes(node_list_t(nodes_start, nodes_end)),
				edges(edge_list_t(edges_start, edges_end)),
				index(0)
			{

			}

			output_t Compute()
			{
				for(auto i : nodes) {
					graph[i] = {};
				}
				for(auto i : edges) {
					graph[i.first].insert(i.second);
				}

				for(auto node : nodes) {
					if(indices.count(node) == 0) {
						strong_connect(node);
					}
				}

				return output;
			}

		private:
			void strong_connect(NodeT v)
			{
				DEBUGPRINT << "Performing strong_connect(" << v << ")" << std::endl;

				indices[v] = index;
				lowlink[v] = index;
				index++;
				S.push_back(v);
				onstack.insert(v);

				for(const node_t &w : graph.at(v)) {
					if(indices.count(w) == 0) {
						strong_connect(w);
						lowlink[v] = std::min(lowlink[v], lowlink[w]);
					} else if(onstack.count(w)) {
						lowlink[v] = std::min(lowlink[v], indices[w]);
					}
				}
				DEBUGPRINT << v << " got index: " << indices[v] << ", lowlink " << lowlink[v] << std::endl;

				if(lowlink[v] == indices[v]) {
					DEBUGPRINT << v << " is a root node" << std::endl;
					scc_t scc;
					node_t w;
					do {
						w = S.back();
						S.pop_back();

						onstack.erase(w);
						scc.insert(w);

						DEBUGPRINT << "Added " << w << " to scc" << std::endl;
					} while(w != v);

					output.insert(scc);
				}
			}

			node_list_t nodes;
			edge_list_t edges;

			output_t output;

			std::map<node_t, std::set<node_t>> graph;
			uint32_t index;
			std::list<NodeT> S;

			std::map<node_t, uint32_t> indices;
			std::map<node_t, uint32_t> lowlink;

			std::set<node_t> onstack;
		};
	}
}

#undef DEBUGPRINT

#endif /* TARJAN_H */

