/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SimOptions.h
 * Author: s0457958
 *
 * Created on 06 February 2014, 12:34
 */

#ifndef SIMOPTIONS_H
#define	SIMOPTIONS_H

#include <string>
#include <list>
#include <map>
#include <cstdint>

namespace archsim
{
	class SimOptionBase
	{
	public:
		SimOptionBase(std::string cat_, std::string name_, std::string description_)
			: category(cat_), name(name_), description(description_)
		{
			RegisteredOptions[cat_].push_back(this);
		}

		inline std::string GetCategory()
		{
			return category;
		}

		inline std::string GetName()
		{
			return name;
		}

		inline std::string GetDescription()
		{
			return description;
		}

	private:
		std::string category;
		std::string name;
		std::string description;

	public:
		static std::map<std::string, std::list<SimOptionBase *> > RegisteredOptions;
	};

	template<typename T>
	class SimOption : public SimOptionBase
	{
	public:
		SimOption(std::string cat_, std::string name_, std::string description_, T default_val_)
			: SimOptionBase(cat_, name_, description_), value(default_val_), is_specified(false)
		{

		}

		inline void SetIsSpecified()
		{
			is_specified = true;
		}

		inline bool IsSpecified() const
		{
			return is_specified;
		}

		inline void SetValue(T new_value)
		{
			value = new_value;
		}

		inline T GetValue() const
		{
			return value;
		}

		inline operator T() const
		{
			return value;
		}

		inline bool operator==(T a) const
		{
			return value == a;
		}

		inline bool operator!=(T a) const
		{
			return value != a;
		}

	private:
		T value;
		bool is_specified;
	};

	class OptionHandler
	{
	public:
		virtual bool IsValid(std::string value) = 0;
	};

#define OptionHandler(_o) \
    class __opt_h_ ## _o; \
    __opt_h_ ## _o *XX; \
    class __opt_h_ ## _o : public archsim::OptionHandler

	namespace options
	{
		extern void PrintOptions();

#ifdef __IMPLEMENT_OPTIONS
# define __STRINGIFY(x) #x
# define STRINGIFY(x) __STRINGIFY(x)
# define DefineOption(_cat, _t, _n, _d, _v) archsim::SimOption<_t> __attribute__((init_priority(130))) _n(STRINGIFY(_cat), STRINGIFY(_n), _d, _v)
#else
# define DefineOption(_cat, _t, _n, _d, _v) extern archsim::SimOption<_t> _n
#endif

#define DefineCategory(_cat, _name)

#define DefineSetting(_cat, _n, _d, _v)		DefineOption(_cat, std::string, _n, _d, _v)
#define DefineListSetting(_cat, _n, _d, _v)	DefineOption(_cat, std::list<std::string> *, _n, _d, _v)
#define DefineIntSetting(_cat, _n, _d, _v)	DefineOption(_cat, uint32_t, _n, _d, _v)
#define DefineFloatSetting(_cat, _n, _d, _v)	DefineOption(_cat, float, _n, _d, _v)
#define DefineInt64Setting(_cat, _n, _d, _v)	DefineOption(_cat, uint64_t, _n, _d, _v)
#define DefineFlag(_cat, _n, _d, _v)		DefineOption(_cat, bool, _n, _d, _v)

#define __MAY_INCLUDE_SD
#include "SimOptionDefinitions.h"
#undef __MAY_INCLUDE_SD

#undef DefineFlag
#undef DefineIntSetting
#undef DefineSetting
#undef DefineOption
#undef DefineCategory
	}
}

#endif	/* SIMOPTIONS_H */
