#ifndef __CPU_OPS_H__
#define __CPU_OPS_H__

#include "spielbub.h"
#include <stdbool.h>
#include <stdint.h>

void _push(context_t *context, uint16_t value);
uint16_t _pop(context_t *context);

void _add(cpu_t* cpu, uint8_t n, bool add_carry);
void _add16(cpu_t* cpu, uint16_t n);
void _sub(cpu_t* cpu, uint8_t n, bool add_carry);
void _inc(cpu_t* cpu, uint8_t* r);
void _dec(cpu_t* cpu, uint8_t* r);
void _and(cpu_t* cpu, uint8_t n);
void _or(cpu_t* cpu, uint8_t n);
void _xor(cpu_t* cpu, uint8_t n);
void _cp(cpu_t* cpu, uint8_t n);
void _swap(cpu_t* cpu, uint8_t* r);
void _rotate_l(cpu_t* cpu, uint8_t* r);
void _rotate_r(cpu_t* cpu, uint8_t *r);
void _rotate_l_carry(cpu_t* cpu, uint8_t* r);
void _rotate_r_carry(cpu_t* cpu, uint8_t *r);
void _shift_l(cpu_t* cpu, uint8_t* r);
void _shift_r_arithm(cpu_t* cpu, uint8_t* r);
void _shift_r_logic(cpu_t* cpu, uint8_t* r);

void _jump(context_t *context);
void _jump_rel(context_t *context);
void _call(context_t *context);
void _restart(context_t *context, uint8_t n);
void _return(context_t *context);

#endif//__CPU_OPS_H__
