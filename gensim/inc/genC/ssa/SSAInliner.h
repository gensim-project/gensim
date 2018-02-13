/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   SSAInliner.h
 * Author: harry
 *
 * Created on 08 May 2017, 17:09
 */

#ifndef SSAINLINER_H
#define SSAINLINER_H

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{

			class SSACallStatement;
			class SSACloneContext;

			class SSAInliner
			{
			public:
				void Inline(SSACallStatement *call_site) const;

			private:
				void InlineParameters(SSACallStatement *call_site, SSACloneContext &ctx) const;
				void FlattenReferences(SSAFormAction *action) const;
				void InsertTemporaries(SSABlock *call_block, SSABlock *post_block) const;
				void RemoveEmptyBlocks(SSAFormAction *action) const;
			};

		}
	}
}

#endif /* SSAINLINER_H */

