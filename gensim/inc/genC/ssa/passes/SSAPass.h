/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SSAPass.h
 * Author: harry
 *
 * Created on 18 April 2017, 12:46
 */

#ifndef SSAPASS_H
#define SSAPASS_H

#include "ComponentManager.h"

#include <vector>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAContext;
			class SSAFormAction;
			class SSAPass;

			class SSAPassManager
			{
			public:
				SSAPassManager();

				void AddPass(const SSAPass *pass);
				void AddDebugPass(const SSAPass *pass);
				bool Run(SSAContext &ctx);
				bool Run(SSAFormAction &action);

				void SetMultirunEach(bool b)
				{
					multirun_each_ = b;
				}
				void SetMultirunAll(bool b)
				{
					multirun_all_ = b;
				}

			private:
				void RunDebugPasses(SSAFormAction &action);

				std::vector<const SSAPass*> passes_;
				std::vector<const SSAPass*> debug_passes_;

				bool multirun_each_;
				bool multirun_all_;
			};

			class SSAPass
			{
			public:
				SSAPass() = default;
				virtual ~SSAPass();

				virtual bool Run(SSAFormAction &action) const = 0;
			};

			class SSAPassDB
			{
			public:
				static const SSAPass *Get(const std::string &passname);

			private:
				static SSAPassDB &GetSingleton();

				SSAPass *GetPass(const std::string &passname);
				std::map<std::string, SSAPass*> passes_;

				static SSAPassDB *singleton_;
			};
		}
	}
}

#endif /* SSAPASS_H */

