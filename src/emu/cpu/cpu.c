#include <nessan-gtr/parser.h>
#include "cpu.h"
#include "cpu_internal.h"
#include "mmu.h"

static struct cpu cpu;

/* Instruction handlers. */

void SET(word_t operand) {
	cpu.regs.p.bits.irq = SR_IRQ_DISABLED;
	++cpu.regs.pc;
}

void STA_Abs(word_t operand) {
	mem_write(operand, cpu.regs.a);
	cpu.regs.pc += 3;
}

void LDX_Imm(word_t operand) {
	cpu.regs.x += operand & 0xff;
	cpu.regs.pc += 2;
}

void LDA_Imm(word_t operand) {
	cpu.regs.a = operand & 0xff;
	cpu.regs.pc += 2;
}

void CLD(word_t operand) {
	++cpu.regs.pc;
}

static instruction_logic opcode_to_insn_logic[] = {
	[0x78] = SET,
	[0x8D] = STA_Abs,
	[0xA2] = LDX_Imm,
	[0xA9] = LDA_Imm,
	[0xD8] = CLD,
};


int execution_loop(void) {
	struct instruction_description desc;
	word_t first_insn_addr;
	unsigned char insn_data[MAX_INSN_SIZE];

	/* Jump to address in reset vector and begin executing. */
	first_insn_addr = mem_read(ADDR_RESET_VEC);
	cpu.regs.pc = first_insn_addr;
	insn_data[0] = (uint8_t)mem_read(cpu.regs.pc);
	*(word_t *)(insn_data + 1) = mem_read(cpu.regs.pc + 1);

	while (parser_get_instruction_description((unsigned char *)&insn_data, MAX_INSN_SIZE, &desc) == 0) {
		opcode_to_insn_logic[desc.opcode](desc.operand);
		insn_data[0] = (uint8_t)mem_read(cpu.regs.pc);
		*(word_t *)(insn_data + 1) = mem_read(cpu.regs.pc + 1);
	}

	return -1;
}

int cpu_power_on(enum memory_mode mem_mode) {
	int i;
	
	mmu_configure(mem_mode);

	cpu.regs.a = 0;
	cpu.regs.x = 0;
	cpu.regs.y = 0;
	cpu.regs.s = 0xfd;
	cpu.regs.p.bits.irq = SR_IRQ_DISABLED;
	cpu.regs.p.bits.carry = SR_CARRY_ON;
	cpu.regs.p.bits.negative = SR_NEGATIVE_ON;
	mem_write(REG_FRAME_COUNTER_CTL, 0); /* Enable frame irq. */
	mem_write(REG_SND_CHN, 0); /* Disable sound channels. */

	for (i = REG_SQ1_VOL; i <= REG_NOISE_HI; ++i) {
		mem_write(i, 0);
	}

	if (execution_loop())
		return -1;

	return 0;
}

int cpu_reset(void) {
	cpu.regs.s -= 3;
	cpu.regs.p.bits.irq = SR_IRQ_DISABLED;
	mem_write(REG_SND_CHN, 0); /* Disable sound channels. */
}
