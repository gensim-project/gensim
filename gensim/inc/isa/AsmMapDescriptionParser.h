/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AsmMapDescriptionParser.h
 * Author: harry
 *
 * Created on 12 April 2018, 15:15
 */

#ifndef ASMMAPDESCRIPTIONPARSER_H
#define ASMMAPDESCRIPTIONPARSER_H

#include "isa/AsmMapDescription.h"

namespace gensim {
	namespace isa {
		class AsmMapDescriptionParser {
		public:
			static AsmMapDescription Parse(void *pnode);
		};
	}
}

#endif /* ASMMAPDESCRIPTIONPARSER_H */

