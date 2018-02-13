/*
 * File:   IJGenerator.h
 * Author: s0457958
 *
 * Created on 15 September 2014, 12:44
 */

#ifndef IJGENERATOR_H
#define	IJGENERATOR_H

#include "generators/GenerationManager.h"
#include "Util.h"

#include <vector>

namespace gensim
{
	namespace generator
	{
		namespace ij
		{
			class IJGenerator : public GenerationComponent
			{
			public:
				IJGenerator(GenerationManager &man);

				bool Generate() const override;
				const std::vector<std::string> GetSources() const override
				{
					return sources;
				}

				std::string GetFunction() const override
				{
					return "ij";
				}

			private:
				bool GenerateHeader() const;
				bool GenerateSource() const;

				mutable util::cppformatstream hdr_stream;
				mutable util::cppformatstream src_stream;

				mutable std::vector<std::string> sources;
			};
		}
	}
}

#endif	/* IJGENERATOR_H */

