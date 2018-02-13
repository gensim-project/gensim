/*
 * ComponentManager.h
 *
 *  Created on: 9 Dec 2013
 *      Author: harry
 */

#ifndef COMPONENTMANAGER_H_
#define COMPONENTMANAGER_H_

#include <map>
#include <string>
#include <sstream>
#include <vector>

template <typename T, typename... Args> class ComponentManager
{
public:
	typedef T* (*t_factory_t)(Args...);

	struct FactoryStruct {
		t_factory_t FactoryFn;
		std::string Description;
	};

	typedef std::map<std::string, FactoryStruct> FactoryMap;

	static FactoryMap Factories;

	static T *Instantiate(std::string str, Args &... args)
	{
		if (Factories.find(str) != Factories.end()) return Factories.at(str).FactoryFn(args...);
		return 0;
	}

	static void Register(std::string name, std::string desc, t_factory_t factory)
	{
		FactoryStruct f;
		f.FactoryFn = factory;
		f.Description = desc;
		Factories[name] = f;
	}

	static void Dump(std::ostringstream &str)
	{
		str << "Registered Components: " << std::endl;
		str << "================================================================================" << std::endl;
		for(const auto &f : Factories) {
			str << "\t" << f.first << "\t\t" << f.second.Description << std::endl;
		}
		str << "================================================================================" << std::endl;
	}
};

template<typename BaseType, typename T, typename... Args> BaseType* ComponentFactory(Args&... args)
{
	return new T(args...);
}

template<typename BaseType, typename... Args> bool GetComponentInstance(std::string name, BaseType *& out, Args &... args)
{
	BaseType *p = ComponentManager<BaseType, Args...>::Instantiate(name, args...);
	if (p) {
		out = p;
		return true;
	} else
		return false;
}

template<typename BaseType, typename... Args> BaseType* GetComponent(std::string name, Args &... args)
{
	BaseType *p = ComponentManager<BaseType, Args...>::Instantiate(name, args...);
	if(!p) {
		throw std::logic_error("Unknown component type");
	}
	return p;
}

template<typename BaseType, typename... Args> std::string GetRegisteredComponents(BaseType *, const Args &... args)
{
	std::ostringstream str;
	ComponentManager<BaseType, Args...>::Dump(str);
	return str.str();
}

template<typename BaseType, typename... Args> std::vector<std::string> GetRegisteredComponentNames()
{
	std::vector<std::string> names;
	for(auto i : ComponentManager<BaseType, Args...>::Factories) {
		names.push_back(i.first);
	}
	return names;
}


#define DefineComponentType(Type, ...) \
	template<> ComponentManager<Type, ##__VA_ARGS__>::FactoryMap ComponentManager<Type, ##__VA_ARGS__>::Factories __attribute__((init_priority(102))) = ComponentManager<Type, ##__VA_ARGS__>::FactoryMap();

#define DefineComponentType0(Type) \
	template<> ComponentManager<Type>::FactoryMap ComponentManager<Type>::Factories __attribute__((init_priority(102))) = ComponentManager<Type>::FactoryMap();


#define RegisterComponent(BaseType, Type, Name, Description, ...) \
	namespace __##Type { \
	class ComponentRegistrationHelper \
	{ public: ComponentRegistrationHelper() { ComponentManager<BaseType, ##__VA_ARGS__>::Register(Name, Description, ComponentFactory<BaseType, Type, ##__VA_ARGS__>); } \
	};\
	static ComponentRegistrationHelper singleton __attribute__((init_priority(103))) = ComponentRegistrationHelper(); \
	}


#define RegisterComponent0(BaseType, Type, Name, Description) \
	namespace __##Type { \
	class ComponentRegistrationHelper \
	{ public: ComponentRegistrationHelper() { ComponentManager<BaseType>::Register(Name, Description, ComponentFactory<BaseType, Type>); } \
	};\
	static ComponentRegistrationHelper singleton __attribute__((init_priority(103))) = ComponentRegistrationHelper(); \
	}


#endif /* COMPONENTMANAGER_H_ */
