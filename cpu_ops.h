#ifndef __CPU_OPS_H__
#define __CPU_OPS_H__

#include "context.h"
#include "cpu.h"
#include <stdbool.h>
#include <stdint.h>

inline void _push(context_t *context, reg_t* reg);
inline void _pop(context_t *context, reg_t* reg);
inline void _add(context_t *context, uint8_t n, bool add_carry);
inline void _add_16(context_t *context, uint16_t n);
inline void _sub(context_t *context, uint8_t n, bool add_carry);
inline void _inc(context_t *context, uint8_t* r);
inline void _dec(context_t *context, uint8_t* r);
inline void _and(context_t *context, uint8_t n);
inline void _or(context_t *context, uint8_t n);
inline void _xor(context_t *context, uint8_t n);
inline void _cp(context_t *context, uint8_t n);
inline void _swap(context_t *context, uint8_t* r);
inline void _rotate_l(context_t *context, uint8_t* r, bool through_carry);
inline void _rotate_r(context_t *context, uint8_t *r, bool through_carry);
inline void _shift_l(context_t *context, uint8_t* r);
inline void _shift_r(context_t *context, uint8_t* r);
inline void _shift_r_logic(context_t *context, uint8_t* r);
inline void _jump(context_t *context);
inline void _call(context_t *context);
inline void _restart(context_t *context, uint8_t n);
inline void _return(context_t *context);

#endif//__CPU_OPS_H__
