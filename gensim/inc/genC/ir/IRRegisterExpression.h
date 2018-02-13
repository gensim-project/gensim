/*
 * IRRegisterAction.h
 *
 *  Created on: 14 May 2015
 *      Author: harry
 */

#ifndef INC_GENC_IR_IRREGISTEREXPRESSION_H_
#define INC_GENC_IR_IRREGISTEREXPRESSION_H_

#include <map>
#include <string>

#include "genC/Enums.h"
#include "genC/ir/IRType.h"
#include "genC/ir/IRScope.h"

namespace gensim
{
	namespace genc
	{

		class IRRegisterSlotReadExpression : public IRCallExpression
		{
		public:
			IRRegisterSlotReadExpression(IRScope &scope);
			virtual ~IRRegisterSlotReadExpression();

			virtual bool Resolve(GenCContext &Context) override;
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const override;
			virtual const IRType EvaluateType() override;

			IRType GetRegisterType() const;

		private:
			uint32_t slot_id;
		};
		class IRRegisterSlotWriteExpression : public IRCallExpression
		{
		public:
			IRRegisterSlotWriteExpression(IRScope &scope);
			virtual ~IRRegisterSlotWriteExpression();

			virtual bool Resolve(GenCContext &Context) override;
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const override;
			virtual const IRType EvaluateType() override;

			IRType GetRegisterType() const;

		private:
			uint32_t slot_id;
			IRExpression *value;
		};


		class IRRegisterBankReadExpression : public IRCallExpression
		{
		public:
			IRRegisterBankReadExpression(IRScope &scope);
			virtual ~IRRegisterBankReadExpression();

			virtual bool Resolve(GenCContext &Context) override;
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const override;
			virtual const IRType EvaluateType() override;

			IRType GetRegisterType() const;
		private:
			uint32_t bank_idx;
			IRExpression *reg_slot_expression;
		};
		class IRRegisterBankWriteExpression : public IRCallExpression
		{
		public:
			IRRegisterBankWriteExpression(IRScope &scope);
			virtual ~IRRegisterBankWriteExpression();

			virtual bool Resolve(GenCContext &Context) override;
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const override;
			virtual const IRType EvaluateType() override;

			IRType GetRegisterType() const;
		private:
			uint32_t bank_idx;
			IRExpression *reg_slot_expression;
			IRExpression *value_expression;
		};

	}
}




#endif /* INC_GENC_IR_IRREGISTEREXPRESSION_H_ */
