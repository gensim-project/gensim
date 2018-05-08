/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   IRAttributes.h
 * Author: harry
 *
 * Created on 27 July 2017, 15:34
 */

#ifndef IRATTRIBUTES_H
#define IRATTRIBUTES_H

namespace gensim
{
	namespace genc
	{
		namespace ActionAttribute
		{
			enum EActionAttribute {
				NoInline,
				Helper,
				External,
				Export
			};
		}
	}
}

#endif /* IRATTRIBUTES_H */

