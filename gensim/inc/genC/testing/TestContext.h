#ifndef GENC_TESTING_TESTCONTEXT_H_
#define GENC_TESTING_TESTCONTEXT_H_

#include <string>

namespace gensim {
	class DiagnosticContext;
	
    namespace genc {
		class GenCContext;
        namespace ssa {
            class SSAContext;
        }

        namespace testing {
            class TestContext {
            public:
                static GenCContext *GetTestContext(bool include_instruction, gensim::DiagnosticContext &diag_ctx);
                static ssa::SSAContext *CompileSource(GenCContext *ctx, const std::string &str);
            };
        }
    }
}

#endif
