#ifndef __CPU_OPS_H__
#define __CPU_OPS_H__

#include "spielbub.h"
#include <stdbool.h>
#include <stdint.h>

void cpu_push(context_t *context, uint16_t value);
uint16_t cpu_pop(context_t *context);

void cpu_add(cpu_t* cpu, unsigned int n);
void cpu_add_carry(cpu_t* cpu, unsigned int n);
void cpu_add16(cpu_t* cpu, uint16_t n);
void cpu_sub(cpu_t* cpu, unsigned int n);
void cpu_sub_carry(cpu_t* cpu, unsigned int n);
void cpu_inc(cpu_t* cpu, uint8_t* r);
void cpu_dec(cpu_t* cpu, uint8_t* r);
void cpu_and(cpu_t* cpu, uint8_t n);
void cpu_or(cpu_t* cpu, uint8_t n);
void cpu_xor(cpu_t* cpu, uint8_t n);
void cpu_cp(cpu_t* cpu, uint8_t n);
void cpu_swap(cpu_t* cpu, uint8_t* r);
void cpu_rotate_l(cpu_t* cpu, uint8_t* r);
void cpu_rotate_r(cpu_t* cpu, uint8_t* r);
void cpu_rotate_l_carry(cpu_t* cpu, uint8_t* r);
void cpu_rotate_r_carry(cpu_t* cpu, uint8_t *r);
void cpu_shift_l(cpu_t* cpu, uint8_t* r);
void cpu_shift_r_arithm(cpu_t* cpu, uint8_t* r);
void cpu_shift_r_logic(cpu_t* cpu, uint8_t* r);
void cpu_daa(cpu_t* cpu);

void cpu_jump(context_t *context);
void cpu_jump_rel(context_t *context);
void cpu_call(context_t *context);
void cpu_restart(context_t *context, uint8_t n);
void cpu_return(context_t *context);

#endif//__CPU_OPS_H__
