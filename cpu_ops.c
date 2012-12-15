#include "cpu_ops.h"
#include "memory.h"

#include "logging.h"

// __ Stack ________________________________________

void _push(context_t *ctx, reg_t* reg)
{
    mem_write(ctx->mem, _SP--, reg->B.h);
    mem_write(ctx->mem, _SP--, reg->B.l);
}

void _pop(context_t *ctx, reg_t* reg)
{
    reg->B.l = mem_read(ctx->mem, ++_SP);
    reg->B.h = mem_read(ctx->mem, ++_SP);
}

// ___ ALU __________________________________________
void _add(context_t *ctx, uint8_t n, bool add_carry)
{
    n += add_carry ? GET_C() : 0;
    register unsigned int value = _A + n;
    register unsigned int half  = (_A & 0xF) + (n & 0xF);

    SET_N(0);
    SET_Z(value);
    SET_C(value > 0xFF);
    SET_H(half > 0xF);

    _A = (uint8_t)value;
}

void _add_16(context_t *ctx, uint16_t n)
{
    unsigned int value = _HL + n;

    SET_N(0);
    SET_C(value > 0xFFFF);
    SET_H((_HL & 0xFF) + (n & 0xFF) > 0xFF);

    _HL = (uint16_t)value;
}

void _sub(context_t *ctx, uint8_t n, bool add_carry)
{
    n += add_carry ? GET_C() : 0;
    register unsigned int value = _A - n;
    register unsigned int half  = (_A & 0xF) - (n & 0xF);

    _A = value;
    
    SET_N(1);
    SET_C(value > 0xFF);
    SET_H(half > 0xF);
    SET_Z(_A);
}

void _inc(context_t *ctx, uint8_t* r)
{
    SET_H((*r & 0xF) + 1 > 0xF);
    SET_N(0);

    *r += 1;
    SET_Z(*r);
}

void _dec(context_t *ctx, uint8_t* r)
{
    SET_N(1);
    SET_H((*r & 0xF) >= 1);

    *r -= 1;
    SET_Z(*r);
}

void _and(context_t *ctx, uint8_t n)
{
    _A &= n;

    SET_Z(_A);
    SET_N(0);
    SET_C(0);
    SET_H(1);
}

void _or(context_t *ctx, uint8_t n)
{
    _A |= n;

    SET_Z(_A);
    SET_N(0);
    SET_C(0);
    SET_H(0);
}

void _xor(context_t *ctx, uint8_t n)
{
    _A ^= n;

    SET_Z(_A);
    SET_N(0);
    SET_C(0);
    SET_H(0);
}

void _cp(context_t *ctx, uint8_t n)
{
    int value = _A - n;

    SET_Z(value);
    SET_N(1);
    SET_C(value < 0);
    SET_H(0);
}

void _swap(context_t *ctx, uint8_t* r)
{
    *r = ((*r & 0xF) << 4) | ((*r & 0xF0) >> 4);

    SET_Z(*r);
    SET_N(0);
    SET_C(0);
    SET_H(0);
}

void _rotate_l(context_t *ctx, uint8_t* r, bool through_carry)
{
    int carry = through_carry ? GET_C() : 0;

    SET_C(*r & 0x80);
    *r <<= 1;
    *r |= carry;

    SET_N(0);
    SET_H(0);
    SET_Z(*r);
}

void _rotate_r(context_t *ctx, uint8_t* r, bool through_carry)
{
    int carry = through_carry ? GET_C() << 7 : 0;

    SET_C(*r & 0x01);
    *r >>= 1;
    *r |= carry;

    SET_N(0);
    SET_H(0);
    SET_Z(*r);
}

void _shift_l(context_t *ctx, uint8_t* r)
{
    SET_C(*r & 0x80);
    SET_N(0);
    SET_H(0);
    *r <<= 1;
    SET_Z(*r);
}

void _shift_r(context_t *ctx, uint8_t* r)
{
    uint8_t b7;

    SET_C(*r & 0x01);
    SET_N(0);
    SET_H(0);

    b7 = *r & 0x80;
    *r >>= 1;
    *r |= b7;

    SET_Z(*r);
}

void _shift_r_logic(context_t *ctx, uint8_t* r)
{
    SET_C(*r & 0x01);
    SET_N(0);
    SET_H(0);
    *r >>= 1;
    SET_Z(*r);
}

// __ Flow control ______________________________

void _jump(context_t *ctx)
{
    _M = mem_read(ctx->mem, _PC++); _T = mem_read(ctx->mem, _PC++);
    _PC = _TMP;
}

void _call(context_t *ctx)
{
    _M = mem_read(ctx->mem, _PC++); _T = mem_read(ctx->mem, _PC++);
    _push(ctx, &(ctx->cpu->PC));
    _PC = _TMP;
}

void _restart(context_t *ctx, uint8_t n)
{
    _push(ctx, &(ctx->cpu->PC));
    _PC = n;
}

void _return(context_t *ctx)
{
    _pop(ctx, &(ctx->cpu->PC));
}
