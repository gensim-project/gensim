/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   StructDescription.h
 * Author: harry
 *
 * Created on 10 July 2018, 15:29
 */

#ifndef STRUCTDESCRIPTION_H
#define STRUCTDESCRIPTION_H

#include <string>
#include <vector>

namespace gensim
{
	namespace isa
	{
		class StructMember
		{
		public:
			StructMember(const std::string &name, const std::string &type) : name_(name), type_(type) {}

			const std::string &GetName()
			{
				return name_;
			}
			const std::string &GetType()
			{
				return type_;
			}

		private:
			std::string name_;
			std::string type_;
		};

		class StructDescription
		{
		public:
			using MembersT = std::vector<StructMember>;

			StructDescription(const std::string &name) : name_(name) {}
			StructDescription(const std::string &name, const std::initializer_list<StructMember> &members) : name_(name), members_(members) {}

			void AddMember(const StructMember &member)
			{
				members_.push_back(member);
			}
			void AddMember(const std::string &name, const std::string &type)
			{
				members_.push_back(StructMember(name, type));
			}

			const MembersT &GetMembers() const
			{
				return members_;
			}
			const std::string &GetName() const
			{
				return name_;
			}


		private:
			std::string name_;
			MembersT members_;
		};
	}
}

#endif /* STRUCTDESCRIPTION_H */

