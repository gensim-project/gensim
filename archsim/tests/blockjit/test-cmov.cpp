
#include "ArchSimBlockJITTest.h"

using namespace captive::shared;
using namespace captive::arch::jit;

class ArchSimBlockJITCMovTest : public ArchSimBlockJITTest {
public:
	template<typename P, typename Y, typename N> void build_skeleton() {
		IRRegId predicate = Allocate(P(), 1);
		IRRegId yes = Allocate(Y(), 4);
		IRRegId no  = Allocate(N(), 4);
		
		Builder().ldreg(IROperand::const32(0), IROperand::vreg(predicate, 1));
		Builder().ldreg(IROperand::const32(4), IROperand::vreg(yes, 4));
		Builder().ldreg(IROperand::const32(8), IROperand::vreg(no, 4));
		
		Builder().cmov(IROperand::vreg(predicate, 1), IROperand::vreg(yes, 4), IROperand::vreg(no, 4));
		Builder().streg(IROperand::vreg(no, 4), IROperand::const32(0));
		Builder().ret();
	}
	
	template<class PredGen, class YGen, class NGen> void build_and_test() {
		transforms::AllocationWriterTransform awt(allocations_);
		awt.Apply(tc_);

		CompileResult cr (true, stack_frame_, archsim::util::vbitset(8, 0xff));

		auto fn = Lower(cr);
		ASSERT_NE(nullptr, fn);

		std::vector<char> regfile_mock(128, 0);
		uint32_t *regfile = (uint32_t*)regfile_mock.data();
		for(int i = 0; i < kIterations; ++i) {

			uint32_t p = PredGen(), x = YGen(), y = NGen();
			regfile[0] = p;
			regfile[1] = x;
			regfile[2] = y;

			fn(regfile_mock.data(), nullptr);

			ASSERT_EQ(regfile[2], p ? x : y);
		}
	}
	
};

TEST_F(ArchSimBlockJITCMovTest, Test1) {
	build_skeleton<RegTag, RegTag, RegTag>();
	build_and_test<RandValueGen<1>, RandValueGen<>, RandValueGen<>>();
}