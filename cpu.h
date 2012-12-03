#ifndef __CPU_H__
#define __CPU_H__

#include <stdbool.h>
#include "hardware.h"
#include "context.h"

typedef enum {
    I_VBLANK = 0x0, I_LCDC, I_TIMER, I_SERIAL_IO, I_JOYPAD
} interrupt_t;

typedef union {
    uint16_t W;
    struct {
        uint8_t l, h;
    } B;
} reg_t;

struct cpu_opaque_t {
    // Registers
    reg_t AF;
    reg_t BC;
    reg_t DE;
    reg_t HL;
    reg_t SP;
    reg_t PC;
    reg_t TMP;
    
    // Interrupt Master Enable
    bool IME;
    
    // If the CPU is halted it does
    // nothing until the next interrupt
    // occurs
    bool halted;
    
    // Number of cycles executed
    int cycles;
};

void cpu_init(cpu_t *cpu);
inline int cpu_run(context_t *context);

void cpu_irq(context_t*, interrupt_t i);
void cpu_interrupts(context_t *ctx);

/*
 * 8bit registers
 */
#define _A ctx->cpu->AF.B.h
#define _F ctx->cpu->AF.B.l
#define _B ctx->cpu->BC.B.h
#define _C ctx->cpu->BC.B.l
#define _D ctx->cpu->DE.B.h
#define _E ctx->cpu->DE.B.l
#define _H ctx->cpu->HL.B.h
#define _L ctx->cpu->HL.B.l
#define _S ctx->cpu->SP.B.h
#define _P ctx->cpu->SP.B.l
// temporary reg
#define _T ctx->cpu->TMP.B.h
#define _M ctx->cpu->TMP.B.l

/*
 * 16bit registers
 */
#define _AF ctx->cpu->AF.W
#define _BC ctx->cpu->BC.W
#define _DE ctx->cpu->DE.W
#define _HL ctx->cpu->HL.W
#define _SP ctx->cpu->SP.W
// temporary reg
#define _TMP ctx->cpu->TMP.W

/*
 * Program counter
 */
#define _PC ctx->cpu->PC.W

/*
 * Flags & flag manipulation
 */
#define SET_Z(x) {if((x) == 0){ _F |= 0x80; }else{ _F &= ~0x80; }}
#define SET_N(x) {if((x) != 0){ _F |= 0x40; }else{ _F &= ~0x40; }}
#define SET_H(x) {if((x) != 0){ _F |= 0x20; }else{ _F &= ~0x20; }}
#define SET_C(x) {if((x) != 0){ _F |= 0x10; }else{ _F &= ~0x10; }}

#define GET_Z() ((_F & 0x80) >> 7)
#define GET_N() ((_F & 0x40) >> 6)
#define GET_H() ((_F & 0x20) >> 5)
#define GET_C() ((_F & 0x10) >> 4)

#endif//__CPU_H__
