/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   AntlrWrapper.h
 * Author: harry
 *
 * Created on 27 October 2017, 15:34
 */

#ifndef ANTLRWRAPPER_H
#define ANTLRWRAPPER_H

struct ANTLR3_BASE_TREE_struct;
typedef struct ANTLR3_BASE_TREE_struct *pANTLR3_BASE_TREE;

namespace gensim
{
	class AntlrTreeWrapper
	{
	public:
		AntlrTreeWrapper(pANTLR3_BASE_TREE tree) : tree_(tree) {}
		pANTLR3_BASE_TREE Get()
		{
			return tree_;
		}

	private:
		pANTLR3_BASE_TREE tree_;
	};
}

#endif /* ANTLRWRAPPER_H */

