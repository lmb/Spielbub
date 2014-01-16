#include "context.h"

#include "cpu_ops.h"
#include "memory.h"

#include "logging.h"

// __ Stack ________________________________________

void _push(context_t *ctx, uint16_t value)
{
    ctx->cpu.SP -= 2;
    mem_write(&ctx->mem, ctx->cpu.SP, value);
}

uint16_t _pop(context_t *ctx)
{
    const uint16_t value = mem_read16(&ctx->mem, ctx->cpu.SP);

    ctx->cpu.SP += 2;
    return value;
}

// ___ ALU __________________________________________
void _add(context_t *ctx, uint8_t n, bool add_carry)
{
    n += add_carry ? cpu_get_c(&ctx->cpu) : 0;
    unsigned int value = ctx->cpu.A + n;
    unsigned int half  = (ctx->cpu.A & 0xF) + (n & 0xF);

    cpu_set_n(&ctx->cpu, 0);
    cpu_set_z(&ctx->cpu, value);
    cpu_set_c(&ctx->cpu, value > 0xFF);
    cpu_set_h(&ctx->cpu, half > 0xF);

    ctx->cpu.A = (uint8_t)value;
}

void _add_16(context_t *ctx, uint16_t n)
{
    unsigned int value = ctx->cpu.HL + n;

    cpu_set_n(&ctx->cpu, 0);
    cpu_set_c(&ctx->cpu, value > 0xFFFF);
    cpu_set_h(&ctx->cpu, (ctx->cpu.HL & 0xFF) + (n & 0xFF) > 0xFF);

    ctx->cpu.HL = (uint16_t)value;
}

void _sub(context_t *ctx, uint8_t n, bool add_carry)
{
    n += add_carry ? cpu_get_c(&ctx->cpu) : 0;
    unsigned int value = ctx->cpu.A - n;
    unsigned int half  = (ctx->cpu.A & 0xF) - (n & 0xF);

    ctx->cpu.A = value;
    
    cpu_set_n(&ctx->cpu, 1);
    cpu_set_c(&ctx->cpu, value > 0xFF);
    cpu_set_h(&ctx->cpu, half > 0xF);
    cpu_set_z(&ctx->cpu, ctx->cpu.A);
}

void _inc(context_t *ctx, uint8_t* r)
{
    cpu_set_h(&ctx->cpu, (*r & 0xF) + 1 > 0xF);
    cpu_set_n(&ctx->cpu, 0);

    *r += 1;
    cpu_set_z(&ctx->cpu, *r);
}

void _dec(context_t *ctx, uint8_t* r)
{
    cpu_set_n(&ctx->cpu, 1);
    cpu_set_h(&ctx->cpu, (*r & 0xF) >= 1);

    *r -= 1;
    cpu_set_z(&ctx->cpu, *r);
}

void _and(context_t *ctx, uint8_t n)
{
    ctx->cpu.A &= n;

    cpu_set_z(&ctx->cpu, ctx->cpu.A);
    cpu_set_n(&ctx->cpu, 0);
    cpu_set_c(&ctx->cpu, 0);
    cpu_set_h(&ctx->cpu, 1);
}

void _or(context_t *ctx, uint8_t n)
{
    ctx->cpu.A |= n;

    cpu_set_z(&ctx->cpu, ctx->cpu.A);
    cpu_set_n(&ctx->cpu, 0);
    cpu_set_c(&ctx->cpu, 0);
    cpu_set_h(&ctx->cpu, 0);
}

void _xor(context_t *ctx, uint8_t n)
{
    ctx->cpu.A ^= n;

    cpu_set_z(&ctx->cpu, ctx->cpu.A);
    cpu_set_n(&ctx->cpu, 0);
    cpu_set_c(&ctx->cpu, 0);
    cpu_set_h(&ctx->cpu, 0);
}

void _cp(context_t *ctx, uint8_t n)
{
    int value = ctx->cpu.A - n;

    cpu_set_z(&ctx->cpu, value);
    cpu_set_n(&ctx->cpu, 1);
    cpu_set_c(&ctx->cpu, value < 0);
    cpu_set_h(&ctx->cpu, 0);
}

void _swap(context_t *ctx, uint8_t* r)
{
    *r = ((*r & 0xF) << 4) | ((*r & 0xF0) >> 4);

    cpu_set_z(&ctx->cpu, *r);
    cpu_set_n(&ctx->cpu, 0);
    cpu_set_c(&ctx->cpu, 0);
    cpu_set_h(&ctx->cpu, 0);
}

void _rotate_l(context_t *ctx, uint8_t* r, bool through_carry)
{
    int carry = through_carry ? cpu_get_c(&ctx->cpu) : 0;

    cpu_set_c(&ctx->cpu, *r & 0x80);
    *r <<= 1;
    *r |= carry;

    cpu_set_n(&ctx->cpu, 0);
    cpu_set_h(&ctx->cpu, 0);
    cpu_set_z(&ctx->cpu, *r);
}

void _rotate_r(context_t *ctx, uint8_t* r, bool through_carry)
{
    int carry = through_carry ? cpu_get_c(&ctx->cpu) << 7 : 0;

    cpu_set_c(&ctx->cpu, *r & 0x01);
    *r >>= 1;
    *r |= carry;

    cpu_set_n(&ctx->cpu, 0);
    cpu_set_h(&ctx->cpu, 0);
    cpu_set_z(&ctx->cpu, *r);
}

void _shift_l(context_t *ctx, uint8_t* r)
{
    cpu_set_c(&ctx->cpu, *r & 0x80);
    cpu_set_n(&ctx->cpu, 0);
    cpu_set_h(&ctx->cpu, 0);
    *r <<= 1;
    cpu_set_z(&ctx->cpu, *r);
}

void _shift_r(context_t *ctx, uint8_t* r)
{
    uint8_t b7;

    cpu_set_c(&ctx->cpu, *r & 0x01);
    cpu_set_n(&ctx->cpu, 0);
    cpu_set_h(&ctx->cpu, 0);

    b7 = *r & 0x80;
    *r >>= 1;
    *r |= b7;

    cpu_set_z(&ctx->cpu, *r);
}

void _shift_r_logic(context_t *ctx, uint8_t* r)
{
    cpu_set_c(&ctx->cpu, *r & 0x01);
    cpu_set_n(&ctx->cpu, 0);
    cpu_set_h(&ctx->cpu, 0);
    *r >>= 1;
    cpu_set_z(&ctx->cpu, *r);
}

// __ Flow control ______________________________

void _jump(context_t *ctx)
{
    ctx->cpu.PC = mem_read16(&ctx->mem, ctx->cpu.PC);
}

void _jump_rel(context_t *ctx)
{
    ctx->cpu.PC += ((int8_t)ctx->mem.map[ctx->cpu.PC]) + 1;
}

void _call(context_t *ctx)
{
    _push(ctx, ctx->cpu.PC + 2);
    ctx->cpu.PC = mem_read16(&ctx->mem, ctx->cpu.PC);
}

void _restart(context_t *ctx, uint8_t n)
{
    _push(ctx, ctx->cpu.PC);
    ctx->cpu.PC = n;
}

void _return(context_t *ctx)
{
    ctx->cpu.PC = _pop(ctx);
}
