/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   Exception.h
 * Author: harry
 *
 * Created on 24 July 2017, 16:03
 */

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <string>

namespace gensim
{
	class Exception
	{
	public:
		Exception() : what_("") {}
		Exception(const std::string &what) : what_(what) {}
		const std::string &what() const
		{
			return what_;
		}
	private:
		const std::string what_;
	};

	class NotImplementedException : public Exception
	{
	public:
		NotImplementedException(const char *file, size_t line) : Exception(std::string("Not Implemented: ") + std::string(file) + ":" + std::to_string(line)) {}
	};

	class NotExpectedException : public Exception
	{
	public:
		NotExpectedException(const char *file, size_t line) : Exception(std::string("Unexpected: ") + std::string(file) + ":" + std::to_string(line)) {}
	};

	class AssertionFailedException : public Exception
	{
	public:
		AssertionFailedException(const char *file, size_t line, const std::string &what) : Exception(std::string("Assertion Failed: ") + std::string(file) + ":" + std::to_string(line) + ": " + what) {}
	};

}

#endif /* EXCEPTION_H */

