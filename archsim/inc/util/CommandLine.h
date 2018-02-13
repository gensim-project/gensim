/*
 * File:   CommandLine.h
 * Author: s0457958
 *
 * Created on 06 February 2014, 11:46
 */

#ifndef COMMANDLINE_H
#define	COMMANDLINE_H

#include <string>
#include <map>
#include <list>

#include "SimOptions.h"

namespace archsim
{
	namespace util
	{
		class ArgumentDescriptor;

		class CommandLineManager
		{
			friend class ArgumentDescriptor;

		public:
			CommandLineManager();
			~CommandLineManager();

			bool Parse(int argc, char **argv);
			void PrintUsage();

			static int guest_argc;
			static char **guest_argv;

		private:
			static std::map<char, ArgumentDescriptor *> ShortMap;
			static std::map<std::string, ArgumentDescriptor *> LongMap;
			static std::list<ArgumentDescriptor *> RegisteredArguments;

			bool FindDescriptor(std::string long_argument, ArgumentDescriptor*& handler);
			bool FindDescriptor(char short_argument, ArgumentDescriptor*& handler);

			std::string binary_name;
		};

		class ArgumentDescriptor
		{
		public:
			enum ValueValidity {
				ValueForbidden,
				ValueOptional,
				ValueRequired
			};

			enum MultipleValidity {
				MultipleForbidden,
				MultiplePermitted,
			};

			ArgumentDescriptor(char short_, std::string long_, ValueValidity vv_ = ValueForbidden, MultipleValidity mv_ = MultipleForbidden)
				: value_validity(vv_),
				  multiple_validity(mv_),
				  short_tag(short_),
				  long_tag(long_)
			{
				CommandLineManager::ShortMap[short_] = this;
				CommandLineManager::LongMap[long_] = this;
				CommandLineManager::RegisteredArguments.push_back(this);
			}

			ArgumentDescriptor(std::string long_, ValueValidity vv_ = ValueForbidden, MultipleValidity mv_ = MultipleForbidden)
				: value_validity(vv_),
				  multiple_validity(mv_),
				  short_tag(0),
				  long_tag(long_)
			{
				CommandLineManager::LongMap[long_] = this;
				CommandLineManager::RegisteredArguments.push_back(this);
			}

			ArgumentDescriptor(char short_, ValueValidity vv_ = ValueForbidden, MultipleValidity mv_ = MultipleForbidden)
				: value_validity(vv_),
				  multiple_validity(mv_),
				  short_tag(short_),
				  long_tag("")
			{
				CommandLineManager::ShortMap[short_] = this;
				CommandLineManager::RegisteredArguments.push_back(this);
			}

			ValueValidity value_validity;
			MultipleValidity multiple_validity;

			virtual void MarkPresent() = 0;
			virtual void Marshal(std::string str) = 0;

			virtual std::string GetName() = 0;
			virtual std::string GetDescription() = 0;

			inline bool HasShort()
			{
				return short_tag != 0;
			}

			inline char GetShort()
			{
				return short_tag;
			}

			inline bool HasLong()
			{
				return long_tag != "";
			}

			inline std::string GetLong()
			{
				return long_tag;
			}

		private:
			char short_tag;
			std::string long_tag;
		};

		template<typename T>
		class SimOptionArgumentDescriptor : public ArgumentDescriptor
		{
		public:
			SimOptionArgumentDescriptor(char short_, std::string long_, archsim::SimOption<T>& option_, ArgumentDescriptor::ValueValidity vv_ = ArgumentDescriptor::ValueForbidden, ArgumentDescriptor::MultipleValidity mv_ = ArgumentDescriptor::MultipleForbidden)
				: ArgumentDescriptor(short_, long_, vv_, mv_),
				  option(option_)
			{

			}

			SimOptionArgumentDescriptor(char short_, archsim::SimOption<T>& option_, ArgumentDescriptor::ValueValidity vv_ = ArgumentDescriptor::ValueForbidden, ArgumentDescriptor::MultipleValidity mv_ = ArgumentDescriptor::MultipleForbidden)
				: ArgumentDescriptor(short_, vv_, mv_),
				  option(option_)
			{

			}

			SimOptionArgumentDescriptor(std::string long_, archsim::SimOption<T>& option_, ArgumentDescriptor::ValueValidity vv_ = ArgumentDescriptor::ValueForbidden, ArgumentDescriptor::MultipleValidity mv_ = ArgumentDescriptor::MultipleForbidden)
				: ArgumentDescriptor(long_, vv_, mv_),
				  option(option_)
			{

			}

			void MarkPresent()
			{
				option.SetIsSpecified();
			}

			void Marshal(std::string str);

			std::string GetName()
			{
				return option.GetName();
			}

			std::string GetDescription()
			{
				return option.GetDescription();
			}

		private:
			archsim::SimOption<T>& option;
		};

	}
}

#endif	/* COMMANDLINE_H */
