#ifndef __CPU_OPS_H__
#define __CPU_OPS_H__

#include "context.h"
#include "cpu.h"
#include <stdbool.h>
#include <stdint.h>

void _push(context_t *context, uint16_t value);
uint16_t _pop(context_t *context);
void _add(context_t *context, uint8_t n, bool add_carry);
void _add_16(context_t *context, uint16_t n);
void _sub(context_t *context, uint8_t n, bool add_carry);
void _inc(context_t *context, uint8_t* r);
void _dec(context_t *context, uint8_t* r);
void _and(context_t *context, uint8_t n);
void _or(context_t *context, uint8_t n);
void _xor(context_t *context, uint8_t n);
void _cp(context_t *context, uint8_t n);
void _swap(context_t *context, uint8_t* r);
void _rotate_l(context_t *context, uint8_t* r, bool through_carry);
void _rotate_r(context_t *context, uint8_t *r, bool through_carry);
void _shift_l(context_t *context, uint8_t* r);
void _shift_r(context_t *context, uint8_t* r);
void _shift_r_logic(context_t *context, uint8_t* r);
void _jump(context_t *context);
void _jump_rel(context_t *context);
void _call(context_t *context);
void _restart(context_t *context, uint8_t n);
void _return(context_t *context);

#endif//__CPU_OPS_H__
