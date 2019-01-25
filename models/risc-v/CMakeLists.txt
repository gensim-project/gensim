
define_model(
	model-riscv32
	riscv32
	riscv32/riscv32.ac

	riscv32/riscv32.ac
	common/behaviours.riscv
	common/execute.riscv
	common/execute.rv32-a
	common/execute.rv32-f
	common/execute.rv32-d
	common/execute.rv32-m
	common/riscv_isa.ac
	common/rv32-a.ac
	common/rv32-f.ac
	common/rv32-d.ac
	common/rv32-m.ac
	common/rv-core.ac
)


define_model(
	model-riscv64
	riscv64
	riscv64/riscv64.ac

	riscv64/riscv64.ac
	riscv64/riscv64_isa.ac
	riscv64/execute.rv64i
	riscv64/rv64-i.ac
	common/behaviours.riscv
	common/execute.riscv
	common/execute.rv32-a
	common/execute.rv64-a
	common/execute.rv32-f
	common/execute.rv32-d
	common/execute.rv64-d
	common/execute.rv32-m
	common/riscv_isa.ac
	common/rv64-a.ac
	common/rv32-a.ac
	common/rv32-f.ac
	common/rv32-d.ac
	common/rv64-d.ac
	common/rv32-m.ac
	common/rv-core.ac
)
