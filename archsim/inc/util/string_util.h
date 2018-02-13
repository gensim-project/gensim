/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   string_util.h
 * Author: harry
 *
 * Created on 06 February 2018, 14:39
 */

#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <vector>
#include <string>
#include <sstream>

namespace archsim {
	namespace util {

		std::vector<std::string> split_string(const std::string &input, char delim);

		template<typename iterator> std::string join_string(iterator begin, iterator end, char delim)
		{
			if (begin == end) {
				return "";
			}
			std::ostringstream str;

			auto it = begin;
			str << *it;
			++it;

			while (it != end) {
				str << delim << *it;
				++it;
			}

			return str.str();
		}

		std::string join_string(const std::vector<std::string> &atoms, char delim);

	}
}

#endif /* STRING_UTIL_H */

