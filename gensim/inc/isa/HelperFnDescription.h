/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <string>
#include <vector>

namespace gensim
{
	namespace isa
	{

		class HelperFnParamDescription
		{
		public:
			std::string type;
			std::string name;
		};

		class HelperFnDescription
		{
		public:
			bool should_inline;
			std::string name;
			std::string body;
			std::string return_type;
			std::vector<HelperFnParamDescription> params;
			std::vector<std::string> attributes;

			HelperFnDescription(const std::string& body) : should_inline(false)
			{
				this->body = body;
			}

			std::string GetPrototype(const std::string& class_prefix = "") const;

			bool HasAttr(const std::string& check_attr) const
			{
				for (const auto& attr : attributes) {
					if (attr == check_attr) return true;
				}

				return false;
			}

		private:
			HelperFnDescription();
		};

	}
}
