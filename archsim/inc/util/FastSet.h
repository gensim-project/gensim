/*
 * File:   FastSet.h
 * Author: s0803652
 *
 * Created on 01 March 2012, 13:32
 */

#ifndef FASTSET_H
#define FASTSET_H

#include <cassert>

#include <bitset>
#include <vector>
#include <functional>
#include <memory>
#include <set>

// 8KB, 512b group size

template <class Key, int NodeSize = 512>
class fastset_node
{
private:
	std::bitset<NodeSize>* Node;

public:
	inline void insert(const Key value)
	{
		if (Node == NULL) Node = new std::bitset<NodeSize>();
		(*Node)[value % NodeSize] = true;
	}

	inline bool get(const Key value) const
	{
		if (Node == NULL) return false;
		return (*Node)[value % NodeSize];
	}

	inline void erase(const Key value)
	{
		if (Node == NULL) return;
		(*Node)[value % NodeSize] = false;
		if (Node->none()) {
			delete Node;
			Node = NULL;
		}
	}

	inline void clear()
	{
		if (Node == NULL) return;
		delete Node;
		Node = NULL;
	}

	fastset_node()
	{
		Node = NULL;
	}
};

template <class Key>
class fastset_node<Key, 512>
{
private:
	uint64_t* Bits;

public:
	inline void insert(const Key value)
	{
		if (Bits == NULL) Bits = (uint64_t*)malloc(sizeof(uint64_t) * 8);
		Bits[value % 8] |= (1 << (value % 64));
	}

	inline bool get(const Key value) const
	{
		if (Bits == NULL) return false;
		return Bits[value % 8] & (1 << (value % 64));
	}

	inline void erase(const Key value)
	{
		if (Bits == NULL) return;
		Bits[value % 8] &= ~(1 << (value % 64));
	}

	inline void clear()
	{
		if (Bits == NULL) return;
		free(Bits);
	}

	fastset_node()
	{
		Bits = NULL;
	}

	~fastset_node()
	{
		clear();
	}
};

template <class Key, int Size = 8192, int NodeSize = 512>
class fastset
{
private:
	std::vector<fastset_node<Key, NodeSize> > Nodes;
	typedef typename std::vector<fastset_node<Key, NodeSize> >::iterator nodes_iterator;

public:
	fastset()
	{
		Nodes.reserve(Size / NodeSize);
		while (Nodes.size() < (Size / NodeSize)) Nodes.push_back(fastset_node<Key, NodeSize>());
	}

	inline void insert(const Key value)
	{
		Nodes[(value % Size) / NodeSize].insert(value);
	}

	inline void erase(const Key value)
	{
		Nodes.at((value % Size) / NodeSize).erase(value);
	}

	inline bool get(const Key value)
	{
		return Nodes[(value % Size) / NodeSize].get(value);
	}

	inline void clear()
	{
		for (nodes_iterator i = Nodes.begin(); i != Nodes.end(); ++i) i->clear();
	}

	~fastset()
	{
		for (nodes_iterator i = Nodes.begin(); i != Nodes.end(); ++i) i->clear();
	}
};

#endif /* FASTSET_H */
