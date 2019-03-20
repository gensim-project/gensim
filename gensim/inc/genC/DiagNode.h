/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <string>
#include <ostream>

#include <flexbison_harness.h>

struct ANTLR3_BASE_TREE_struct;
typedef struct ANTLR3_BASE_TREE_struct *pANTLR3_BASE_TREE;

namespace gensim
{

	class DiagNode
	{
	public:
		DiagNode() : filename_("???"), line_number_(-1), column_(-1)
		{
		}

		DiagNode(const std::string& filename) : filename_(filename), line_number_(-1), column_(-1)
		{
		}

		DiagNode(const std::string& filename, uint32_t line, uint32_t column = -1) : filename_(filename), line_number_(line), column_(column)
		{
		}

		DiagNode(const std::string& filename, pANTLR3_BASE_TREE node);
		DiagNode(const std::string& filename, const location_data &node);

		DiagNode(const DiagNode& o) : line_number_(o.line_number_), column_(o.column_), filename_(o.filename_) { }
		DiagNode(DiagNode&& o) : line_number_(o.line_number_), column_(o.column_), filename_(std::move(o.filename_)) { }

		DiagNode& operator=(const DiagNode& o)
		{
			line_number_ = o.line_number_;
			column_ = o.column_;
			filename_ = o.filename_;

			return *this;
		}

		const std::string &Filename() const
		{
			return filename_;
		}

		uint32_t Line() const
		{
			return line_number_;
		}

		uint32_t Column() const
		{
			return column_;
		}

	private:
		uint32_t line_number_;
		uint32_t column_;
		std::string filename_;
	};
}

std::ostream &operator<<(std::ostream &str, const gensim::DiagNode& node);
