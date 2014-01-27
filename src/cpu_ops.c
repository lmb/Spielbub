#include "context.h"

#include "cpu_ops.h"
#include "memory.h"

#include "logging.h"

// __ Stack ________________________________________

void _push(context_t *ctx, uint16_t value)
{
    ctx->cpu.SP -= 2;
    mem_write16(&ctx->mem, ctx->cpu.SP, value);
}

uint16_t _pop(context_t *ctx)
{
    const uint16_t value = mem_read16(&ctx->mem, ctx->cpu.SP);

    ctx->cpu.SP += 2;
    return value;
}

// ___ ALU __________________________________________
void _add(cpu_t* cpu, uint8_t n, bool add_carry)
{
    n += add_carry ? cpu_get_c(cpu) : 0;
    unsigned int value = cpu->A + n;
    unsigned int half  = (cpu->A & 0xF) + (n & 0xF);

    cpu_set_n(cpu, false);
    cpu_set_z(cpu, value == 0);
    cpu_set_c(cpu, value > 0xFF);
    cpu_set_h(cpu, half > 0xF);

    cpu->A = (uint8_t)value;
}

void _add16(cpu_t* cpu, uint16_t n)
{
    unsigned int value = cpu->HL + n;

    cpu_set_n(cpu, false);
    cpu_set_c(cpu, value > 0xFFFF);
    cpu_set_h(cpu, (cpu->HL & 0xFF) + (n & 0xFF) > 0xFF);

    cpu->HL = (uint16_t)value;
}

void _sub(cpu_t* cpu, uint8_t n, bool add_carry)
{
    n += add_carry ? cpu_get_c(cpu) : 0;
    unsigned int value = cpu->A - n;

    cpu_set_n(cpu, true);
    cpu_set_z(cpu, value == 0);
    cpu_set_c(cpu, n > cpu->A);
    cpu_set_h(cpu, (n & 0xF) > (cpu->A & 0xF));

    cpu->A = value;
}

void _inc(cpu_t* cpu, uint8_t* r)
{
    cpu_set_h(cpu, (*r & 0xF) + 1 > 0xF);
    cpu_set_n(cpu, false);

    *r += 1;
    cpu_set_z(cpu, *r == 0);
}

void _dec(cpu_t* cpu, uint8_t* r)
{
    cpu_set_n(cpu, true);
    cpu_set_h(cpu, (*r & 0xF) == 0);

    *r -= 1;
    cpu_set_z(cpu, *r == 0);
}

void _and(cpu_t* cpu, uint8_t n)
{
    cpu->A &= n;

    cpu_set_z(cpu, cpu->A == 0);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, false);
    cpu_set_h(cpu, true);
}

void _or(cpu_t* cpu, uint8_t n)
{
    cpu->A |= n;

    cpu_set_z(cpu, cpu->A == 0);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, false);
    cpu_set_h(cpu, false);
}

void _xor(cpu_t* cpu, uint8_t n)
{
    cpu->A ^= n;

    cpu_set_z(cpu, cpu->A == 0);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, false);
    cpu_set_h(cpu, false);
}

void _cp(cpu_t* cpu, uint8_t n)
{
    int value = cpu->A - n;

    cpu_set_z(cpu, value == 0);
    cpu_set_n(cpu, true);
    cpu_set_c(cpu, value < 0);
    cpu_set_h(cpu, false);
}

void _swap(cpu_t* cpu, uint8_t* r)
{
    *r = ((*r & 0xF) << 4) | ((*r & 0xF0) >> 4);

    cpu_set_z(cpu, *r == 0);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, false);
    cpu_set_h(cpu, false);
}

void _rotate_l(cpu_t* cpu, uint8_t* r)
{
    unsigned int msb = (*r & 0x80) >> 7;

    *r = (*r << 1) | msb;

    cpu_set_z(cpu, false);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, msb);
    cpu_set_h(cpu, false);
}

void _rotate_l_carry(cpu_t* cpu, uint8_t* r)
{
    unsigned int msb = (*r & 0x80) >> 7;

    *r = (*r << 1) | cpu_get_c(cpu);

    cpu_set_z(cpu, false);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, msb);
    cpu_set_h(cpu, false);
}

void _rotate_r(cpu_t* cpu, uint8_t* r)
{
    unsigned int lsb = *r & 1;

    *r = (lsb << 7) | (*r >> 1);

    cpu_set_z(cpu, false);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, lsb);
    cpu_set_h(cpu, false);
}

void _rotate_r_carry(cpu_t* cpu, uint8_t* r)
{
    unsigned int lsb = *r & 1;

    *r = (cpu_get_c(cpu) << 7) | (*r >> 1);

    cpu_set_z(cpu, false);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, lsb);
    cpu_set_h(cpu, false);
}

void _shift_l(cpu_t* cpu, uint8_t* r)
{
    cpu_set_c(cpu, *r & 0x80);
    cpu_set_n(cpu, false);
    cpu_set_h(cpu, false);

    *r <<= 1;

    cpu_set_z(cpu, *r == 0);
}

void _shift_r_arithm(cpu_t* cpu, uint8_t* r)
{
    unsigned int msb = *r & 0x80;

    cpu_set_c(cpu, *r & 0x01);
    cpu_set_n(cpu, false);
    cpu_set_h(cpu, false);

    *r = msb | (*r >> 1);

    cpu_set_z(cpu, *r == 0);
}

void _shift_r_logic(cpu_t* cpu, uint8_t* r)
{
    cpu_set_c(cpu, *r & 0x01);
    cpu_set_n(cpu, false);
    cpu_set_h(cpu, false);

    *r >>= 1;

    cpu_set_z(cpu, *r == 0);
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
