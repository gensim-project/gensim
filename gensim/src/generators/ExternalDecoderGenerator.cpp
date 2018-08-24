/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/ArchDescription.h"
#include "generators/GenerationManager.h"
#include "generators/ExternalDecoderGenerator.h"

using namespace gensim::generator;

bool ExternalDecoder::Generate() const
{
	std::stringstream str;
	str << "#include <arch/" << Manager.GetArch().Name << "/" << GetProperty("Filename") << ">\n";
	str << "namespace gensim { namespace " << Manager.GetArch().Name << " { using Decode = " << GetProperty("Class") << "; } }";
	WriteOutputFile("decode.h", str);
	return true;
}

using gensim::generator::ExternalDecoder;
DEFINE_COMPONENT(ExternalDecoder, external_decoder);
COMPONENT_OPTION(external_decoder, Filename, "decode.h", "The filename to use within archsim (archsim/inc/arch/{arch}/{Filename})");
COMPONENT_OPTION(external_decoder, Class, "Decode", "The class to use within archsim for decoding");