/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   IRSignature.h
 * Author: harry
 *
 * Created on 22 April 2017, 13:23
 */

#ifndef IRSIGNATURE_H
#define IRSIGNATURE_H

#include "genC/ir/IRType.h"
#include "genC/ir/IRAttributes.h"

#include <string>
#include <vector>

namespace gensim
{
	namespace genc
	{
		class IRParam
		{
		public:

			const std::string GetName() const
			{
				return name_;
			}

			const IRType &GetType() const
			{
				return type_;
			}

			IRParam(const std::string &name, const IRType &type) : name_(name), type_(type) {}

		private:
			std::string name_;
			IRType type_;
		};

		class IRSignature
		{
		public:
			typedef std::vector<IRParam> param_type_list_t;
			typedef std::vector<ActionAttribute::EActionAttribute> attribute_list_t;

			IRSignature(const std::string& name, const IRType& return_type, const param_type_list_t& params = {});

			void AddAttribute(ActionAttribute::EActionAttribute attribute)
			{
				attributes_.push_back(attribute);
			}

			bool HasAttribute(ActionAttribute::EActionAttribute attribute) const
			{
				for (auto i : GetAttributes()) {
					if (i == attribute) {
						return true;
					}
				}

				return false;
			}

			const std::string& GetName() const
			{
				return name_;
			}
			const IRType& GetType() const
			{
				return return_type_;
			}
			const param_type_list_t& GetParams() const
			{
				return parameters_;
			}
			const attribute_list_t& GetAttributes() const
			{
				return attributes_;
			}
			bool HasReturnValue() const
			{
				return return_type_ != IRTypes::Void;
			}

		private:
			const std::string name_;
			const IRType return_type_;
			param_type_list_t parameters_;
			attribute_list_t attributes_;
		};

	}
}

#endif /* IRSIGNATURE_H */

