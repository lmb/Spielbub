#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "context.h"

#include "cpu_ops.h"
#include "ioregs.h"
#include "logging.h"
#include "meta.h"

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

#if defined(DEBUG)
    if (cpu->trace != NULL) {
        cb_reset(cpu->trace);
    } else {
        cpu->trace = cb_init(20, 2);
    }
#endif
}

/*
 * Request an interrupt. See interrupt_t for possible interrupts.
 */
void cpu_irq(context_t *ctx, interrupt_t i)
{
    // Set bit in IF
    ctx->mem.map[R_IF] |= 1 << i;
}

/*
 * Handle pending interrupts. Called from run().
 */
void cpu_interrupts(context_t *ctx)
{
    // Interrupt requests
    const uint8_t r_if = ctx->mem.map[R_IF];
    // Interrupt enable
    const uint8_t r_ie = ctx->mem.map[R_IE];

    int i;
    for (i = 0; i < 5; i++)
    {
        const uint8_t mask = 1 << i;

        if ((r_if & mask) && (r_ie & mask))
        {
            // Interrupt is requested and user
            // code is interested in it.
            ctx->cpu.IME    = false;
            ctx->cpu.halted = false;
            
            // TODO: Jumping to an ISR possibly consumes 5 cycles
            
            // Clear the interrupt from the interrupt
            // request register.
            ctx->mem.map[R_IF] &= ~mask;

            // Push the current program counter onto the stack
            _push(ctx, ctx->cpu.PC);

            // Select the appropriate ISR
            switch (i)
            {
                case I_VBLANK:    ctx->cpu.PC = 0x40; break;
                case I_LCDC:      ctx->cpu.PC = 0x48; break;
                case I_TIMER:     ctx->cpu.PC = 0x50; break;
                case I_SERIAL_IO: ctx->cpu.PC = 0x58; break;
                case I_JOYPAD:    ctx->cpu.PC = 0x60; break;
            }

            return;
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

    unsigned int value = 0;
    unsigned int opcode;
    unsigned int cycles = 0;

#if defined(DEBUG)
    cb_write(cpu->trace, &cpu->PC);
#endif

    if (pl_check(&(cpu->breakpoints), cpu->PC))
    {
        ctx->state = STOPPED;
        return cycles;
    }
    
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
            _jump(ctx);
            break;

        // JP NZ, nn
        case 0xC2:
            if (!cpu_get_z(cpu)) _jump(ctx);
            else cpu->PC += 2;
            break;

        // JP Z, nn
        case 0xCA:
            if (cpu_get_z(cpu)) _jump(ctx);
            else cpu->PC += 2;
            break;

        // JP NC, nn
        case 0xD2:
            if (!cpu_get_c(cpu)) _jump(ctx);
            else cpu->PC += 2;
            break;

        // JP Z, nn
        case 0xDA:
            if (cpu_get_c(cpu)) _jump(ctx);
            else cpu->PC += 2;
            break;

        // JP (HL)
        case 0xE9:
            cpu->PC = cpu->HL;
            break;

        // Jump is calculated from instruction after the JR
        // JR n
        case 0x18:
            _jump_rel(ctx);
            break;

        // JR NZ, n
        case 0x20:
            if (!cpu_get_z(cpu)) _jump_rel(ctx);
            else cpu->PC++;
            break;

        // JR Z, n
        case 0x28:
            if (cpu_get_z(cpu)) _jump_rel(ctx);
            else cpu->PC++;
            break;

        // JR NC, n
        case 0x30:
            if (!cpu_get_c(cpu)) _jump_rel(ctx);
            else cpu->PC++;
            break;

        // JR C, n
        case 0x38:
            if (cpu_get_c(cpu)) _jump_rel(ctx);
            else cpu->PC++;
            break;

        // CALL nn
        case 0xCD:
            _call(ctx);
            break;

        // CALL NZ, nn
        case 0xC4:
            if (!cpu_get_z(cpu)) _call(ctx);
            else cpu->PC += 2;
            break;

        // CALL Z, nn
        case 0xCC:
            if (cpu_get_z(cpu)) _call(ctx);
            else cpu->PC += 2;
            break;

        // CALL NC, nn
        case 0xD4:
            if (!cpu_get_c(cpu)) _call(ctx);
            else cpu->PC += 2;
            break;

        // CALL C, nn
        case 0xDC:
            if (cpu_get_c(cpu)) _call(ctx);
            else cpu->PC += 2;
            break;

        // RST
        case 0xC7: _restart(ctx, 0x00); break;
        case 0xCF: _restart(ctx, 0x08); break;
        case 0xD7: _restart(ctx, 0x10); break;
        case 0xDF: _restart(ctx, 0x18); break;
        case 0xE7: _restart(ctx, 0x20); break;
        case 0xEF: _restart(ctx, 0x28); break;
        case 0xF7: _restart(ctx, 0x30); break;
        case 0xFF: _restart(ctx, 0x38); break;

        // RET
        case 0xC9:
            _return(ctx);
            break;

        // RET NZ
        case 0xC0:
            if (!cpu_get_z(cpu)) _return(ctx);
            break;

        // RET Z
        case 0xC8:
            if (cpu_get_z(cpu)) _return(ctx);
            break;

        // RET NC
        case 0xD0:
            if (!cpu_get_c(cpu)) _return(ctx);
            break;

        // RET C
        case 0xD8:
            if (cpu_get_c(cpu)) _return(ctx);
            break;

        // RETI
        case 0xD9:
            _return(ctx);
            cpu->IME = true;
            break;


        // ___ 8bit loads ________________________

        // LD B, r
        case 0x40: cpu->B = cpu->B; break;
        case 0x41: cpu->B = cpu->C; break;
        case 0x42: cpu->B = cpu->D; break;
        case 0x43: cpu->B = cpu->E; break;
        case 0x44: cpu->B = cpu->H; break;
        case 0x45: cpu->B = cpu->L; break;
        case 0x46: cpu->B = mem->map[cpu->HL]; break;
        case 0x47: cpu->B = cpu->A; break;

        // LD B, n
        case 0x06: cpu->B = mem->map[cpu->PC++]; break;

        // LD C, r
        case 0x48: cpu->C = cpu->B; break;
        case 0x49: cpu->C = cpu->C; break;
        case 0x4A: cpu->C = cpu->D; break;
        case 0x4B: cpu->C = cpu->E; break;
        case 0x4C: cpu->C = cpu->H; break;
        case 0x4D: cpu->C = cpu->L; break;
        case 0x4E: cpu->C = mem->map[cpu->HL]; break;
        case 0x4F: cpu->C = cpu->A; break;

        // LD C, n
        case 0x0E: cpu->C = mem->map[cpu->PC++]; break;

        // LD D, r
        case 0x50: cpu->D = cpu->B; break;
        case 0x51: cpu->D = cpu->C; break;
        case 0x52: cpu->D = cpu->D; break;
        case 0x53: cpu->D = cpu->E; break;
        case 0x54: cpu->D = cpu->H; break;
        case 0x55: cpu->D = cpu->L; break;
        case 0x56: cpu->D = mem->map[cpu->HL]; break;
        case 0x57: cpu->D = cpu->A; break;

        // LD D, n
        case 0x16: cpu->D = mem->map[cpu->PC++]; break;

        // LD E, r
        case 0x58: cpu->E = cpu->B; break;
        case 0x59: cpu->E = cpu->C; break;
        case 0x5A: cpu->E = cpu->D; break;
        case 0x5B: cpu->E = cpu->E; break;
        case 0x5C: cpu->E = cpu->H; break;
        case 0x5D: cpu->E = cpu->L; break;
        case 0x5E: cpu->E = mem->map[cpu->HL]; break;
        case 0x5F: cpu->E = cpu->A; break;

        // LD E, n
        case 0x1E: cpu->E = mem->map[cpu->PC++]; break;

        // LD H, r
        case 0x60: cpu->H = cpu->B; break;
        case 0x61: cpu->H = cpu->C; break;
        case 0x62: cpu->H = cpu->D; break;
        case 0x63: cpu->H = cpu->E; break;
        case 0x64: cpu->H = cpu->H; break;
        case 0x65: cpu->H = cpu->L; break;
        case 0x66: cpu->H = mem->map[cpu->HL]; break;
        case 0x67: cpu->H = cpu->A; break;

        // LD H, n
        case 0x26: cpu->H = mem->map[cpu->PC++]; break;

        // LD L, r
        case 0x68: cpu->L = cpu->B; break;
        case 0x69: cpu->L = cpu->C; break;
        case 0x6A: cpu->L = cpu->D; break;
        case 0x6B: cpu->L = cpu->E; break;
        case 0x6C: cpu->L = cpu->H; break;
        case 0x6D: cpu->L = cpu->L; break;
        case 0x6E: cpu->L = mem->map[cpu->HL]; break;
        case 0x6F: cpu->L = cpu->A; break;

        // LD L, n
        case 0x2E: cpu->L = mem->map[cpu->PC++]; break;

        // LD (HL), r
        case 0x70: mem_write(&ctx->mem, cpu->HL, cpu->B); break;
        case 0x71: mem_write(&ctx->mem, cpu->HL, cpu->C); break;
        case 0x72: mem_write(&ctx->mem, cpu->HL, cpu->D); break;
        case 0x73: mem_write(&ctx->mem, cpu->HL, cpu->E); break;
        case 0x74: mem_write(&ctx->mem, cpu->HL, cpu->H); break;
        case 0x75: mem_write(&ctx->mem, cpu->HL, cpu->L); break;
        // 0x76: HALT
        case 0x77: mem_write(&ctx->mem, cpu->HL, cpu->A); break;

        // LD (HL), n
        case 0x36: mem_write(&ctx->mem, cpu->HL, mem->map[cpu->PC++]); break;

        // LD A, r
        case 0x78: cpu->A = cpu->B; break;
        case 0x79: cpu->A = cpu->C; break;
        case 0x7A: cpu->A = cpu->D; break;
        case 0x7B: cpu->A = cpu->E; break;
        case 0x7C: cpu->A = cpu->H; break;
        case 0x7D: cpu->A = cpu->L; break;
        case 0x7E: cpu->A = mem->map[cpu->HL]; break;
        case 0x7F: cpu->A = cpu->A; break;

        // LD A, n
        case 0x3E: cpu->A = mem->map[cpu->PC++]; break;

        // LD A, (BC)
        case 0x0A:
            cpu->A = mem->map[cpu->BC];
            break;

        // LD A, (DE)
        case 0x1A:
            cpu->A = mem->map[cpu->DE];
            break;

        // LD A, (nn)
        case 0xFA: {
            cpu->A = mem->map[mem_read16(mem, cpu->PC)];
            cpu->PC += 2;
            break;
        }

        // LDD A, (HL)
        case 0x3A:
            cpu->A = mem->map[cpu->HL--];
            break;

        // LD A, (0xFF00+C)
        case 0xF2:
            cpu->A = mem->map[0xFF00 + cpu->C];
            break;

        // LDI A, (HL)
        case 0x2A:
            cpu->A = mem->map[cpu->HL++];
            break;

        // LDH A, (0xFF00 + n)
        case 0xF0:
            cpu->A = mem->map[0xFF00 + mem->map[cpu->PC++]];
            break;

        // --------------------------------------

        // LD (BC), A
        case 0x02:
            mem_write(&ctx->mem, cpu->BC, cpu->A);
            break;

        // LD (DE), A
        case 0x12:
            mem_write(&ctx->mem, cpu->DE, cpu->A);
            break;

        // LD (nn), A
        case 0xEA: {
            const uint16_t addr = mem_read16(mem, cpu->PC);
            cpu->PC += 2;

            mem_write(&ctx->mem, addr, cpu->A);
            break;
        }

        // LD (0xFF00+C), A
        case 0xE2:
            mem_write(&ctx->mem, 0xFF00 + cpu->C, cpu->A);
            break;

        // LDD (HL), A
        case 0x32:
            mem_write(&ctx->mem, cpu->HL--, cpu->A);
            break;

        // LDI (HL), A
        case 0x22:
            mem_write(&ctx->mem, cpu->HL++, cpu->A);
            break;

        // LDH (0xFF00 + n), A
        case 0xE0:
            mem_write(&ctx->mem, 0xFF00 + mem->map[cpu->PC++], cpu->A);
            break;

        // ___ 16bit loads ________________________

        // LD BC, nn
        case 0x01:
            cpu->BC = mem_read16(mem, cpu->PC);
            cpu->PC += 2;
            break;

        // LD DE, nn
        case 0x11:
            cpu->DE = mem_read16(mem, cpu->PC);
            cpu->PC += 2;
            break;

        // LD HL, nn
        case 0x21:
            cpu->HL = mem_read16(mem, cpu->PC);
            cpu->PC += 2;
            break;

        // LD SP, nn
        case 0x31:
            cpu->SP = mem_read16(mem, cpu->PC);
            cpu->PC += 2;
            break;

        // LD SP, HL
        case 0xF9:
            cpu->SP = cpu->HL;
            break;

        // LD HL, SP+n
        case 0xF8:
            value = (int8_t)mem->map[cpu->PC++];

            cpu_set_z(cpu, 0);
            cpu_set_n(cpu, 0);
            cpu_set_h(cpu, (cpu->SP & 0xF) + (value & 0xF) > 0xF);

            value += cpu->SP;
            cpu_set_c(cpu, value > 0xFFFF);

            cpu->HL = (uint16_t)value; // Check for overflow?

            break;

        // LD (nn), SP
        case 0x08: {
            const uint16_t addr = mem_read16(mem, cpu->PC);
            cpu->PC += 2;

            mem_write16(mem, addr, cpu->SP);
            break;
        }

        // PUSH BC
        case 0xC5:
            _push(ctx, cpu->BC);
            break;

        // PUSH DE
        case 0xD5:
            _push(ctx, cpu->DE);
            break;

        // PUSH HL
        case 0xE5:
            _push(ctx, cpu->HL);
            break;

        // PUSH AF
        case 0xF5:
            _push(ctx, cpu->AF);
            break;

        // POP BC
        case 0xC1:
            cpu->BC = _pop(ctx);
            break;

        // POP DE
        case 0xD1:
            cpu->DE = _pop(ctx);
            break;

        // POP HL
        case 0xE1:
            cpu->HL = _pop(ctx);
            break;

        // POP AF
        case 0xF1:
            cpu->AF = _pop(ctx);
            break;

        // ___ ALU ____________________________________________

        // --- 8bit -------------------------------------------

        // ADC A, r; ADD A, r
        case 0x88: value = 1;
        case 0x80: _add(ctx, cpu->B, (bool)value); break;
        case 0x89: value = 1;
        case 0x81: _add(ctx, cpu->C, (bool)value); break;
        case 0x8A: value = 1;
        case 0x82: _add(ctx, cpu->D, (bool)value); break;
        case 0x8B: value = 1;
        case 0x83: _add(ctx, cpu->E, (bool)value); break;
        case 0x8C: value = 1;
        case 0x84: _add(ctx, cpu->H, (bool)value); break;
        case 0x8D: value = 1;
        case 0x85: _add(ctx, cpu->L, (bool)value); break;
        case 0x8E: value = 1;
        case 0x86: _add(ctx, mem->map[cpu->HL], (bool)value);  break;
        case 0x8F: value = 1;
        case 0x87: _add(ctx, cpu->A, (bool)value); break;
        case 0xCE: value = 1;
        case 0xC6: _add(ctx, mem->map[cpu->PC++], (bool)value); break;

        // SBC A, r; SUB A, r
        case 0x98: value = 1;
        case 0x90: _sub(ctx, cpu->B, (bool)value); break;
        case 0x99: value = 1;
        case 0x91: _sub(ctx, cpu->C, (bool)value); break;
        case 0x9A: value = 1;
        case 0x92: _sub(ctx, cpu->D, (bool)value); break;
        case 0x9B: value = 1;
        case 0x93: _sub(ctx, cpu->E, (bool)value); break;
        case 0x9C: value = 1;
        case 0x94: _sub(ctx, cpu->H, (bool)value); break;
        case 0x9D: value = 1;
        case 0x95: _sub(ctx, cpu->L, (bool)value); break;
        case 0x9E: value = 1;
        case 0x96: _sub(ctx, mem->map[cpu->HL], (bool)value); break;
        case 0x9F: value = 1;
        case 0x97: _sub(ctx, cpu->A, (bool)value); break;
        case 0xDE: value = 1; // BIG ???, SBC A, n
        case 0xD6: _sub(ctx, mem->map[cpu->PC++], (bool)value); break;

        // AND r
        case 0xA0: _and(ctx, cpu->B); break;
        case 0xA1: _and(ctx, cpu->C); break;
        case 0xA2: _and(ctx, cpu->D); break;
        case 0xA3: _and(ctx, cpu->E); break;
        case 0xA4: _and(ctx, cpu->H); break;
        case 0xA5: _and(ctx, cpu->L); break;
        case 0xA6: _and(ctx, mem->map[cpu->HL]); break;
        case 0xA7: _and(ctx, cpu->A); break; // cpu->A = cpu->A?
        case 0xE6: _and(ctx, mem->map[cpu->PC++]); break;

        // XOR r
        case 0xA8: _xor(ctx, cpu->B); break;
        case 0xA9: _xor(ctx, cpu->C); break;
        case 0xAA: _xor(ctx, cpu->D); break;
        case 0xAB: _xor(ctx, cpu->E); break;
        case 0xAC: _xor(ctx, cpu->H); break;
        case 0xAD: _xor(ctx, cpu->L); break;
        case 0xAE: _xor(ctx, mem->map[cpu->HL]); break;
        case 0xAF: cpu->A = 0; break;
        case 0xEE: _xor(ctx, mem->map[cpu->PC++]); break;

        // OR r
        case 0xB0: _or(ctx, cpu->B); break;
        case 0xB1: _or(ctx, cpu->C); break;
        case 0xB2: _or(ctx, cpu->D); break;
        case 0xB3: _or(ctx, cpu->E); break;
        case 0xB4: _or(ctx, cpu->H); break;
        case 0xB5: _or(ctx, cpu->L); break;
        case 0xB6: _or(ctx, mem->map[cpu->HL]); break;
        case 0xB7: _or(ctx, cpu->A); break;
        case 0xF6: _or(ctx, mem->map[cpu->PC++]); break;

        // CP r
        case 0xB8: _cp(ctx, cpu->B); break;
        case 0xB9: _cp(ctx, cpu->C); break;
        case 0xBA: _cp(ctx, cpu->D); break;
        case 0xBB: _cp(ctx, cpu->E); break;
        case 0xBC: _cp(ctx, cpu->H); break;
        case 0xBD: _cp(ctx, cpu->L); break;
        case 0xBE: _cp(ctx, mem->map[cpu->HL]); break;
        case 0xBF: _cp(ctx, cpu->A); break;
        case 0xFE: _cp(ctx, mem->map[cpu->PC++]); break;

        // INC r
        case 0x04: _inc(ctx, &cpu->B); break;
        case 0x0C: _inc(ctx, &cpu->C); break;
        case 0x14: _inc(ctx, &cpu->D); break;
        case 0x1C: _inc(ctx, &cpu->E); break;
        case 0x24: _inc(ctx, &cpu->H); break;
        case 0x2C: _inc(ctx, &cpu->L); break;
        case 0x34: _inc(ctx, &mem->map[cpu->HL]); break;
        case 0x3C: _inc(ctx, &cpu->A); break;

        // DEC r
        case 0x05: _dec(ctx, &cpu->B); break;
        case 0x0D: _dec(ctx, &cpu->C); break;
        case 0x15: _dec(ctx, &cpu->D); break;
        case 0x1D: _dec(ctx, &cpu->E); break;
        case 0x25: _dec(ctx, &cpu->H); break;
        case 0x2D: _dec(ctx, &cpu->L); break;
        case 0x35: _dec(ctx, &mem->map[cpu->HL]); break;
        case 0x3D: _dec(ctx, &cpu->A); break;

        // --- 16bit ------------------------------

        // ADD HL, r
        case 0x09: _add_16(ctx, cpu->BC); break;
        case 0x19: _add_16(ctx, cpu->DE); break;
        case 0x29: _add_16(ctx, cpu->HL); break;
        case 0x39: _add_16(ctx, cpu->SP); break;

        // ADD SP, n
        case 0xE8:
            value = mem->map[cpu->PC++];

            cpu_set_z(cpu, 0);
            cpu_set_n(cpu, 0);
            cpu_set_h(cpu, (cpu->SP & 0xFF) + (value & 0xFF) > 0xFF);

            cpu->SP += (int8_t)value;

            break;

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
        case 0x27:
        /*
            The processor will have set the adjust flag if the
            sum of both lower nibbles is 16 or higher, and the
            carry flag if the sum of both bytes is 256 or higher.
            adjust flag = half carry flag
        */
        {
            // We need t to be > 16 bits to hold the carry bit
            unsigned int t = cpu->A;
            
            // Thank you, MAME
            if (cpu_get_n(cpu)) {
                // Last operation was a subtraction
                if (cpu_get_h(cpu))
                {
                    t -= 0x6;
                    
                    if (!cpu_get_c(cpu))
                    {
                        t &= 0xFF;
                    }
                }
                
                if (cpu_get_c(cpu))
                {
                    t -= 0x60;
                }
            } else {
                // Last operation was an addition
                if ((t & 0xF) > 0x9 || cpu_get_h(cpu)) {
                    t += 0x6;
                }
                
                if (t > 0x9F || cpu_get_c(cpu)) {
                    t += 0x60;
                }
            }
            
            cpu->A = t & 0xFF;
            cpu_set_c(cpu, t & 0x100);
            cpu_set_z(cpu, cpu->A);
            cpu_set_h(cpu, 0);
        }
        break;

        // CPL
        case 0x2F:
            cpu->A = ~cpu->A;
            cpu_set_n(cpu, 1);
            cpu_set_h(cpu, 1);
            break;

        // CCF
        case 0x3F:
            cpu_set_c(cpu, ~cpu_get_c(cpu));
            cpu_set_n(cpu, 0);
            cpu_set_h(cpu, 0);
            break;

        // SCF
        case 0x37:
            cpu_set_c(cpu, 1);
            cpu_set_n(cpu, 0);
            cpu_set_h(cpu, 0);

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
            _rotate_l(ctx, &cpu->A, false);
            break;

        // RLA
        case 0x17:
            _rotate_l(ctx, &cpu->A, true);
            break;

        // RRCA
        case 0x0F:
            _rotate_r(ctx, &cpu->A, false);
            break;

        // RRA
        case 0x1F:
            _rotate_r(ctx, &cpu->A, true);
            break;

        case 0xCB:
            value = mem->map[cpu->PC++];
            switch (value)
            {
                // RLC r
                case 0x00: _rotate_l(ctx, &cpu->B, false); break;
                case 0x01: _rotate_l(ctx, &cpu->C, false); break;
                case 0x02: _rotate_l(ctx, &cpu->D, false); break;
                case 0x03: _rotate_l(ctx, &cpu->E, false); break;
                case 0x04: _rotate_l(ctx, &cpu->H, false); break;
                case 0x05: _rotate_l(ctx, &cpu->L, false); break;
                case 0x06: _rotate_l(ctx, &mem->map[cpu->HL], false); break;
                case 0x07: _rotate_l(ctx, &cpu->A, false); break;

                // RRC r
                case 0x08: _rotate_r(ctx, &cpu->B, false); break;
                case 0x09: _rotate_r(ctx, &cpu->C, false); break;
                case 0x0A: _rotate_r(ctx, &cpu->D, false); break;
                case 0x0B: _rotate_r(ctx, &cpu->E, false); break;
                case 0x0C: _rotate_r(ctx, &cpu->H, false); break;
                case 0x0D: _rotate_r(ctx, &cpu->L, false); break;
                case 0x0E: _rotate_r(ctx, &mem->map[cpu->HL], false); break;
                case 0x0F: _rotate_r(ctx, &cpu->A, false); break;

                // RL r
                case 0x10: _rotate_l(ctx, &cpu->B, true); break;
                case 0x11: _rotate_l(ctx, &cpu->C, true); break;
                case 0x12: _rotate_l(ctx, &cpu->D, true); break;
                case 0x13: _rotate_l(ctx, &cpu->E, true); break;
                case 0x14: _rotate_l(ctx, &cpu->H, true); break;
                case 0x15: _rotate_l(ctx, &cpu->L, true); break;
                case 0x16: _rotate_l(ctx, &mem->map[cpu->HL], true); break;
                case 0x17: _rotate_l(ctx, &cpu->A, true); break;


                // RR r
                case 0x18: _rotate_r(ctx, &cpu->B, true); break;
                case 0x19: _rotate_r(ctx, &cpu->C, true); break;
                case 0x1A: _rotate_r(ctx, &cpu->D, true); break;
                case 0x1B: _rotate_r(ctx, &cpu->E, true); break;
                case 0x1C: _rotate_r(ctx, &cpu->H, true); break;
                case 0x1D: _rotate_r(ctx, &cpu->L, true); break;
                case 0x1E: _rotate_r(ctx, &mem->map[cpu->HL], true); break;
                case 0x1F: _rotate_r(ctx, &cpu->A, true); break;

                // SLA r
                case 0x20: _shift_l(ctx, &cpu->B); break;
                case 0x21: _shift_l(ctx, &cpu->C); break;
                case 0x22: _shift_l(ctx, &cpu->D); break;
                case 0x23: _shift_l(ctx, &cpu->E); break;
                case 0x24: _shift_l(ctx, &cpu->H); break;
                case 0x25: _shift_l(ctx, &cpu->L); break;
                case 0x26: _shift_l(ctx, &mem->map[cpu->HL]); break;
                case 0x27: _shift_l(ctx, &cpu->A); break;

                // SRA r
                case 0x28: _shift_r(ctx, &cpu->B); break;
                case 0x29: _shift_r(ctx, &cpu->C); break;
                case 0x2A: _shift_r(ctx, &cpu->D); break;
                case 0x2B: _shift_r(ctx, &cpu->E); break;
                case 0x2C: _shift_r(ctx, &cpu->H); break;
                case 0x2D: _shift_r(ctx, &cpu->L); break;
                case 0x2E: _shift_r(ctx, &mem->map[cpu->HL]); break;
                case 0x2F: _shift_r(ctx, &cpu->A); break;

                // SWAP r
                case 0x30: _swap(ctx, &cpu->B); break;
                case 0x31: _swap(ctx, &cpu->C); break;
                case 0x32: _swap(ctx, &cpu->D); break;
                case 0x33: _swap(ctx, &cpu->E); break;
                case 0x34: _swap(ctx, &cpu->H); break;
                case 0x35: _swap(ctx, &cpu->L); break;
                case 0x36: _swap(ctx, &mem->map[cpu->HL]); break;
                case 0x37: _swap(ctx, &cpu->A); break;

                // SRL r
                case 0x38: _shift_r_logic(ctx, &cpu->B); break;
                case 0x39: _shift_r_logic(ctx, &cpu->C); break;
                case 0x3A: _shift_r_logic(ctx, &cpu->D); break;
                case 0x3B: _shift_r_logic(ctx, &cpu->E); break;
                case 0x3C: _shift_r_logic(ctx, &cpu->H); break;
                case 0x3D: _shift_r_logic(ctx, &cpu->L); break;
                case 0x3E: _shift_r_logic(ctx, &mem->map[cpu->HL]); break;
                case 0x3F: _shift_r_logic(ctx, &cpu->A); break;

                // BIT n,B
                case 0x40:
                case 0x48:
                case 0x50:
                case 0x58:
                case 0x60:
                case 0x68:
                case 0x70:
                case 0x78:
                    value = (opcode - 0x40) % 8; // find bit
                    cpu_set_z(cpu, BIT_ISSET(cpu->B, value));
                    cpu_set_n(cpu, 0);
                    cpu_set_h(cpu, 1);
                    break;

                // BIT n,C
                case 0x41:
                case 0x49:
                case 0x51:
                case 0x59:
                case 0x61:
                case 0x69:
                case 0x71:
                case 0x79:
                    value = (opcode - 0x41) % 8;
                    cpu_set_z(cpu, BIT_ISSET(cpu->C, value));
                    cpu_set_n(cpu, 0);
                    cpu_set_h(cpu, 1);
                    break;

                // BIT n,D
                case 0x42:
                case 0x4A:
                case 0x52:
                case 0x5A:
                case 0x62:
                case 0x6A:
                case 0x72:
                case 0x7A:
                    value = (opcode - 0x42) % 8;
                    cpu_set_z(cpu, BIT_ISSET(cpu->D, value));
                    cpu_set_n(cpu, 0);
                    cpu_set_h(cpu, 1);
                    break;

                // BIT n,E
                case 0x43:
                case 0x4B:
                case 0x53:
                case 0x5B:
                case 0x63:
                case 0x6B:
                case 0x73:
                case 0x7B:
                    value = (opcode - 0x43) % 8;
                    cpu_set_z(cpu, BIT_ISSET(cpu->E, value));
                    cpu_set_n(cpu, 0);
                    cpu_set_h(cpu, 1);
                    break;

                // BIT n,H
                case 0x44:
                case 0x4C:
                case 0x54:
                case 0x5C:
                case 0x64:
                case 0x6C:
                case 0x74:
                case 0x7C:
                    value = (opcode - 0x44) % 8;
                    cpu_set_z(cpu, BIT_ISSET(cpu->H, value));
                    cpu_set_n(cpu, 0);
                    cpu_set_h(cpu, 1);
                    break;

                // BIT n,L
                case 0x45:
                case 0x4D:
                case 0x55:
                case 0x5D:
                case 0x65:
                case 0x6D:
                case 0x75:
                case 0x7D:
                    value = (opcode - 0x45) % 8;
                    cpu_set_z(cpu, BIT_ISSET(cpu->L, value));
                    cpu_set_n(cpu, 0);
                    cpu_set_h(cpu, 1);
                    break;

                // BIT n,(HL)
                case 0x46:
                case 0x4E:
                case 0x56:
                case 0x5E:
                case 0x66:
                case 0x6E:
                case 0x76:
                case 0x7E:
                    value = (opcode - 0x46) % 8;
                    cpu_set_z(cpu, BIT_ISSET(mem->map[cpu->HL], value));
                    cpu_set_n(cpu, 0);
                    cpu_set_h(cpu, 1);
                    break;

                // BIT n,A
                case 0x47:
                case 0x4F:
                case 0x57:
                case 0x5F:
                case 0x67:
                case 0x6F:
                case 0x77:
                case 0x7F:
                    value = (opcode - 0x47) % 8;
                    cpu_set_z(cpu, BIT_ISSET(cpu->A, value));
                    cpu_set_n(cpu, 0);
                    cpu_set_h(cpu, 1);
                    break;

                // RES n,B
                case 0x80:
                case 0x88:
                case 0x90:
                case 0x98:
                case 0xA0:
                case 0xA8:
                case 0xB0:
                case 0xB8:
                    value = (opcode - 0x80) % 8;
                    BIT_RESET(cpu->B, value);
                    break;

                // RES n,C
                case 0x81:
                case 0x89:
                case 0x91:
                case 0x99:
                case 0xA1:
                case 0xA9:
                case 0xB1:
                case 0xB9:
                    value = (opcode - 0x81) % 8;
                    BIT_RESET(cpu->C, value);
                    break;

                // RES n,D
                case 0x82:
                case 0x8A:
                case 0x92:
                case 0x9A:
                case 0xA2:
                case 0xAA:
                case 0xB2:
                case 0xBA:
                    value = (opcode - 0x82) % 8;
                    BIT_RESET(cpu->D, value);
                    break;

                // RES n,E
                case 0x83:
                case 0x8B:
                case 0x93:
                case 0x9B:
                case 0xA3:
                case 0xAB:
                case 0xB3:
                case 0xBB:
                    value = (opcode - 0x83) % 8;
                    BIT_RESET(cpu->E, value);
                    break;

                // RES n,H
                case 0x84:
                case 0x8C:
                case 0x94:
                case 0x9C:
                case 0xA4:
                case 0xAC:
                case 0xB4:
                case 0xBC:
                    value = (opcode - 0x84) % 8;
                    BIT_RESET(cpu->H, value);
                    break;

                // RES n,L
                case 0x85:
                case 0x8D:
                case 0x95:
                case 0x9D:
                case 0xA5:
                case 0xAD:
                case 0xB5:
                case 0xBD:
                    value = (opcode - 0x85) % 8;
                    BIT_RESET(cpu->L, value);
                    break;

                // RES n,(HL)
                case 0x86:
                case 0x8E:
                case 0x96:
                case 0x9E:
                case 0xA6:
                case 0xAE:
                case 0xB6:
                case 0xBE:
                    value = (opcode - 0x86) % 8;
                    BIT_RESET(*&mem->map[cpu->HL], value);
                    break;

                // RES n,A
                case 0x87:
                case 0x8F:
                case 0x97:
                case 0x9F:
                case 0xA7:
                case 0xAF:
                case 0xB7:
                case 0xBF:
                    value = (opcode - 0x87) % 8;
                    BIT_RESET(cpu->A, value);
                    break;

                // SET n,B
                case 0xC0:
                case 0xC8:
                case 0xD0:
                case 0xD8:
                case 0xE0:
                case 0xE8:
                case 0xF0:
                case 0xF8:
                    value = (opcode - 0xC0) % 8;
                    BIT_SET(cpu->B, value);
                    break;

                // SET n,C
                case 0xC1:
                case 0xC9:
                case 0xD1:
                case 0xD9:
                case 0xE1:
                case 0xE9:
                case 0xF1:
                case 0xF9:
                    value = (opcode - 0xC1) % 8;
                    BIT_SET(cpu->C, value);
                    break;

                // SET n,D
                case 0xC2:
                case 0xCA:
                case 0xD2:
                case 0xDA:
                case 0xE2:
                case 0xEA:
                case 0xF2:
                case 0xFA:
                    value = (opcode - 0xC2) % 8;
                    BIT_SET(cpu->D, value);
                    break;

                // SET n,E
                case 0xC3:
                case 0xCB:
                case 0xD3:
                case 0xDB:
                case 0xE3:
                case 0xEB:
                case 0xF3:
                case 0xFB:
                    value = (opcode - 0xC3) % 8;
                    BIT_SET(cpu->E, value);
                    break;

                // SET n,H
                case 0xC4:
                case 0xCC:
                case 0xD4:
                case 0xDC:
                case 0xE4:
                case 0xEC:
                case 0xF4:
                case 0xFC:
                    value = (opcode - 0xC4) % 8;
                    BIT_SET(cpu->H, value);
                    break;

                // SET n,L
                case 0xC5:
                case 0xCD:
                case 0xD5:
                case 0xDD:
                case 0xE5:
                case 0xED:
                case 0xF5:
                case 0xFD:
                    value = (opcode - 0xC5) % 8;
                    BIT_SET(cpu->L, value);
                    break;

                // SET n,(HL)
                case 0xC6:
                case 0xCE:
                case 0xD6:
                case 0xDE:
                case 0xE6:
                case 0xEE:
                case 0xF6:
                case 0xFE:
                    value = (opcode - 0xC6) % 8;
                    BIT_SET(mem->map[cpu->HL], value);
                    break;

                // SET n,A
                case 0xC7:
                case 0xCF:
                case 0xD7:
                case 0xDF:
                case 0xE7:
                case 0xEF:
                case 0xF7:
                case 0xFF:
                    value = (opcode - 0xC7) % 8;
                    BIT_SET(cpu->A, value);
                    break;

                default:
                    printf("FATAL: unhandled opcode 0xCB%02X at %X\n", opcode, cpu->PC);
                    cpu->halted = true;
                    goto exit_loop;
            }

            cycles += ext_opcode_meta[value].cycles;
            break;

        default:
            printf("FATAL: unhandled opcode 0x%02X at %X\n", opcode, cpu->PC);
            ctx->cpu.halted = true;
            goto exit_loop;
    }

    exit_loop:
    cycles += opcode_meta[opcode].cycles;
    return cycles;
}
