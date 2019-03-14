/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef LLVMREGISTEROPTIMISATIONPASS_H__
#define LLVMREGISTEROPTIMISATIONPASS_H__

#include "define.h"
#include <llvm/Pass.h>
#include <llvm/Analysis/RegionInfo.h>

#include <wutils/vset.h>

#include <vector>
#include <unordered_set>

namespace archsim
{
	namespace translate
	{
		namespace translate_llvm
		{

			class TagContext
			{
			public:
				TagContext() : next_tag_(0) {}
				uint64_t NextTag()
				{
					return next_tag_++;
				}
				uint64_t MaxTag() const
				{
					return next_tag_;
				}
			private:
				uint64_t next_tag_;
			};

			class RegisterExtent : public std::pair<int, int>
			{
			public:
				RegisterExtent(int start, int end) : std::pair<int, int>(start, end)
				{
					ASSERT(start <= end);
				}
			};

			class RegisterReference
			{
			public:
				using Extents = RegisterExtent;
				static RegisterReference::Extents UnlimitedExtent()
				{
					return {0, INT_MAX};
				}

				RegisterReference() : extents_(UnlimitedExtent()), precise_(false) {}
				RegisterReference(Extents extents, bool precise) : extents_(extents), precise_(precise) {}

				Extents GetExtents() const;
				size_t GetSize() const
				{
					return extents_.second - extents_.first + 1;
				}
				bool IsPrecise() const
				{
					return precise_;
				}
				bool IsUnlimited() const
				{
					// We don't care about where an extent starts, it's only
					// unlimited if it ends at infinity.
					return extents_.second == INT_MAX;
				}

			private:
				Extents extents_;
				bool precise_;
			};

			class RegisterAccess
			{
			public:

				static RegisterAccess Store(llvm::Instruction *llvm_inst, const RegisterReference &reg)
				{
					return RegisterAccess(llvm_inst, reg, true);
				}
				static RegisterAccess Load(llvm::Instruction *llvm_inst, const RegisterReference &reg)
				{
					return RegisterAccess(llvm_inst, reg, false);
				}

				bool IsStore() const
				{
					return is_store_;
				}
				const RegisterReference &GetRegRef() const
				{
					return reg_;
				}
				llvm::Instruction *GetInstruction() const
				{
					return llvm_inst_;
				}

			protected:
				RegisterAccess() = delete;
				RegisterAccess(llvm::Instruction *llvm_inst, const RegisterReference &reg, bool is_store) : is_store_(is_store), llvm_inst_(llvm_inst), reg_(reg) {}

			private:
				bool is_store_;

				llvm::Instruction *llvm_inst_;
				RegisterReference reg_;
			};

			class RegisterAccessDB
			{
			public:
				RegisterAccess &Get(uint64_t i)
				{
					return *storage_.at(i);
				}
				uint64_t GetReverse(RegisterAccess &ra) const
				{
					return reverse_storage_.at(&ra);
				}

				const RegisterAccess &Get(uint64_t i) const
				{
					return *storage_.at(i);
				}

				uint64_t Size() const
				{
					return storage_.size();
				}

				uint64_t Insert(const RegisterAccess &ra)
				{
					auto ptr = new RegisterAccess(ra);

					storage_.push_back(ptr);
					uint64_t index = storage_.size()-1;
					reverse_storage_[ptr] = index;
					return index;
				}

				~RegisterAccessDB()
				{
					for(auto i : storage_) {
						delete i;
					}
				}

			private:
				std::vector<RegisterAccess*> storage_;
				std::map<RegisterAccess*, uint64_t> reverse_storage_;
			};

			class RegisterDefinitions
			{
			public:

				RegisterDefinitions(RegisterAccessDB &radb) : radb_(&radb)
				{
					definitions_[RegisterReference::UnlimitedExtent()] = {};

					FailIfInvariantBroken();
				}

				void AddDefinition(RegisterAccess *access);

				wutils::vset<uint64_t> GetDefinitions(RegisterReference::Extents extents) const;

				static RegisterDefinitions MergeDefinitions(const std::vector<RegisterDefinitions*> &incoming_defs, RegisterAccessDB &radb);

				bool operator==(const RegisterDefinitions &other) const
				{
					return definitions_ == other.definitions_;
				}
				bool operator!=(const RegisterDefinitions &other) const
				{
					return !operator==(other);
				}

				const RegisterAccessDB &GetRADB() const
				{
					return *radb_;
				}
			private:
				void Flatten();

				void SetDefinitionForRange(RegisterReference::Extents extents, RegisterAccess *access);
				void EraseDefinitionsForRange(RegisterReference::Extents erase);
				void FailIfInvariantBroken() const;

				RegisterAccessDB *radb_;
				std::map<RegisterReference::Extents, std::vector<uint64_t>> definitions_;
			};


			class BlockInformation
			{
			public:
				using AccessList = std::vector<uint64_t>;

				BlockInformation(RegisterAccessDB &radb);

				BlockInformation(llvm::BasicBlock *block, RegisterAccessDB &radb);

				void ProcessBlock(llvm::BasicBlock *block);

				AccessList &GetAccesses()
				{
					return register_accesses_;
				}

				const AccessList &GetAccesses() const
				{
					return register_accesses_;
				}

			private:
				void ProcessLoad(llvm::Instruction *insn);
				void ProcessStore(llvm::Instruction *insn);
				void ProcessBreaker(llvm::Instruction *insn);
				void ProcessReturn(llvm::Instruction *insn);

				void AddRegisterAccess(const RegisterAccess &access)
				{
					register_accesses_.push_back(radb_.Insert(access));
				}

				RegisterAccessDB &radb_;
				AccessList register_accesses_;
			};


			class BlockDefinitions
			{
			public:
				BlockDefinitions(BlockInformation &block, RegisterAccessDB &radb);

				RegisterDefinitions PropagateDefinitions(RegisterDefinitions &incoming_defs, std::vector<uint64_t> &live_accesses);

				const std::vector<RegisterAccess*> &GetLocalLive() const
				{
					return locally_live_stores_;
				}

				const std::vector<RegisterReference> &GetIncomingLoads() const
				{
					return incoming_loaded_regs_;
				}

				const std::vector<RegisterAccess*> &GetOutgoingStores() const
				{
					return outgoing_stored_regs_;
				}

			private:
				std::vector<RegisterReference> ConvertToImpreciseReferences(const std::vector<bool> &bytes);

				void ProcessBlock(BlockInformation &block);

				RegisterAccessDB &radb_;
				std::vector<RegisterAccess *> locally_live_stores_;
				std::vector<RegisterReference> incoming_loaded_regs_;
				std::vector<RegisterAccess*> outgoing_stored_regs_;
			};

			class LLVMRegisterOptimisationPass : public llvm::FunctionPass
			{
			public:
				LLVMRegisterOptimisationPass();
				bool runOnFunction(llvm::Function & F) override;
				void getAnalysisUsage(llvm::AnalysisUsage & ) const override;

				std::vector<llvm::Instruction*> getDeadStores(llvm::Function &f);

				static char ID;

				using DefinitionSet = std::unordered_set<llvm::Instruction*>;

			private:
				bool ProcessFunction(llvm::Function &f, RegisterAccessDB &radb, std::vector<uint64_t> &all_definitions, std::vector<uint64_t> &live_definitions);
			};


		}
	}
}

static std::ostream &operator<<(std::ostream &str, const archsim::translate::translate_llvm::RegisterExtent &ref)
{
	str << "(" << ref.first << ", " << ref.second << ")";
	return str;
}
std::ostream &operator<<(std::ostream &str, const archsim::translate::translate_llvm::RegisterReference &ref);
std::ostream &operator<<(std::ostream &str, const archsim::translate::translate_llvm::RegisterAccess &access);
#endif
