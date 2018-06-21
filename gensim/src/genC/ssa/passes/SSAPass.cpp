/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/passes/SSAPass.h"
#include "ComponentManager.h"

using namespace gensim::genc::ssa;

SSAPassManager::SSAPassManager() : multirun_all_(true), multirun_each_(true)
{

}


void SSAPassManager::AddPass(const SSAPass* pass)
{
	passes_.push_back(pass);
}

bool SSAPassManager::Run(SSAContext& ctx)
{
	bool changed = false;
	for(auto action : ctx.Actions()) {
		SSAActionBase *base = action.second;
		if(SSAFormAction *safaction = dynamic_cast<SSAFormAction*>(base)) {
			changed |= Run(*safaction);
		}
	}
	return changed;
}

bool SSAPassManager::Run(SSAFormAction &action)
{
	bool changed = false;
	bool anychanged = false;
	do {
		changed = false;
		for(auto i : passes_) {
			RunDebugPasses(action);
			if(multirun_each_) {
				while(i->Run(action)) {
					changed = true;
					RunDebugPasses(action);
				}
			} else {
				changed |= i->Run(action);
				RunDebugPasses(action);
			}
		}
		anychanged |= changed;
	} while(changed && multirun_all_);
	return anychanged;
}

void SSAPassManager::RunDebugPasses(SSAFormAction& action)
{
	for(auto i : debug_passes_) {
		i->Run(action);
	}
}

void SSAPassManager::AddDebugPass(const SSAPass* pass)
{
	debug_passes_.push_back(pass);
}

SSAPass::~SSAPass()
{

}

SSAPassDB *SSAPassDB::singleton_ = nullptr;

const SSAPass* SSAPassDB::Get(const std::string& passname)
{
	return GetSingleton().GetPass(passname);
}


SSAPassDB& SSAPassDB::GetSingleton()
{
	if(singleton_ == nullptr) {
		singleton_ = new SSAPassDB();
	}
	return *singleton_;
}

SSAPass* SSAPassDB::GetPass(const std::string& passname)
{
	if(passes_.count(passname) == 0) {
		passes_[passname] = GetComponent<SSAPass>(passname);
	}
	return passes_.at(passname);
}


DefineComponentType0(SSAPass)
