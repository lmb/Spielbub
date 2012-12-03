#include "cpu.h"
#include "cpu_ops.h"
#include "memory.h"
#include "ioregs.h"
#include "timers.h"
#include "graphics.h"

#include "logging.h"
#include "meta.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LOAD_REG16(hi, lo) { lo = mem_read(ctx->mem, _PC++); hi = mem_read(ctx->mem, _PC++); }

/*
 * Init the cpu part of the context_t struct.
 */
void cpu_init(cpu_t* cpu)
{
    // TODO: Move this to struct definition?
    // TODO: Are the .W values endian dependent?
    cpu->AF.B.h = 0x01;
    cpu->AF.B.l = 0xB0;
    cpu->BC.W   = 0x0013;
    cpu->DE.W   = 0x00D8;
    cpu->HL.W   = 0x014D;
    cpu->SP.W   = 0xFFFE;
    cpu->PC.W   = 0x0100;
    cpu->IME    = true;
}

/*
 * Request an interrupt. See interrupt_t for
 * possible interrupts.
 */
void cpu_irq(context_t *ctx, interrupt_t i)
{
    uint8_t* r_if = mem_address(ctx->mem, R_IF);

    // Set bit in IF
    *r_if |= 1 << i;
}

/*
 * Handle pending interrupts. Called from run().
 */
void cpu_interrupts(context_t *ctx)
{
    // Interrupt requests
    uint8_t* r_if = mem_address(ctx->mem, R_IF);
    // Interrupt enable
    uint8_t* r_ie = mem_address(ctx->mem, R_IE);

    int i;
    for (i = 0; i < 5; i++)
    {
        // We care about the first five bits.
        uint8_t mask = 1 << i;

        if (*r_if & mask && *r_ie & mask)
        {
            // Interrupt is requested and user
            // code is interested in it.
            ctx->cpu->IME    = false;
            ctx->cpu->halted = false;

            // Clear the interrupt from the interrupt
            // request register.
            *r_if &= ~mask;

            // Push the current program counter onto the stack
            _push(ctx, &(ctx->cpu->PC));

            // Select the appropriate ISR
            switch (i)
            {
                case 0: _PC = 0x40; break; // VBLANK
                case 1: _PC = 0x48; break; // LCDC
                case 2: _PC = 0x50; break; // TIMER
                case 3: _PC = 0x58; break; // SERIAL I/O
                case 4: _PC = 0x60; break; // JOYPAD
            }

            return;
        }
    }
}

/*
 * Executes one opcode at the current program counter.
 */
inline int cpu_run(context_t *ctx)
{
    register unsigned int value = 0;
    register unsigned int opcode;
    register unsigned int cycles = 0;

#if defined(DEBUG)
        cb_write(ctx->trace, &_PC);
#endif

        opcode = mem_read(ctx->mem, _PC++);

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
                ctx->cpu->halted = true;
                break;

            // ___ Jumps _____________________________

            // JP nn
            case 0xC3:
                _jump(ctx);
                break;

            // JP NZ, nn
            case 0xC2:
                if (!GET_Z()) _jump(ctx);
                else _PC += 2;
                break;

            // JP Z, nn
            case 0xCA:
                if (GET_Z()) _jump(ctx);
                else _PC += 2;
                break;

            // JP NC, nn
            case 0xD2:
                if (!GET_C()) _jump(ctx);
                else _PC += 2;
                break;

            // JP Z, nn
            case 0xDA:
                if (GET_C()) _jump(ctx);
                else _PC += 2;
                break;

            // JP (HL)
            case 0xE9:
                _PC = _HL;
                break;

            // Jump is calculated from instruction after the JR
            // JR n
            case 0x18:
                _PC += (int8_t)mem_read(ctx->mem, _PC) + 1;
                break;

            // JR NZ, n
            case 0x20:
                if (!GET_Z()) _PC += (int8_t)mem_read(ctx->mem, _PC) + 1;
                else _PC++;
                break;

            // JR Z, n
            case 0x28:
                if (GET_Z()) _PC += (int8_t)mem_read(ctx->mem, _PC) + 1;
                else _PC++;
                break;

            // JR NC, n
            case 0x30:
                if (!GET_C()) _PC += (int8_t)mem_read(ctx->mem, _PC) + 1;
                else _PC++;
                break;

            // JR C, n
            case 0x38:
                if (GET_C()) _PC += (int8_t)mem_read(ctx->mem, _PC) + 1;
                else _PC++;
                break;

            // CALL nn
            case 0xCD:
                _call(ctx);
                break;

            // CALL NZ, nn
            case 0xC4:
                if (!GET_Z()) _call(ctx);
                else _PC += 2;
                break;

            // CALL Z, nn
            case 0xCC:
                if (GET_Z()) _call(ctx);
                else _PC += 2;
                break;

            // CALL NC, nn
            case 0xD4:
                if (!GET_C()) _call(ctx);
                else _PC += 2;
                break;

            // CALL C, nn
            case 0xDC:
                if (GET_C()) _call(ctx);
                else _PC += 2;
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
                if (!GET_Z()) _return(ctx);
                break;

            // RET Z
            case 0xC8:
                if (GET_Z()) _return(ctx);
                break;

            // RET NC
            case 0xD0:
                if (!GET_C()) _return(ctx);
                break;

            // RET C
            case 0xD8:
                if (GET_C()) _return(ctx);
                break;

            // RETI
            case 0xD9:
                _return(ctx);
                ctx->cpu->IME = true;
                break;


            // ___ 8bit loads ________________________

            // LD B, r
            case 0x40: _B = _B; break;
            case 0x41: _B = _C; break;
            case 0x42: _B = _D; break;
            case 0x43: _B = _E; break;
            case 0x44: _B = _H; break;
            case 0x45: _B = _L; break;
            case 0x46: _B = mem_read(ctx->mem, _HL); break;
            case 0x47: _B = _A; break;

            // LD B, n
            case 0x06: _B = mem_read(ctx->mem, _PC++); break;

            // LD C, r
            case 0x48: _C = _B; break;
            case 0x49: _C = _C; break;
            case 0x4A: _C = _D; break;
            case 0x4B: _C = _E; break;
            case 0x4C: _C = _H; break;
            case 0x4D: _C = _L; break;
            case 0x4E: _C = mem_read(ctx->mem, _HL); break;
            case 0x4F: _C = _A; break;

            // LD C, n
            case 0x0E: _C = mem_read(ctx->mem, _PC++); break;

            // LD D, r
            case 0x50: _D = _B; break;
            case 0x51: _D = _C; break;
            case 0x52: _D = _D; break;
            case 0x53: _D = _E; break;
            case 0x54: _D = _H; break;
            case 0x55: _D = _L; break;
            case 0x56: _D = mem_read(ctx->mem, _HL); break;
            case 0x57: _D = _A; break;

            // LD D, n
            case 0x16: _D = mem_read(ctx->mem, _PC++); break;

            // LD E, r
            case 0x58: _E = _B; break;
            case 0x59: _E = _C; break;
            case 0x5A: _E = _D; break;
            case 0x5B: _E = _E; break;
            case 0x5C: _E = _H; break;
            case 0x5D: _E = _L; break;
            case 0x5E: _E = mem_read(ctx->mem, _HL); break;
            case 0x5F: _E = _A; break;

            // LD E, n
            case 0x1E: mem_read(ctx->mem, _PC++); break;

            // LD H, r
            case 0x60: _H = _B; break;
            case 0x61: _H = _C; break;
            case 0x62: _H = _D; break;
            case 0x63: _H = _E; break;
            case 0x64: _H = _H; break;
            case 0x65: _H = _L; break;
            case 0x66: _H = mem_read(ctx->mem, _HL); break;
            case 0x67: _H = _A; break;

            // LD H, n
            case 0x26: _H = mem_read(ctx->mem, _PC++); break;

            // LD L, r
            case 0x68: _L = _B; break;
            case 0x69: _L = _C; break;
            case 0x6A: _L = _D; break;
            case 0x6B: _L = _E; break;
            case 0x6C: _L = _H; break;
            case 0x6D: _L = _L; break;
            case 0x6E: _L = mem_read(ctx->mem, _HL); break;
            case 0x6F: _L = _A; break;

            // LD L, n
            case 0x2E: _L = mem_read(ctx->mem, _PC++); break;

            // LD (HL), r
            case 0x70: mem_write(ctx->mem, _HL, _B); break;
            case 0x71: mem_write(ctx->mem, _HL, _C); break;
            case 0x72: mem_write(ctx->mem, _HL, _D); break;
            case 0x73: mem_write(ctx->mem, _HL, _E); break;
            case 0x74: mem_write(ctx->mem, _HL, _H); break;
            case 0x75: mem_write(ctx->mem, _HL, _L); break;
            // TODO: What happens here?
            //case 0x76: /* HALT */ break;
            case 0x77: mem_write(ctx->mem, _HL, _A); break;

            // LD (HL), n
            case 0x36: mem_write(ctx->mem, _HL, mem_read(ctx->mem, _PC++)); break;

            // LD A, r
            case 0x78: _A = _B; break;
            case 0x79: _A = _C; break;
            case 0x7A: _A = _D; break;
            case 0x7B: _A = _E; break;
            case 0x7C: _A = _H; break;
            case 0x7D: _A = _L; break;
            case 0x7E: _A = mem_read(ctx->mem, _HL); break;
            case 0x7F: _A = _A; break;

            // LD A, n
            case 0x3E: _A = mem_read(ctx->mem, _PC++); break;

            // LD A, (BC)
            case 0x0A:
                _A = mem_read(ctx->mem, _BC);
                break;

            // LD A, (DE)
            case 0x1A:
                _A = mem_read(ctx->mem, _DE);
                break;

            // LD A, (nn)
            case 0xFA:
                _M = mem_read(ctx->mem, _PC++); _T = mem_read(ctx->mem, _PC++);
                _A = mem_read(ctx->mem, _TMP);
                break;

            // LDD A, (HL)
            case 0x3A:
                _A = mem_read(ctx->mem, _HL--);
                break;

            // LD A, (0xFF00+C)
            case 0xF2:
                _A = mem_read(ctx->mem, 0xFF00 + _C);
                break;

            // LDI A, (HL)
            case 0x2A:
                _A = mem_read(ctx->mem, _HL++);
                break;

            // LDH A, (0xFF00 + n)
            case 0xF0:
                _A = mem_read(ctx->mem, 0xFF00 + mem_read(ctx->mem, _PC++));
                break;

            // --------------------------------------

            // LD (BC), A
            case 0x02:
                mem_write(ctx->mem, _BC, _A);
                break;

            // LD (DE), A
            case 0x12:
                mem_write(ctx->mem, _DE, _A);
                break;

            // LD (nn), A
            case 0xEA:
                _M = mem_read(ctx->mem, _PC++); _T = mem_read(ctx->mem, _PC++);
                mem_write(ctx->mem, _TMP, _A);
                break;

            // LD (0xFF00+C), A
            case 0xE2:
                mem_write(ctx->mem, 0xFF00 + _C, _A);
                break;

            // LDD (HL), A
            case 0x32:
                mem_write(ctx->mem, _HL--, _A);
                break;

            // LDI (HL), A
            case 0x22:
                mem_write(ctx->mem, _HL++, _A);
                break;

            // LDH (0xFF00 + n), A
            case 0xE0:
                mem_write(ctx->mem, 0xFF00 + mem_read(ctx->mem, _PC++), _A);
                break;

            // ___ 16bit loads ________________________

            // LD BC, nn
            case 0x01:
                LOAD_REG16(_B, _C);
                break;

            // LD DE, nn
            case 0x11:
                LOAD_REG16(_D, _E);
                break;

            // LD HL, nn
            case 0x21:
                LOAD_REG16(_H, _L);
                break;

            // LD SP, nn
            case 0x31:
                LOAD_REG16(_S, _P);
                break;

            // LD SP, HL
            case 0xF9:
                _SP = _HL;
                break;

            // LD HL, SP+n
            case 0xF8:
                value = (int8_t)mem_read(ctx->mem, _PC++);

                SET_Z(0);
                SET_N(0);
                SET_H((_SP & 0xF) + (value & 0xF) > 0xF);

                value += _SP;
                SET_C(value > 0xFFFF);

                _HL = (uint16_t)value; // Check for overflow?

                break;

            // LD (nn), SP
            case 0x08:
                _M = mem_read(ctx->mem, _PC++); _T = mem_read(ctx->mem, _PC++);
                mem_write(ctx->mem, _TMP++, _P);
                mem_write(ctx->mem, _TMP,   _S);
                break;

            // PUSH BC
            case 0xC5:
                _push(ctx, &(ctx->cpu->BC));
                break;

            // PUSH DE
            case 0xD5:
                _push(ctx, &(ctx->cpu->DE));
                break;

            // PUSH HL
            case 0xE5:
                _push(ctx, &(ctx->cpu->HL));
                break;

            // PUSH AF
            case 0xF5:
                _push(ctx, &(ctx->cpu->AF));
                break;

            // POP BC
            case 0xC1:
                _pop(ctx, &(ctx->cpu->BC));
                break;

            // POP DE
            case 0xD1:
                _pop(ctx, &(ctx->cpu->DE));
                break;

            // POP HL
            case 0xE1:
                _pop(ctx, &(ctx->cpu->HL));
                break;

            // POP AF
            case 0xF1:
                _pop(ctx, &(ctx->cpu->AF));
                break;

            // ___ ALU ____________________________________________

            // --- 8bit -------------------------------------------

            // ADC A, r; ADD A, r
            case 0x88: value = 1;
            case 0x80: _add(ctx, _B, (bool)value); break;
            case 0x89: value = 1;
            case 0x81: _add(ctx, _C, (bool)value); break;
            case 0x8A: value = 1;
            case 0x82: _add(ctx, _D, (bool)value); break;
            case 0x8B: value = 1;
            case 0x83: _add(ctx, _E, (bool)value); break;
            case 0x8C: value = 1;
            case 0x84: _add(ctx, _H, (bool)value); break;
            case 0x8D: value = 1;
            case 0x85: _add(ctx, _L, (bool)value); break;
            case 0x8E: value = 1;
            case 0x86: _add(ctx, mem_read(ctx->mem, _HL), (bool)value);  break;
            case 0x8F: value = 1;
            case 0x87: _add(ctx, _A, (bool)value); break;
            case 0xCE: value = 1;
            case 0xC6: _add(ctx, mem_read(ctx->mem, _PC++), (bool)value); break;

            // SBC A, r; SUB A, r
            case 0x98: value = 1;
            case 0x90: _sub(ctx, _B, (bool)value); break;
            case 0x99: value = 1;
            case 0x91: _sub(ctx, _C, (bool)value); break;
            case 0x9A: value = 1;
            case 0x92: _sub(ctx, _D, (bool)value); break;
            case 0x9B: value = 1;
            case 0x93: _sub(ctx, _E, (bool)value); break;
            case 0x9C: value = 1;
            case 0x94: _sub(ctx, _H, (bool)value); break;
            case 0x9D: value = 1;
            case 0x95: _sub(ctx, _L, (bool)value); break;
            case 0x9E: value = 1;
            case 0x96: _sub(ctx, mem_read(ctx->mem, _HL), (bool)value); break;
            case 0x9F: value = 1;
            case 0x97: _sub(ctx, _A, (bool)value); break;
            case 0xDE: value = 1; // BIG ???, SBC A, n
            case 0xD6: _sub(ctx, mem_read(ctx->mem, _PC++), (bool)value); break;

            // AND r
            case 0xA0: _and(ctx, _B); break;
            case 0xA1: _and(ctx, _C); break;
            case 0xA2: _and(ctx, _D); break;
            case 0xA3: _and(ctx, _E); break;
            case 0xA4: _and(ctx, _H); break;
            case 0xA5: _and(ctx, _L); break;
            case 0xA6: _and(ctx, mem_read(ctx->mem, _HL)); break;
            case 0xA7: _and(ctx, _A); break; // _A = _A?
            case 0xE6: _and(ctx, mem_read(ctx->mem, _PC++)); break;

            // XOR r
            case 0xA8: _xor(ctx, _B); break;
            case 0xA9: _xor(ctx, _C); break;
            case 0xAA: _xor(ctx, _D); break;
            case 0xAB: _xor(ctx, _E); break;
            case 0xAC: _xor(ctx, _H); break;
            case 0xAD: _xor(ctx, _L); break;
            case 0xAE: _xor(ctx, mem_read(ctx->mem, _HL)); break;
            case 0xAF: _A = 0; break;
            case 0xEE: _xor(ctx, mem_read(ctx->mem, _PC++)); break;

            // OR r
            case 0xB0: _or(ctx, _B); break;
            case 0xB1: _or(ctx, _C); break;
            case 0xB2: _or(ctx, _D); break;
            case 0xB3: _or(ctx, _E); break;
            case 0xB4: _or(ctx, _H); break;
            case 0xB5: _or(ctx, _L); break;
            case 0xB6: _or(ctx, mem_read(ctx->mem, _HL)); break;
            case 0xB7: _or(ctx, _A); break;
            case 0xF6: _or(ctx, mem_read(ctx->mem, _PC++)); break;

            // CP r
            case 0xB8: _cp(ctx, _B); break;
            case 0xB9: _cp(ctx, _C); break;
            case 0xBA: _cp(ctx, _D); break;
            case 0xBB: _cp(ctx, _E); break;
            case 0xBC: _cp(ctx, _H); break;
            case 0xBD: _cp(ctx, _L); break;
            case 0xBE: _cp(ctx, mem_read(ctx->mem, _HL)); break;
            case 0xBF: _cp(ctx, _A); break;
            case 0xFE: _cp(ctx, mem_read(ctx->mem, _PC++)); break;

            // INC r
            case 0x04: _inc(ctx, &_B); break;
            case 0x0C: _inc(ctx, &_C); break;
            case 0x14: _inc(ctx, &_D); break;
            case 0x1C: _inc(ctx, &_E); break;
            case 0x24: _inc(ctx, &_H); break;
            case 0x2C: _inc(ctx, &_L); break;
            case 0x34: _inc(ctx, mem_address(ctx->mem, _HL)); break;
            case 0x3C: _inc(ctx, &_A); break;

            // DEC r
            case 0x05: _dec(ctx, &_B); break;
            case 0x0D: _dec(ctx, &_C); break;
            case 0x15: _dec(ctx, &_D); break;
            case 0x1D: _dec(ctx, &_E); break;
            case 0x25: _dec(ctx, &_H); break;
            case 0x2D: _dec(ctx, &_L); break;
            case 0x35: _dec(ctx, mem_address(ctx->mem, _HL)); break;
            case 0x3D: _dec(ctx, &_A); break;

            // --- 16bit ------------------------------

            // ADD HL, r
            case 0x09: _add_16(ctx, _BC); break;
            case 0x19: _add_16(ctx, _DE); break;
            case 0x29: _add_16(ctx, _HL); break;
            case 0x39: _add_16(ctx, _SP); break;

            // ADD SP, n
            case 0xE8:
                value = mem_read(ctx->mem, _PC++);

                SET_Z(0);
                SET_N(0);
                SET_H((_SP & 0xFF) + (value & 0xFF) > 0xFF);

                _SP += (int8_t)value;

                break;

            // INC r
            case 0x03: _BC++; break;
            case 0x13: _DE++; break;
            case 0x23: _HL++; break;
            case 0x33: _SP++; break;

            // DEC r
            case 0x0B: _BC--; break;
            case 0x1B: _DE--; break;
            case 0x2B: _HL--; break;
            case 0x3B: _SP--; break;

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
                unsigned int t = _A;
                
                // Thank you, MAME
                if (GET_N()) {
                    // Last operation was a subtraction
                    if (GET_H())
                    {
                        t -= 0x6;
                        
                        if (!GET_C())
                        {
                            t &= 0xFF;
                        }
                    }
                    
                    if (GET_C())
                    {
                        t -= 0x60;
                    }
                } else {
                    // Last operation was an addition
                    if (t & 0xF > 0x9 || GET_H()) {
                        t += 0x6;
                    }
                    
                    if (t > 0x9F || GET_C()) {
                        t += 0x60;
                    }
                }
                
                _A = t & 0xFF;
                SET_C(t & 0x100);
                SET_Z(_A);
                SET_H(0);
            }
            break;

            // CPL
            case 0x2F:
                _A = ~_A;
                SET_N(1);
                SET_H(1);
                break;

            // CCF
            case 0x3F:
                SET_C(~GET_C());
                SET_N(0);
                SET_H(0);
                break;

            // SCF
            case 0x37:
                SET_C(1);
                SET_N(0);
                SET_H(0);

            // DI
            case 0xF3:
                // Disable interrupts
                ctx->cpu->IME = false;
                break;

            // EI
            case 0xFB:
                // Enable interrupts
                ctx->cpu->IME = true;
                break;

            // RLCA
            case 0x07:
                _rotate_l(ctx, &_A, false);
                break;

            // RLA
            case 0x17:
                _rotate_l(ctx, &_A, true);
                break;

            // RRCA
            case 0x0F:
                _rotate_r(ctx, &_A, false);
                break;

            // RRA
            case 0x1F:
                _rotate_r(ctx, &_A, true);
                break;

            case 0xCB:
                value = mem_read(ctx->mem, _PC++);
                switch (value)
                {
                    // RLC r
                    case 0x00: _rotate_l(ctx, &_B, false); break;
                    case 0x01: _rotate_l(ctx, &_C, false); break;
                    case 0x02: _rotate_l(ctx, &_D, false); break;
                    case 0x03: _rotate_l(ctx, &_E, false); break;
                    case 0x04: _rotate_l(ctx, &_H, false); break;
                    case 0x05: _rotate_l(ctx, &_L, false); break;
                    case 0x06: _rotate_l(ctx, mem_address(ctx->mem, _HL), false); break;
                    case 0x07: _rotate_l(ctx, &_A, false); break;

                    // RRC r
                    case 0x08: _rotate_r(ctx, &_B, false); break;
                    case 0x09: _rotate_r(ctx, &_C, false); break;
                    case 0x0A: _rotate_r(ctx, &_D, false); break;
                    case 0x0B: _rotate_r(ctx, &_E, false); break;
                    case 0x0C: _rotate_r(ctx, &_H, false); break;
                    case 0x0D: _rotate_r(ctx, &_L, false); break;
                    case 0x0E: _rotate_r(ctx, mem_address(ctx->mem, _HL), false); break;
                    case 0x0F: _rotate_r(ctx, &_A, false); break;

                    // RL r
                    case 0x10: _rotate_l(ctx, &_B, true); break;
                    case 0x11: _rotate_l(ctx, &_C, true); break;
                    case 0x12: _rotate_l(ctx, &_D, true); break;
                    case 0x13: _rotate_l(ctx, &_E, true); break;
                    case 0x14: _rotate_l(ctx, &_H, true); break;
                    case 0x15: _rotate_l(ctx, &_L, true); break;
                    case 0x16: _rotate_l(ctx, mem_address(ctx->mem, _HL), true); break;
                    case 0x17: _rotate_l(ctx, &_A, true); break;


                    // RR r
                    case 0x18: _rotate_r(ctx, &_B, true); break;
                    case 0x19: _rotate_r(ctx, &_C, true); break;
                    case 0x1A: _rotate_r(ctx, &_D, true); break;
                    case 0x1B: _rotate_r(ctx, &_E, true); break;
                    case 0x1C: _rotate_r(ctx, &_H, true); break;
                    case 0x1D: _rotate_r(ctx, &_L, true); break;
                    case 0x1E: _rotate_r(ctx, mem_address(ctx->mem, _HL), true); break;
                    case 0x1F: _rotate_r(ctx, &_A, true); break;

                    // SLA r
                    case 0x20: _shift_l(ctx, &_B); break;
                    case 0x21: _shift_l(ctx, &_C); break;
                    case 0x22: _shift_l(ctx, &_D); break;
                    case 0x23: _shift_l(ctx, &_E); break;
                    case 0x24: _shift_l(ctx, &_H); break;
                    case 0x25: _shift_l(ctx, &_L); break;
                    case 0x26: _shift_l(ctx, mem_address(ctx->mem, _HL)); break;
                    case 0x27: _shift_l(ctx, &_A); break;

                    // SRA r
                    case 0x28: _shift_r(ctx, &_B); break;
                    case 0x29: _shift_r(ctx, &_C); break;
                    case 0x2A: _shift_r(ctx, &_D); break;
                    case 0x2B: _shift_r(ctx, &_E); break;
                    case 0x2C: _shift_r(ctx, &_H); break;
                    case 0x2D: _shift_r(ctx, &_L); break;
                    case 0x2E: _shift_r(ctx, mem_address(ctx->mem, _HL)); break;
                    case 0x2F: _shift_r(ctx, &_A); break;

                    // SWAP r
                    case 0x30: _swap(ctx, &_B); break;
                    case 0x31: _swap(ctx, &_C); break;
                    case 0x32: _swap(ctx, &_D); break;
                    case 0x33: _swap(ctx, &_E); break;
                    case 0x34: _swap(ctx, &_H); break;
                    case 0x35: _swap(ctx, &_L); break;
                    case 0x36: _swap(ctx, mem_address(ctx->mem, _HL)); break;
                    case 0x37: _swap(ctx, &_A); break;

                    // SRL r
                    case 0x38: _shift_r_logic(ctx, &_B); break;
                    case 0x39: _shift_r_logic(ctx, &_C); break;
                    case 0x3A: _shift_r_logic(ctx, &_D); break;
                    case 0x3B: _shift_r_logic(ctx, &_E); break;
                    case 0x3C: _shift_r_logic(ctx, &_H); break;
                    case 0x3D: _shift_r_logic(ctx, &_L); break;
                    case 0x3E: _shift_r_logic(ctx, mem_address(ctx->mem, _HL)); break;
                    case 0x3F: _shift_r_logic(ctx, &_A); break;

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
                        SET_Z(BIT_ISSET(_B, value));
                        SET_N(0);
                        SET_H(1);
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
                        SET_Z(BIT_ISSET(_C, value));
                        SET_N(0);
                        SET_H(1);
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
                        SET_Z(BIT_ISSET(_D, value));
                        SET_N(0);
                        SET_H(1);
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
                        SET_Z(BIT_ISSET(_E, value));
                        SET_N(0);
                        SET_H(1);
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
                        SET_Z(BIT_ISSET(_H, value));
                        SET_N(0);
                        SET_H(1);
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
                        SET_Z(BIT_ISSET(_L, value));
                        SET_N(0);
                        SET_H(1);
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
                        SET_Z(BIT_ISSET(mem_read(ctx->mem, _HL), value));
                        SET_N(0);
                        SET_H(1);
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
                        SET_Z(BIT_ISSET(_A, value));
                        SET_N(0);
                        SET_H(1);
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
                        BIT_RESET(_B, value);
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
                        BIT_RESET(_C, value);
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
                        BIT_RESET(_D, value);
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
                        BIT_RESET(_E, value);
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
                        BIT_RESET(_H, value);
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
                        BIT_RESET(_L, value);
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
                        BIT_RESET(*mem_address(ctx->mem, _HL), value);
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
                        BIT_RESET(_A, value);
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
                        BIT_SET(_B, value);
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
                        BIT_SET(_C, value);
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
                        BIT_SET(_D, value);
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
                        BIT_SET(_E, value);
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
                        BIT_SET(_H, value);
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
                        BIT_SET(_L, value);
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
                        BIT_SET(*mem_address(ctx->mem, _HL), value);
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
                        BIT_SET(_A, value);
                        break;

                    default:
                        log_dbg("FATAL: unhandled opcode 0xCB%02X at %X\n", opcode, _PC);
                        ctx->cpu->halted = true;
                        goto exit_loop;
                }

                cycles += ext_opcode_meta[value].cycles;
                break;

            default:
                log_dbg("FATAL: unhandled opcode 0x%02X at %X\n", opcode, _PC);
                ctx->cpu->halted = true;
                goto exit_loop;
        }

    exit_loop:
    cycles += opcode_meta[opcode].cycles;
    return cycles;
}
