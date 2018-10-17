/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   wlist.h
 * Author: harry
 *
 * Created on 25 July 2018, 15:21
 */

#ifndef WLIST_H
#define WLIST_H

namespace wutils
{

	template<typename T> class wlist
	{
	public:
		using wsize_t = unsigned long;

		class wlist_node
		{
		public:
			wlist_node(const T& data, wlist_node *next) : data_(data), next_(next) {}
			~wlist_node()
			{
				delete next_;
			}

			T& data()
			{
				return data_;
			}
			wlist_node *next()
			{
				return next_;
			}

		private:
			T data_;
			wlist_node *next_;
		};

		class iterator
		{
		public:
			iterator(wlist_node *node) : node_(node) {}

			T& operator*()
			{
				return node_->data();
			}

			iterator operator++()
			{
				iterator old_it (node_);
				node_ = node_->next();
				return old_it;
			}
			bool operator!=(const iterator &other)
			{
				return node_ != other.node_;
			}
		private:
			wlist_node *node_;
		};

		wlist() : head_(nullptr) {}
		~wlist()
		{
			delete head_;
		}

		iterator begin()
		{
			return iterator(head_);
		}
		iterator end()
		{
			return iterator(nullptr);
		}

		void push_front(const T& t)
		{
			head_ = new wlist_node(t, head_);
		}
		void pop_front()
		{
			auto old_head = head_;
			head_ = head_->next();
			delete old_head;
		}

		T& front()
		{
			return head_->data();
		}

		wsize_t size() const
		{
			auto i = head_;
			wsize_t count = 0;
			while(i) {
				count++;
				i = i->next();
			}
			return count;
		}

	private:
		wlist_node *head_;
	};

}

#endif /* WLIST_H */

