/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "generators/GenerationManager.h"
#include "Util.h"

namespace gensim
{
	namespace generator
	{

		std::string GenerationManager::FnDecode = "decode";
		std::string GenerationManager::FnDisasm = "disasm";
		std::string GenerationManager::FnInterpret = "interpret_engine";
		std::string GenerationManager::FnTranslate = "translate";
		std::string GenerationManager::FnPipeline = "pipeline";
		std::string GenerationManager::FnJumpInfo = "jumpinfo";

		FunctionEntry::FunctionEntry(const std::string prototype, const std::string& body, const std::vector<std::string> &local_headers, const std::vector<std::string> &sys_headers, const std::vector<std::string> &specialisations, bool global) : prototype_(prototype), body_(body), local_headers_(local_headers), system_headers_(sys_headers), specialisations_(specialisations), is_global_(global)
		{

		}

		size_t FunctionEntry::GetBodySize() const
		{
			return body_.size();
		}

		bool FunctionEntry::IsGlobal() const
		{
			return is_global_;
		}

		std::string FunctionEntry::Format() const
		{
			// We could be smart with figuring out which headers we need to
			// re-include but for now just blast them all out for every function
			// and let the preprocessor deal with it
			std::stringstream str;
			
			str << body_;
			
			// If that was a template, then emit any specialisations that should be instantiated
			for(auto i : specialisations_) {
				str << i << ";";
			}

			return str.str();
		}
		std::string FunctionEntry::FormatPrototype() const
		{
			return prototype_;
		}

		std::string FunctionEntry::FormatIncludes() const
		{
			std::stringstream str;
			for(auto i : local_headers_) {
				str << "#include \"" << i << "\"\n";
			}
			for(auto i : system_headers_) {
				str << "#include <" << i << ">\n";
			}
			return str.str();
		}

		const std::vector<std::string>& FunctionEntry::GetLocalHeaders() const
		{
			return local_headers_;
		}

		const std::vector<std::string>& FunctionEntry::GetSystemHeaders() const
		{
			return system_headers_;
		}





		std::map<std::string, std::map<std::string, GenerationOption*> > GenerationComponent::Options __attribute__((init_priority(101)));
		std::map<std::string, std::string> GenerationComponent::Inheritance __attribute__((init_priority(102)));

		std::list<GenerationComponentInitializer*> GenerationComponentInitializer::Initializers __attribute__((init_priority(103)));

		GenerationOptionInitializer::GenerationOptionInitializer(const char* const component, const char* const name, const char* const default_value, const char* const help_string)
		{
			for (std::list<GenerationComponentInitializer*>::const_iterator ci = GenerationComponentInitializer::Initializers.begin(); ci != GenerationComponentInitializer::Initializers.end(); ++ci) {
				if ((*ci)->GetName() == component) {
					GenerationComponent::Options[component][name] = new GenerationOption(name, default_value, help_string);
					return;
				}
			}
			assert(false && "Unrecognized component type");
		}

		GenerationInheritanceInitializer::GenerationInheritanceInitializer(const char* const _sub, const char* const _super) : Subcomponent(_sub), Supercomponent(_super)
		{
			GenerationComponent::Inheritance[Subcomponent] = Supercomponent;
		}

		void GenerationManager::AddComponent(GenerationComponent& component)
		{
			Components.insert(std::pair<std::string, GenerationComponent*>(component.GetFunction(), &component));
			_components.push_back(&component);
		}

		GenerationComponent* GenerationManager::GetComponent(const std::string str)
		{
			std::map<std::string, GenerationComponent*>::iterator it = Components.find(str);
			if (it == Components.end()) return NULL;
			return (it->second);
		}

		const GenerationComponent* GenerationManager::GetComponentC(const std::string str) const
		{
			std::map<std::string, GenerationComponent*>::const_iterator it = Components.find(str);
			if (it == Components.end()) return NULL;
			return (it->second);
		}

		const std::vector<GenerationComponent*>& GenerationManager::GetComponents() const
		{
			if (!_components_up_to_date) {
				_components.clear();
				for (std::map<std::string, GenerationComponent*>::const_iterator ci = Components.begin(); ci != Components.end(); ++ci) {
					_components.push_back(ci->second);
				}
			}
			return _components;
		}

		bool GenerationManager::Generate()
		{
			mkdir(target.c_str(), S_IRWXU);

			bool success = true;
			for (std::vector<GenerationComponent*>::iterator i = _components.begin(); i != _components.end(); ++i) {
				(*i)->Reset();
			}

			GenerationSetupManager gsm(*this);
			for (std::vector<GenerationComponent*>::iterator i = _components.begin(); i != _components.end(); ++i) {
				(*i)->Setup(gsm);
			}

			for (std::vector<GenerationComponent*>::iterator i = _components.begin(); i != _components.end(); ++i) {
				bool component_success = (*i)->Generate();
				success &= component_success;
				if(!component_success) {
					fprintf(stderr, "Generation failure in component %s!\n", (*i)->name.c_str());
				}
			}

			return success;
		}

		std::list<std::string> GenerationComponent::GetPropertyList() const
		{
			std::list<std::string> rVal;
			for (std::map<std::string, std::string>::const_iterator i = Properties.begin(); i != Properties.end(); ++i) {
				rVal.push_back(i->first);
			}
			return rVal;
		}

		std::string GenerationComponent::GetProperty(const std::string key) const
		{
			if (Properties.find(key) != Properties.end()) return Properties.at(key);

			std::string component = name;
			if (Options[component].find(key) != Options[component].end()) return Options[component][key]->DefaultValue;

			while (Inheritance.find(component) != Inheritance.end()) {
				component = Inheritance[component];
				if (Options[component].find(key) != Options[component].end()) return Options[component][key]->DefaultValue;
			}

			fprintf(stderr, "Undefined property: %s\n", key.c_str());
			abort();
		}

		bool GenerationComponent::HasProperty(const std::string key) const
		{
			if (Properties.find(key) != Properties.end()) return true;

			std::string component = name;
			if (Options[component].find(key) != Options[component].end()) return true;

			while (Inheritance.find(component) != Inheritance.end()) {
				component = Inheritance[component];
				if (Options[component].find(key) != Options[component].end()) return true;
			}

			return false;
		}

		void GenerationComponent::WriteOutputFile(const std::string filename, const std::stringstream& contents) const
		{
			std::string path = Manager.GetTarget();
			path.append("/");
			path.append(filename);
			std::ofstream file(path.c_str());

			util::cppformatstream temp;
			temp << contents.str();

			file << temp.str();
			file.flush();
			file.close();
		}

		void GenerationComponent::WriteOutputFile(const std::string filename, const util::cppformatstream& contents) const
		{
			std::string path = Manager.GetTarget();
			path.append("/");
			path.append(filename);
			std::ofstream file(path.c_str());

			file << contents.str();
			file.flush();
			file.close();
		}

	}  // namespace generator
}  // namespace gensim
