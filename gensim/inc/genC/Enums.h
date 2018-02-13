/*
 * genC/Enums.h
 *
 * GenSim
 * Copyright (C) University of Edinburgh.  All Rights Reserved.
 *
 * Harry Wagstaff <hwagstaf@inf.ed.ac.uk>
 * Tom Spink <tspink@inf.ed.ac.uk>
 */
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
