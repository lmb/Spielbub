#ifndef __CPU_H__
#define __CPU_H__

#include <stdbool.h>

#include "spielbub.h"
#include "hardware.h"

#if defined(DEBUG)
#include "probability_list.h"
#include "buffers.h"
#endif

typedef enum {
    I_VBLANK = 0x0, I_LCDC, I_TIMER, I_SERIAL_IO, I_JOYPAD
} interrupt_t;

typedef struct cpu {
    union {
        struct {
            uint16_t AF;
            uint16_t BC;
            uint16_t DE;
            uint16_t HL;
            uint16_t SP;
            uint16_t PC;
        };
        struct {
            uint8_t F, A;
            uint8_t C, B;
            uint8_t E, D;
            uint8_t L, H;
        };
    };

    // Interrupt Master Enable
    bool IME;
    
    // If the CPU is halted it does
    // nothing until the next interrupt
    // occurs
    bool halted;
    
    // Number of cycles executed
    //unsigned int cycles;

#if defined(DEBUG)
    circular_buffer* trace;
    prob_list_t breakpoints;
#endif
} cpu_t;

void cpu_init(cpu_t *cpu);
int cpu_run(context_t *context);

void cpu_irq(context_t*, interrupt_t i);
void cpu_interrupts(context_t *ctx);

/*
 * Flags & flag manipulation
 */

static inline void cpu_set_z(cpu_t *cpu, bool value)
{
    if (!value) {
        cpu->F |= 0x80;
    } else {
        cpu->F &= ~0x80;
    }
}

static inline void cpu_set_n(cpu_t *cpu, bool value)
{
    if (value) {
        cpu->F |= 0x40;
    } else {
        cpu->F &= ~0x40;
    }
}

static inline void cpu_set_h(cpu_t *cpu, bool value)
{
    if (value) {
        cpu->F |= 0x20;
    } else {
        cpu->F &= ~0x20;
    }
}

static inline void cpu_set_c(cpu_t *cpu, bool value)
{
    if (value) {
        cpu->F |= 0x10;
    } else {
        cpu->F &= ~0x10;
    }
}

static inline bool cpu_get_z(const cpu_t *cpu)
{
    return (cpu->F & 0x80) >> 7;
}

static inline bool cpu_get_n(const cpu_t *cpu)
{
    return (cpu->F & 0x40) >> 6;
}

static inline bool cpu_get_h(const cpu_t *cpu)
{
    return (cpu->F & 0x20) >> 5;
}

static inline bool cpu_get_c(const cpu_t *cpu)
{
    return (cpu->F & 0x10) >> 4;
}

#endif//__CPU_H__
