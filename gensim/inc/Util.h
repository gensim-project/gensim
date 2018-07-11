/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   Util.h
 * Author: s0803652
 *
 * Created on 28 September 2011, 17:36
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdint.h>

#include <vector>
#include <sstream>
#include <string>

#include <cassert>

static uint32_t rol32(uint32_t x, uint32_t n)
{
	assert (n<32);
	return (x<<n) | (x>>(-n&31));
}

static uint32_t rol64(uint32_t x, uint32_t n)
{
	assert (n<64);
	return (x<<n) | (x>>(-n&63));
}

static uint32_t ror32(uint32_t x, uint32_t n)
{
	assert (n<32);
	return (x>>n) | (x<<(-n&31));
}

static uint32_t ror64(uint32_t x, uint32_t n)
{
	assert (n<64);
	return (x>>n) | (x<<(-n&63));
}

namespace gensim
{

	namespace util
	{

		template <class k, class v>
		struct MapSecondComparison {
			bool operator()(std::pair<k, v> i, std::pair<k, v> j)
			{
				return i.second < j.second;
			}
		};

		template <class k, class v>
		struct MapFirstComparison {
			bool operator()(std::pair<k, v> i, std::pair<k, v> j)
			{
				return i.first < j.first;
			}
		};

		enum ExpressionNodeType {
			EXPNODE_ID,
			EXPNODE_VAL,
			EXPNODE_OPERATOR,
			EXPNODE_FNCALL
		};

		class expression
		{
		public:
			ExpressionNodeType nodetype;
			std::string node_str;
			uint32_t node_val;
			const expression **subexprs;
			int subexprcount;

			static const expression *Parse(void *tree);

			expression() : subexprs(NULL), subexprcount(0) {}

			// function pointer:
			// std::string fn_process_fn(this_string, function_name, arg_count, function_args)
			std::string ToString(std::string this_str, std::string (*fn_process_fn)(std::string, std::string, int, const expression **)) const;
		};

		class Util
		{
		public:
			static uint8_t Verbose_Level;

			static uint32_t parse_binary(std::string str);
			static std::string FormatBinary(uint32_t x, int width);
			static std::string StrDup(std::string todup, uint32_t n);

			static std::string Trim_Quotes(std::string);
			static std::string trim_whitespace(std::string &r);

			static void InlineFile(std::stringstream &stream, std::string filename);

			static std::vector<std::string> Tokenize(const std::string source, const std::string breaks, bool include_breaks, bool merge);
			static std::vector<std::string> Tokenize(const std::string source, const std::string breaks, bool match_brackets);
			static std::string clean(const std::string str);

			static std::string FindReplace(const std::string &input, const std::string &find, const std::string &replace);
			static size_t Match(const std::string str, size_t begin, char inc, char dec);

			static std::string TypeString(uint32_t width);

			static std::string trim(std::string, std::string);
			static std::string trim_back(std::string, std::string);
			static std::string trim_front(std::string, std::string);

		private:
			Util();
			Util(const Util &orig);
			virtual ~Util();
		};

		class cppformatstream : public std::ostringstream
		{
		public:
			std::string str() const;
		};

	}  // namespace util

}  // namespace gensim

#endif /* _UTIL_H_ */
