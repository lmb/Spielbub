#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "context.h"

#include "cpu_ops.h"
#include "bit.h"
#include "logging.h"
#include "meta.h"

#define NUM(x) (sizeof (x) / sizeof (x)[0])

static size_t
opcode_to_bit(uint8_t opcode)
{
    return (opcode / 8) % 8;
}

void cpu_init(cpu_t* cpu)
{
    cpu->A   = 0x01;
    cpu->F   = 0xB0;
    cpu->BC  = 0x0013;
    cpu->DE  = 0x00D8;
    cpu->HL  = 0x014D;
    cpu->SP  = 0xFFFE;
    cpu->PC  = 0x0100;
    cpu->IME = true;
}

static uint8_t*
offset_to_reg(context_t* ctx, size_t offset)
{
    cpu_t* cpu = &ctx->cpu;

    switch (offset % 8)
    {
        case 0x0:
            return &cpu->B;
        case 0x1:
            return &cpu->C;
        case 0x2:
            return &cpu->D;
        case 0x3:
            return &cpu->E;
        case 0x4:
            return &cpu->H;
        case 0x5:
            return &cpu->L;
        case 0x6:
            abort();
        case 0x7:
            return &cpu->A;
    }

    // This will never happen, since offset is % 8
    return NULL;
}

uint8_t*
cpu_get_operand(context_t *ctx, uint8_t opcode) {
    return offset_to_reg(ctx, opcode);
}

uint8_t*
cpu_get_dest(context_t* ctx, uint8_t opcode)
{
    return offset_to_reg(ctx, opcode / 8);
}

/*
 * Request an interrupt. See interrupt_t for possible interrupts.
 */
void cpu_irq(context_t *ctx, interrupt_t i)
{
    if (i < I_MAX) {
        ctx->mem.io.IF |= 1 << i;
    }
}

/*
 * Handle pending interrupts. Called from run().
 */
void cpu_interrupts(context_t *ctx)
{
    const uint8_t interrupts = ctx->mem.io.IF & ctx->mem.io.IE;

    int i;
    for (i = 0; i < I_MAX; i++)
    {
        const uint8_t mask = 1 << i;

        if (interrupts & mask)
        {
            // Interrupt is requested and user
            // code is interested in it.
            ctx->cpu.IME    = false;
            ctx->cpu.halted = false;
            
            // TODO: Jumping to an ISR possibly consumes 5 cycles
            
            // Clear the interrupt from the interrupt
            // request register.
            ctx->mem.io.IF &= ~mask;

            // Push the current program counter onto the stack
            cpu_push(ctx, ctx->cpu.PC);

            // Select the appropriate ISR
            switch (i)
            {
                case I_VBLANK:    ctx->cpu.PC = 0x40; break;
                case I_LCDC:      ctx->cpu.PC = 0x48; break;
                case I_TIMER:     ctx->cpu.PC = 0x50; break;
                case I_SERIAL_IO: ctx->cpu.PC = 0x58; break;
                case I_JOYPAD:    ctx->cpu.PC = 0x60; break;
            }

            break;
        }
    }
}

/*
 * Executes one opcode at the current program counter.
 */
int cpu_run(context_t *ctx)
{
    memory_t *mem = &ctx->mem;
    cpu_t *cpu = &ctx->cpu;

    unsigned int opcode;

    opcode = mem->map[cpu->PC++];

    switch(opcode)
    {
        // NOP
        case 0x00:
            // Increment cycle counter
            break;

        // HALT
        case 0x76:
            // Energy saving mode
            // Emulator wakes up when
            // the next interrupt occurs.
            cpu->halted = true;
            break;

        // ___ Jumps _____________________________

        // JP nn
        case 0xC3:
            cpu_jump(ctx);
            break;

        // JP NZ, nn
        case 0xC2:
            if (!cpu_get_z(cpu)) cpu_jump(ctx);
            else cpu->PC += 2;
            break;

        // JP Z, nn
        case 0xCA:
            if (cpu_get_z(cpu)) cpu_jump(ctx);
            else cpu->PC += 2;
            break;

        // JP NC, nn
        case 0xD2:
            if (!cpu_get_c(cpu)) cpu_jump(ctx);
            else cpu->PC += 2;
            break;

        // JP Z, nn
        case 0xDA:
            if (cpu_get_c(cpu)) cpu_jump(ctx);
            else cpu->PC += 2;
            break;

        // JP (HL)
        case 0xE9:
            cpu->PC = cpu->HL;
            break;

        // Jump is calculated from instruction after the JR
        // JR n
        case 0x18:
            cpu_jump_rel(ctx);
            break;

        // JR NZ, n
        case 0x20:
            if (!cpu_get_z(cpu)) cpu_jump_rel(ctx);
            else cpu->PC++;
            break;

        // JR Z, n
        case 0x28:
            if (cpu_get_z(cpu)) cpu_jump_rel(ctx);
            else cpu->PC++;
            break;

        // JR NC, n
        case 0x30:
            if (!cpu_get_c(cpu)) cpu_jump_rel(ctx);
            else cpu->PC++;
            break;

        // JR C, n
        case 0x38:
            if (cpu_get_c(cpu)) cpu_jump_rel(ctx);
            else cpu->PC++;
            break;

        // CALL nn
        case 0xCD:
            cpu_call(ctx);
            break;

        // CALL NZ, nn
        case 0xC4:
            if (!cpu_get_z(cpu)) cpu_call(ctx);
            else cpu->PC += 2;
            break;

        // CALL Z, nn
        case 0xCC:
            if (cpu_get_z(cpu)) cpu_call(ctx);
            else cpu->PC += 2;
            break;

        // CALL NC, nn
        case 0xD4:
            if (!cpu_get_c(cpu)) cpu_call(ctx);
            else cpu->PC += 2;
            break;

        // CALL C, nn
        case 0xDC:
            if (cpu_get_c(cpu)) cpu_call(ctx);
            else cpu->PC += 2;
            break;

        // RST
        case 0xC7: cpu_restart(ctx, 0x00); break;
        case 0xCF: cpu_restart(ctx, 0x08); break;
        case 0xD7: cpu_restart(ctx, 0x10); break;
        case 0xDF: cpu_restart(ctx, 0x18); break;
        case 0xE7: cpu_restart(ctx, 0x20); break;
        case 0xEF: cpu_restart(ctx, 0x28); break;
        case 0xF7: cpu_restart(ctx, 0x30); break;
        case 0xFF: cpu_restart(ctx, 0x38); break;

        // RET
        case 0xC9:
            cpu_return(ctx);
            break;

        // RET NZ
        case 0xC0:
            if (!cpu_get_z(cpu)) cpu_return(ctx);
            break;

        // RET Z
        case 0xC8:
            if (cpu_get_z(cpu)) cpu_return(ctx);
            break;

        // RET NC
        case 0xD0:
            if (!cpu_get_c(cpu)) cpu_return(ctx);
            break;

        // RET C
        case 0xD8:
            if (cpu_get_c(cpu)) cpu_return(ctx);
            break;

        // RETI
        case 0xD9:
            cpu_return(ctx);
            cpu->IME = true;
            break;


        // ___ 8bit loads ________________________

        // LD B, r
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x47:
        // LD C, r
        case 0x48:
        case 0x49:
        case 0x4A:
        case 0x4B:
        case 0x4C:
        case 0x4D:
        case 0x4F:
        // LD D, r
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x57:
        // LD E, r
        case 0x58:
        case 0x59:
        case 0x5A:
        case 0x5B:
        case 0x5C:
        case 0x5D:
        case 0x5F:
        // LD H, r
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x67:
        // LD L, r
        case 0x68:
        case 0x69:
        case 0x6A:
        case 0x6B:
        case 0x6C:
        case 0x6D:
        case 0x6F:
        
        // LD A, r
        case 0x78:
        case 0x79:
        case 0x7A:
        case 0x7B:
        case 0x7C:
        case 0x7D:
        case 0x7F: {
            uint8_t* operand = cpu_get_operand(ctx, opcode);
            uint8_t* dest = cpu_get_dest(ctx, opcode);

            *dest = *operand;
            break;
        }
        
        // LD r, (HL)
        case 0x46:
        case 0x4E:
        case 0x56:
        case 0x5E:
        case 0x66:
        case 0x6E:
        case 0x7E: {
            uint8_t *dest = cpu_get_dest(ctx, opcode);
            *dest = mem_read(ctx, cpu->HL);
            break;
        }

        // LD (HL), r
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75: {
            uint8_t *operand = cpu_get_operand(ctx, opcode);
            mem_write(ctx, cpu->HL, *operand);
            break;
        }

        // LD r, n
        case 0x06:
        case 0x0E:
        case 0x16:
        case 0x1E:
        case 0x26:
        case 0x2E:
        case 0x3E: {
            uint8_t *dest = cpu_get_dest(ctx, opcode);

            *dest = mem_read(ctx, cpu->PC++);
            break;
        }

        // LD (HL), n
        case 0x36: mem_write(ctx, cpu->HL, mem_read(ctx, cpu->PC++)); break;

        // LD (HL), A
        case 0x77: mem_write(ctx, cpu->HL, cpu->A); break;

        // LD A, (BC)
        case 0x0A:
            cpu->A = mem_read(ctx, cpu->BC);
            break;

        // LD A, (DE)
        case 0x1A:
            cpu->A = mem_read(ctx, cpu->DE);
            break;

        // LD A, (nn)
        case 0xFA: {
            cpu->A = mem_read(ctx, mem_read16(ctx, cpu->PC));
            cpu->PC += 2;
            break;
        }

        // LDD A, (HL)
        case 0x3A:
            cpu->A = mem_read(ctx, cpu->HL--);
            break;

        // LD A, (0xFF00+C)
        case 0xF2:
            cpu->A = mem_read(ctx, 0xFF00 + cpu->C);
            break;

        // LDI A, (HL)
        case 0x2A:
            cpu->A = mem_read(ctx, cpu->HL++);
            break;

        // LDH A, (0xFF00 + n)
        case 0xF0:
            cpu->A = mem_read(ctx, 0xFF00 + mem_read(ctx, cpu->PC++));
            break;

        // --------------------------------------

        // LD (BC), A
        case 0x02:
            mem_write(ctx, cpu->BC, cpu->A);
            break;

        // LD (DE), A
        case 0x12:
            mem_write(ctx, cpu->DE, cpu->A);
            break;

        // LD (nn), A
        case 0xEA: {
            const uint16_t addr = mem_read16(ctx, cpu->PC);
            cpu->PC += 2;

            mem_write(ctx, addr, cpu->A);
            break;
        }

        // LD (0xFF00+C), A
        case 0xE2:
            mem_write(ctx, 0xFF00 + cpu->C, cpu->A);
            break;

        // LDD (HL), A
        case 0x32:
            mem_write(ctx, cpu->HL--, cpu->A);
            break;

        // LDI (HL), A
        case 0x22:
            mem_write(ctx, cpu->HL++, cpu->A);
            break;

        // LDH (0xFF00 + n), A
        case 0xE0:
            mem_write(ctx, 0xFF00 + mem->map[cpu->PC++], cpu->A);
            break;

        // ___ 16bit loads ________________________

        // LD BC, nn
        case 0x01:
            cpu->BC = mem_read16(ctx, cpu->PC);
            cpu->PC += 2;
            break;

        // LD DE, nn
        case 0x11:
            cpu->DE = mem_read16(ctx, cpu->PC);
            cpu->PC += 2;
            break;

        // LD HL, nn
        case 0x21:
            cpu->HL = mem_read16(ctx, cpu->PC);
            cpu->PC += 2;
            break;

        // LD SP, nn
        case 0x31:
            cpu->SP = mem_read16(ctx, cpu->PC);
            cpu->PC += 2;
            break;

        // LD SP, HL
        case 0xF9:
            cpu->SP = cpu->HL;
            break;

        // LD HL, SP+n
        case 0xF8: {
            int value = (int8_t)mem->map[cpu->PC++];

            cpu_set_z(cpu, false);
            cpu_set_n(cpu, false);
            cpu_set_h(cpu, (cpu->SP & 0xFF) + (value & 0xFF) > 0xFF);

            value += cpu->SP;
            cpu_set_c(cpu, value > 0xFFFF);

            cpu->HL = value;

            break;
        }

        // LD (nn), SP
        case 0x08: {
            const uint16_t addr = mem_read16(ctx, cpu->PC);
            cpu->PC += 2;

            mem_write16(ctx, addr, cpu->SP);
            break;
        }

        // PUSH BC
        case 0xC5:
            cpu_push(ctx, cpu->BC);
            break;

        // PUSH DE
        case 0xD5:
            cpu_push(ctx, cpu->DE);
            break;

        // PUSH HL
        case 0xE5:
            cpu_push(ctx, cpu->HL);
            break;

        // PUSH AF
        case 0xF5:
            cpu_push(ctx, cpu->AF);
            break;

        // POP BC
        case 0xC1:
            cpu->BC = cpu_pop(ctx);
            break;

        // POP DE
        case 0xD1:
            cpu->DE = cpu_pop(ctx);
            break;

        // POP HL
        case 0xE1:
            cpu->HL = cpu_pop(ctx);
            break;

        // POP AF
        case 0xF1:
            cpu->AF = cpu_pop(ctx);
            break;

        // ___ ALU ____________________________________________

        // --- 8bit -------------------------------------------

        // ADD A, r
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
        case 0x84:
        case 0x85:
        case 0x87: {
            uint8_t* operand = cpu_get_operand(ctx, opcode);

            cpu_add(cpu, *operand);
            break;
        }
        
        // ADD A, (HL)
        case 0x86: {
            uint8_t operand = mem_read(ctx, cpu->HL);
            cpu_add(cpu, operand);
            break;
        }

        // ADC A, r
        case 0x88:
        case 0x89:
        case 0x8A:
        case 0x8B:
        case 0x8C:
        case 0x8D:
        case 0x8F: {
            uint8_t* operand = cpu_get_operand(ctx, opcode);

            cpu_add_carry(cpu, *operand);
            break;
        }

        // ADC A, (HL)
        case 0x8E: {
            uint8_t operand = mem_read(ctx, cpu->HL);
            cpu_add_carry(cpu, operand);
            break;
        }
        // ADD A, n
        case 0xC6: cpu_add(cpu, mem->map[cpu->PC++]); break;

        // ADC A, n
        case 0xCE: cpu_add_carry(cpu, mem->map[cpu->PC++]); break;

        // SUB A, r
        case 0x90:
        case 0x91:
        case 0x92:
        case 0x93:
        case 0x94:
        case 0x95:
        case 0x97: {
            uint8_t* operand = cpu_get_operand(ctx, opcode);

            cpu_sub(cpu, *operand);
            break;
        }
        
        // SUB A, (HL)
        case 0x96: {
            uint8_t operand = mem_read(ctx, cpu->HL);
            cpu_sub(cpu, operand);
            break;
        }
    

        // SBC A, r
        case 0x98:
        case 0x99:
        case 0x9A:
        case 0x9B:
        case 0x9C:
        case 0x9D:
        case 0x9F: {
            uint8_t* operand = cpu_get_operand(ctx, opcode);

            cpu_sub_carry(cpu, *operand);
            break;
        }
        
        // SBC A, (HL)
        case 0x9E: {
            uint8_t operand = mem_read(ctx, cpu->HL);
            cpu_sub_carry(cpu, operand);
            break;
        }

        // SUB A, n
        case 0xD6: cpu_sub(cpu, mem->map[cpu->PC++]); break;

        // SBC A, n
        case 0xDE: cpu_sub_carry(cpu, mem->map[cpu->PC++]); break;

        // AND r
        case 0xA0:
        case 0xA1:
        case 0xA2:
        case 0xA3:
        case 0xA4:
        case 0xA5:
        case 0xA7: {
            uint8_t* operand = cpu_get_operand(ctx, opcode);

            cpu_and(cpu, *operand);
            break;
        }

        // AND (HL)
        case 0xA6: {
            uint8_t operand = mem_read(ctx, cpu->HL);
            cpu_and(cpu, operand);
            break;
        }


        // AND n
        case 0xE6: cpu_and(cpu, mem->map[cpu->PC++]); break;

        // XOR r
        case 0xA8:
        case 0xA9:
        case 0xAA:
        case 0xAB:
        case 0xAC:
        case 0xAD:
        case 0xAF: {
            uint8_t* operand = cpu_get_operand(ctx, opcode);

            cpu_xor(cpu, *operand);
            break;
        }
        
        // XOR (HL)
        case 0xAE: {
            uint8_t operand = mem_read(ctx, cpu->HL);
            cpu_xor(cpu, operand);
            break;
        }

        // XOR n
        case 0xEE: cpu_xor(cpu, mem->map[cpu->PC++]); break;

        // OR r
        case 0xB0:
        case 0xB1:
        case 0xB2:
        case 0xB3:
        case 0xB4:
        case 0xB5:
        case 0xB7: {
            uint8_t* operand = cpu_get_operand(ctx, opcode);

            cpu_or(cpu, *operand);
            break;
        }

        // OR (HL)
        case 0xB6: {
            uint8_t operand = mem_read(ctx, cpu->HL);
            cpu_or(cpu, operand);
            break;
        }

        // OR n
        case 0xF6: cpu_or(cpu, mem->map[cpu->PC++]); break;

        // CP r
        case 0xB8:
        case 0xB9:
        case 0xBA:
        case 0xBB:
        case 0xBC:
        case 0xBD:
        case 0xBF: {
            uint8_t* operand = cpu_get_operand(ctx, opcode);

            cpu_cp(cpu, *operand);
            break;
        }

        // CP (HL)
        case 0xBE: {
            uint8_t operand = mem_read(ctx, cpu->HL);
            cpu_cp(cpu, operand);
            break;
        }

        // CP n
        case 0xFE: cpu_cp(cpu, mem->map[cpu->PC++]); break;

        // INC r
        case 0x04:
        case 0x0C:
        case 0x14:
        case 0x1C:
        case 0x24:
        case 0x2C:
        case 0x3C: {
            uint8_t *dest = cpu_get_dest(ctx, opcode);

            cpu_inc(cpu, dest);
            break;
        }

        // INC (HL)
        case 0x34: {
            uint8_t operand = mem_read(ctx, cpu->HL);
            cpu_inc(cpu, &operand);
            mem_write(ctx, cpu->HL, operand);
            break;
        }

        // DEC r
        case 0x05:
        case 0x0D:
        case 0x15:
        case 0x1D:
        case 0x25:
        case 0x2D:
        case 0x3D: {
            uint8_t *dest = cpu_get_dest(ctx, opcode);

            cpu_dec(cpu, dest);
            break;
        }
        
        // DEC (HL)
        case 0x35: {
            uint8_t operand = mem_read(ctx, cpu->HL);
            cpu_dec(cpu, &operand);
            mem_write(ctx, cpu->HL, operand);
            break;
        }

        // --- 16bit ------------------------------

        // ADD HL, r
        case 0x09: cpu_add16(cpu, cpu->BC); break;
        case 0x19: cpu_add16(cpu, cpu->DE); break;
        case 0x29: cpu_add16(cpu, cpu->HL); break;
        case 0x39: cpu_add16(cpu, cpu->SP); break;

        // ADD SP, n
        case 0xE8: {
            int value = (int8_t)mem->map[cpu->PC++];

            cpu_set_z(cpu, false); // TODO: Is this flag correct? Not set in cpu_add16
            cpu_set_n(cpu, false);
            cpu_set_c(cpu, cpu->SP + value > 0xFFFF);
            cpu_set_h(cpu, (cpu->SP & 0xFF) + (value & 0xFF) > 0xFF);

            cpu->SP += value;
            break;
        }

        // INC r
        case 0x03: cpu->BC++; break;
        case 0x13: cpu->DE++; break;
        case 0x23: cpu->HL++; break;
        case 0x33: cpu->SP++; break;

        // DEC r
        case 0x0B: cpu->BC--; break;
        case 0x1B: cpu->DE--; break;
        case 0x2B: cpu->HL--; break;
        case 0x3B: cpu->SP--; break;

        // ___ Miscellaneous __________________________________

        // DAA
        case 0x27: cpu_daa(cpu); break;

        // CPL
        case 0x2F:
            cpu->A = ~cpu->A;
            cpu_set_n(cpu, true);
            cpu_set_h(cpu, true);
            break;

        // CCF
        case 0x3F:
            cpu_set_c(cpu, ~cpu_get_c(cpu));
            cpu_set_n(cpu, false);
            cpu_set_h(cpu, false);
            break;

        // SCF
        case 0x37:
            cpu_set_c(cpu, true);
            cpu_set_n(cpu, false);
            cpu_set_h(cpu, false);

        // DI
        case 0xF3:
            // Disable interrupts
            cpu->IME = false;
            break;

        // EI
        case 0xFB:
            // Enable interrupts
            cpu->IME = true;
            break;

        // RLCA
        case 0x07:
            cpu_rotate_l(cpu, &cpu->A);
            break;

        // RLA
        case 0x17:
            cpu_rotate_l_carry(cpu, &cpu->A);
            break;

        // RRCA
        case 0x0F:
            cpu_rotate_r(cpu, &cpu->A);
            break;

        // RRA
        case 0x1F:
            cpu_rotate_r_carry(cpu, &cpu->A);
            break;

        case 0xCB:
            // TODO: Should this go via mem_read?
            opcode = mem->map[cpu->PC++];
            switch (opcode)
            {
                // RLC r
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x07: {
                    uint8_t* operand = cpu_get_operand(ctx, opcode);

                    cpu_rotate_l(cpu, operand);
                    break;
                }

                // RLC (HL)
                case 0x06: {
                    uint8_t operand = mem_read(ctx, cpu->HL);
                    cpu_rotate_l(cpu, &operand);
                    mem_write(ctx, cpu->HL, operand);
                    break;
                }

                // RRC r
                case 0x08:
                case 0x09:
                case 0x0A:
                case 0x0B:
                case 0x0C:
                case 0x0D:
                case 0x0F: {
                    uint8_t* operand = cpu_get_operand(ctx, opcode);

                    cpu_rotate_r(cpu, operand);
                    break;
                }

                // RRC (HL)
                case 0x0E: {
                    uint8_t operand = mem_read(ctx, cpu->HL);
                    cpu_rotate_r(cpu, &operand);
                    mem_write(ctx, cpu->HL, operand);
                    break;
                }

                // RL r
                case 0x10:
                case 0x11:
                case 0x12:
                case 0x13:
                case 0x14:
                case 0x15:
                case 0x17: {
                    uint8_t* operand = cpu_get_operand(ctx, opcode);

                    cpu_rotate_l_carry(cpu, operand);
                    break;
                }

                // RL (HL)
                case 0x16: {
                    uint8_t operand = mem_read(ctx, cpu->HL);
                    cpu_rotate_l_carry(cpu, &operand);
                    mem_write(ctx, cpu->HL, operand);
                    break;
                }

                // RR r
                case 0x18:
                case 0x19:
                case 0x1A:
                case 0x1B:
                case 0x1C:
                case 0x1D:
                case 0x1F: {
                    uint8_t* operand = cpu_get_operand(ctx, opcode);

                    cpu_rotate_r_carry(cpu, operand);
                    break;
                }
                
                // RR (HL)
                case 0x1E: {
                    uint8_t operand = mem_read(ctx, cpu->HL);
                    cpu_rotate_r_carry(cpu, &operand);
                    mem_write(ctx, cpu->HL, operand);
                    break;
                }

                // SLA r
                case 0x20:
                case 0x21:
                case 0x22:
                case 0x23:
                case 0x24:
                case 0x25:
                case 0x27: {
                    uint8_t* operand = cpu_get_operand(ctx, opcode);

                    cpu_shift_l(cpu, operand);
                    break;
                }
                
                // SLA (HL)
                case 0x26: {
                    uint8_t operand = mem_read(ctx, cpu->HL);
                    cpu_shift_l(cpu, &operand);
                    mem_write(ctx, cpu->HL, operand);
                    break;
                }

                // SRA r
                case 0x28:
                case 0x29:
                case 0x2A:
                case 0x2B:
                case 0x2C:
                case 0x2D:
                case 0x2F: {
                    uint8_t* operand = cpu_get_operand(ctx, opcode);

                    cpu_shift_r_arithm(cpu, operand);
                    break;
                }

                // SRA (HL)
                case 0x2E: {
                    uint8_t operand = mem_read(ctx, cpu->HL);
                    cpu_shift_r_arithm(cpu, &operand);
                    mem_write(ctx, cpu->HL, operand);
                    break;
                }

                // SWAP r
                case 0x30:
                case 0x31:
                case 0x32:
                case 0x33:
                case 0x34:
                case 0x35:
                case 0x37: {
                    uint8_t* operand = cpu_get_operand(ctx, opcode);

                    cpu_swap(cpu, operand);
                    break;
                }
                
                // SWAP (HL)
                case 0x36: {
                    uint8_t operand = mem_read(ctx, cpu->HL);
                    cpu_swap(cpu, &operand);
                    mem_write(ctx, cpu->HL, operand);
                    break;
                }

                // SRL r
                case 0x38:
                case 0x39:
                case 0x3A:
                case 0x3B:
                case 0x3C:
                case 0x3D:
                case 0x3F: {
                    uint8_t* operand = cpu_get_operand(ctx, opcode);

                    cpu_shift_r_logic(cpu, operand);
                    break;
                }
                
                // SRL (HL)
                case 0x3E: {
                    uint8_t operand = mem_read(ctx, cpu->HL);
                    cpu_shift_r_logic(cpu, &operand);
                    mem_write(ctx, cpu->HL, operand);
                    break;
                }

                // BIT n,B
                case 0x40:
                case 0x48:
                case 0x50:
                case 0x58:
                case 0x60:
                case 0x68:
                case 0x70:
                case 0x78:
                // BIT n,C
                case 0x41:
                case 0x49:
                case 0x51:
                case 0x59:
                case 0x61:
                case 0x69:
                case 0x71:
                case 0x79:
                // BIT n,D
                case 0x42:
                case 0x4A:
                case 0x52:
                case 0x5A:
                case 0x62:
                case 0x6A:
                case 0x72:
                case 0x7A:
                // BIT n,E
                case 0x43:
                case 0x4B:
                case 0x53:
                case 0x5B:
                case 0x63:
                case 0x6B:
                case 0x73:
                case 0x7B:
                // BIT n,H
                case 0x44:
                case 0x4C:
                case 0x54:
                case 0x5C:
                case 0x64:
                case 0x6C:
                case 0x74:
                case 0x7C:
                // BIT n,L
                case 0x45:
                case 0x4D:
                case 0x55:
                case 0x5D:
                case 0x65:
                case 0x6D:
                case 0x75:
                case 0x7D:
                // BIT n,A
                case 0x47:
                case 0x4F:
                case 0x57:
                case 0x5F:
                case 0x67:
                case 0x6F:
                case 0x77:
                case 0x7F: {
                    uint8_t *operand = cpu_get_operand(ctx, opcode);
                    size_t bit = opcode_to_bit(opcode);

                    cpu_set_z(cpu, !bit_is_set(*operand, bit));
                    cpu_set_n(cpu, false);
                    cpu_set_h(cpu, true);

                    break;
                }
                
                // BIT n,(HL)
                case 0x46:
                case 0x4E:
                case 0x56:
                case 0x5E:
                case 0x66:
                case 0x6E:
                case 0x76:
                case 0x7E: {
                    uint8_t operand = mem_read(ctx, cpu->HL);
                    size_t bit = opcode_to_bit(opcode);

                    cpu_set_z(cpu, !bit_is_set(operand, bit));
                    cpu_set_n(cpu, false);
                    cpu_set_h(cpu, true);

                    break;
                }

                // RES n,B
                case 0x80:
                case 0x88:
                case 0x90:
                case 0x98:
                case 0xA0:
                case 0xA8:
                case 0xB0:
                case 0xB8:
                // RES n,C
                case 0x81:
                case 0x89:
                case 0x91:
                case 0x99:
                case 0xA1:
                case 0xA9:
                case 0xB1:
                case 0xB9:
                // RES n,D
                case 0x82:
                case 0x8A:
                case 0x92:
                case 0x9A:
                case 0xA2:
                case 0xAA:
                case 0xB2:
                case 0xBA:
                // RES n,E
                case 0x83:
                case 0x8B:
                case 0x93:
                case 0x9B:
                case 0xA3:
                case 0xAB:
                case 0xB3:
                case 0xBB:
                // RES n,H
                case 0x84:
                case 0x8C:
                case 0x94:
                case 0x9C:
                case 0xA4:
                case 0xAC:
                case 0xB4:
                case 0xBC:
                // RES n,L
                case 0x85:
                case 0x8D:
                case 0x95:
                case 0x9D:
                case 0xA5:
                case 0xAD:
                case 0xB5:
                case 0xBD:
                // RES n,A
                case 0x87:
                case 0x8F:
                case 0x97:
                case 0x9F:
                case 0xA7:
                case 0xAF:
                case 0xB7:
                case 0xBF: {
                    uint8_t* operand = cpu_get_operand(ctx, opcode);
                    size_t bit = opcode_to_bit(opcode);

                    *operand = bit_unset(*operand, bit);
                    break;
                }

                // RES n,(HL)
                case 0x86:
                case 0x8E:
                case 0x96:
                case 0x9E:
                case 0xA6:
                case 0xAE:
                case 0xB6:
                case 0xBE: {
                    uint8_t operand = mem_read(ctx, cpu->HL);
                    size_t bit = opcode_to_bit(opcode);

                    mem_write(ctx, cpu->HL, bit_unset(operand, bit));
                    break;
                }

                // SET n,B
                case 0xC0:
                case 0xC8:
                case 0xD0:
                case 0xD8:
                case 0xE0:
                case 0xE8:
                case 0xF0:
                case 0xF8:
                // SET n,C
                case 0xC1:
                case 0xC9:
                case 0xD1:
                case 0xD9:
                case 0xE1:
                case 0xE9:
                case 0xF1:
                case 0xF9:
                // SET n,D
                case 0xC2:
                case 0xCA:
                case 0xD2:
                case 0xDA:
                case 0xE2:
                case 0xEA:
                case 0xF2:
                case 0xFA:
                // SET n,E
                case 0xC3:
                case 0xCB:
                case 0xD3:
                case 0xDB:
                case 0xE3:
                case 0xEB:
                case 0xF3:
                case 0xFB:
                // SET n,H
                case 0xC4:
                case 0xCC:
                case 0xD4:
                case 0xDC:
                case 0xE4:
                case 0xEC:
                case 0xF4:
                case 0xFC:
                // SET n,L
                case 0xC5:
                case 0xCD:
                case 0xD5:
                case 0xDD:
                case 0xE5:
                case 0xED:
                case 0xF5:
                case 0xFD:
                // SET n,A
                case 0xC7:
                case 0xCF:
                case 0xD7:
                case 0xDF:
                case 0xE7:
                case 0xEF:
                case 0xF7:
                case 0xFF:
                {
                    uint8_t* operand = cpu_get_operand(ctx, opcode);
                    size_t bit = opcode_to_bit(opcode);

                    *operand = bit_set(*operand, bit);
                    break;
                }
                
                // SET n,(HL)
                case 0xC6:
                case 0xCE:
                case 0xD6:
                case 0xDE:
                case 0xE6:
                case 0xEE:
                case 0xF6:
                case 0xFE: {
                    uint8_t operand = mem_read(ctx, cpu->HL);
                    size_t bit = opcode_to_bit(opcode);

                    mem_write(ctx, cpu->HL, bit_set(operand, bit));
                    break;
                }

                default:
                    printf("FATAL: unhandled opcode 0xCB%02X at %X\n", opcode, cpu->PC);
                    cpu->halted = true;
                    return 0;
            }

            return ext_opcode_meta[opcode].cycles;

        default:
            printf("FATAL: unhandled opcode 0x%02X at %X\n", opcode, cpu->PC);
            ctx->cpu.halted = true;
            return 0;
    }

    return opcode_meta[opcode].cycles;
}
