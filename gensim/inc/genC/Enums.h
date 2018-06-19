/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

namespace gensim
{
	namespace genc
	{

		enum SymbolType {
			Symbol_Local,
			Symbol_Temporary,
			Symbol_InlineParam,
			Symbol_Parameter,
			Symbol_Constant
		};

		enum HelperScope {
			PublicScope,
			PrivateScope,
			InternalScope
		};

		namespace IRConstness
		{

			enum IRConstness {
				always_const,
				sometimes_const,
				never_const
			};
		}
	}
}
