/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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

