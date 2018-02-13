/*
 * File:   IRValue.h
 * Author: s0457958
 *
 * Created on 15 September 2014, 14:50
 */

#ifndef IRVALUE_H
#define	IRVALUE_H

#include "define.h"

#include <string>
#include <ostream>

namespace archsim
{
	namespace ij
	{
		class IJIR;
		class IJIRBlock;

		namespace ir
		{
			class IRValue
			{
			public:
				enum AllocationClass {
					None,
					Register,
					Stack,
					Constant
				};

				IRValue(IJIR& ir, uint8_t size, std::string name = "");
				virtual ~IRValue();

				IJIR& GetIR() const
				{
					return ir;
				}

				uint8_t GetSize() const
				{
					return size;
				}
				std::string GetName() const
				{
					return name;
				}

				void SetScope(IJIRBlock *block)
				{
					scope = block;
				}
				IJIRBlock *GetScope() const
				{
					return scope;
				}

				inline bool IsAllocated() const
				{
					return alloc_class != None;
				}
				inline bool IsNotConstAllocated() const
				{
					return alloc_class == Register || alloc_class == Stack;
				}

				inline AllocationClass GetAllocationClass() const
				{
					return alloc_class;
				}
				inline uint64_t GetAllocationData() const
				{
					return alloc_data;
				}

				inline void Allocate(AllocationClass alloc_class, uint64_t data)
				{
					assert(!this->IsAllocated() && "IR Value is already allocated");

					this->alloc_class = alloc_class;
					this->alloc_data = data;
				}

				inline void EnsureAllocated() const
				{
					assert(this->IsAllocated() && "IR Value is not allocated");
				}

				virtual void Dump(std::ostream& stream) const;

			private:
				IJIR& ir;
				IJIRBlock *scope;
				uint8_t size;
				std::string name;
				AllocationClass alloc_class;
				uint64_t alloc_data;
			};

			class IRConstant : public IRValue
			{
			public:
				IRConstant(IJIR& ir, uint8_t size, uint64_t value, std::string name = "");

				uint64_t GetValue() const
				{
					return value;
				}

				void Dump(std::ostream& stream) const override;

			private:
				uint64_t value;
			};

			class IRVariable : public IRValue
			{
			public:
				IRVariable(IJIR& ir, uint8_t size, std::string name = "");

				void Dump(std::ostream& stream) const override;
			};
		}
	}
}

#endif	/* IRVALUE_H */

