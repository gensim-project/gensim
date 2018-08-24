/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "SSAFormAction.h"
#include "SSAValueNamespace.h"
#include "SSATypeManager.h"

#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <vector>
#include <queue>

#include <stdint.h>

namespace gensim
{
	class DiagnosticContext;

	namespace arch
	{
		class ArchDescription;
	}

	namespace genc
	{

		namespace ssa
		{
			class SSAActionBase;

			class SSAContext
			{
			public:
				typedef std::map<std::string, SSAActionBase *> ActionList;
				typedef ActionList::iterator ActionListIterator;
				typedef ActionList::const_iterator ActionListConstIterator;

				SSAContext(const gensim::isa::ISADescription& isa, const gensim::arch::ArchDescription& arch);
				SSAContext(const gensim::isa::ISADescription& isa, const gensim::arch::ArchDescription& arch, std::shared_ptr<SSATypeManager> type_manager);
				virtual ~SSAContext();

				const gensim::arch::ArchDescription& GetArchDescription() const
				{
					return arch_;
				}

				const gensim::isa::ISADescription& GetIsaDescription() const
				{
					return isa_;
				}

				bool Resolve(DiagnosticContext& ctx);
				bool Validate(DiagnosticContext &ctx);

				const ActionList& Actions() const
				{
					return actions_;
				}
				ActionListConstIterator ActionsBegin() const
				{
					return actions_.begin();
				}
				ActionListConstIterator ExecuteEnd() const
				{
					return actions_.end();
				}

				void AddAction(SSAActionBase *action)
				{
					actions_.insert(std::make_pair(action->GetPrototype().GetIRSignature().GetName(), action));
				}

				void RemoveAction(SSAActionBase *action)
				{
					actions_.erase(action->GetPrototype().GetIRSignature().GetName());
				}

				bool HasAction(const std::string& action_name) const
				{
					return actions_.find(action_name) != actions_.end();
				}

				const SSAActionBase *GetAction(const std::string& action_name) const
				{
					return actions_.at(action_name);
				}
				SSAActionBase *GetAction(const std::string& action_name)
				{
					return actions_.at(action_name);
				}

				SSAValueNamespace &GetValueNamespace()
				{
					return vns_;
				}

				SSATypeManager& GetTypeManager()
				{
					return *type_manager_;
				}

				bool ShouldParallelOptimise() const
				{
					return parallel_optimise_;
				}

				bool ShouldTestOptimise() const
				{
					return test_optimise_;
				}

				void SetParallelOptimise(bool o)
				{
					parallel_optimise_ = o;
				}

				void SetTestOptimise(bool o)
				{
					test_optimise_ = o;
				}


				void Optimise();
			private:
				void Optimise(SSAFormAction* action);

				const gensim::arch::ArchDescription& arch_;
				const gensim::isa::ISADescription& isa_;
				ActionList actions_;

				SSAValueNamespace vns_;
				std::shared_ptr<SSATypeManager> type_manager_;
				bool parallel_optimise_;
				bool test_optimise_;
			};
		}
	}
}
