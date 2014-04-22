#include "context.h"

#include "cpu_ops.h"
#include "memory.h"

#include "logging.h"

// __ Stack ________________________________________

void cpu_push(context_t *ctx, uint16_t value)
{
    ctx->cpu.SP -= 2;
    mem_write16(&ctx->mem, ctx->cpu.SP, value);
}

uint16_t cpu_pop(context_t *ctx)
{
    const uint16_t value = mem_read16(&ctx->mem, ctx->cpu.SP);

    ctx->cpu.SP += 2;
    return value;
}

// ___ ALU __________________________________________
void cpu_add(cpu_t* cpu, unsigned int n)
{
    unsigned int value = cpu->A + n;
    unsigned int half  = (cpu->A & 0xF) + (n & 0xF);

    cpu_set_n(cpu, false);
    cpu_set_z(cpu, value == 0);
    cpu_set_c(cpu, value > 0xFF);
    cpu_set_h(cpu, half > 0xF);

    cpu->A = value;
}

void cpu_add_carry(cpu_t* cpu, unsigned int n) {
    cpu_add(cpu, n += cpu_get_c(cpu));
}

void cpu_add16(cpu_t* cpu, uint16_t n)
{
    unsigned int value = cpu->HL + n;

    cpu_set_n(cpu, false);
    cpu_set_c(cpu, value > 0xFFFF);
    cpu_set_h(cpu, (cpu->HL & 0xFF) + (n & 0xFF) > 0xFF);

    cpu->HL = value;
}

void cpu_sub(cpu_t* cpu, unsigned int n)
{
    unsigned int value = cpu->A - n;

    cpu_set_n(cpu, true);
    cpu_set_z(cpu, value == 0);
    cpu_set_c(cpu, n > cpu->A);
    cpu_set_h(cpu, (n & 0xF) > (cpu->A & 0xF));

    cpu->A = value;
}

void cpu_sub_carry(cpu_t* cpu, unsigned int n)
{
    cpu_sub(cpu, n + cpu_get_c(cpu));
}

void cpu_inc(cpu_t* cpu, uint8_t* r)
{
    cpu_set_h(cpu, (*r & 0xF) + 1 > 0xF);
    cpu_set_n(cpu, false);

    *r += 1;
    cpu_set_z(cpu, *r == 0);
}

void cpu_dec(cpu_t* cpu, uint8_t* r)
{
    cpu_set_n(cpu, true);
    cpu_set_h(cpu, (*r & 0xF) == 0);

    *r -= 1;
    cpu_set_z(cpu, *r == 0);
}

void cpu_and(cpu_t* cpu, uint8_t n)
{
    cpu->A &= n;

    cpu_set_z(cpu, cpu->A == 0);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, false);
    cpu_set_h(cpu, true);
}

void cpu_or(cpu_t* cpu, uint8_t n)
{
    cpu->A |= n;

    cpu_set_z(cpu, cpu->A == 0);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, false);
    cpu_set_h(cpu, false);
}

void cpu_xor(cpu_t* cpu, uint8_t n)
{
    cpu->A ^= n;

    cpu_set_z(cpu, cpu->A == 0);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, false);
    cpu_set_h(cpu, false);
}

void cpu_cp(cpu_t* cpu, uint8_t n)
{
    int value = cpu->A - n;

    cpu_set_z(cpu, value == 0);
    cpu_set_n(cpu, true);
    cpu_set_c(cpu, value < 0);
    cpu_set_h(cpu, false);
}

void cpu_swap(cpu_t* cpu, uint8_t* r)
{
    *r = ((*r & 0xF) << 4) | ((*r & 0xF0) >> 4);

    cpu_set_z(cpu, *r == 0);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, false);
    cpu_set_h(cpu, false);
}

void cpu_rotate_l(cpu_t* cpu, uint8_t* r)
{
    unsigned int msb = (*r & 0x80) >> 7;

    *r = (*r << 1) | msb;

    cpu_set_z(cpu, false);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, msb);
    cpu_set_h(cpu, false);
}

void cpu_rotate_l_carry(cpu_t* cpu, uint8_t* r)
{
    unsigned int msb = (*r & 0x80) >> 7;

    *r = (*r << 1) | cpu_get_c(cpu);

    cpu_set_z(cpu, false);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, msb);
    cpu_set_h(cpu, false);
}

void cpu_rotate_r(cpu_t* cpu, uint8_t* r)
{
    unsigned int lsb = *r & 1;

    *r = (lsb << 7) | (*r >> 1);

    cpu_set_z(cpu, false);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, lsb);
    cpu_set_h(cpu, false);
}

void cpu_rotate_r_carry(cpu_t* cpu, uint8_t* r)
{
    unsigned int lsb = *r & 1;

    *r = (cpu_get_c(cpu) << 7) | (*r >> 1);

    cpu_set_z(cpu, false);
    cpu_set_n(cpu, false);
    cpu_set_c(cpu, lsb);
    cpu_set_h(cpu, false);
}

void cpu_shift_l(cpu_t* cpu, uint8_t* r)
{
    cpu_set_c(cpu, *r & 0x80);
    cpu_set_n(cpu, false);
    cpu_set_h(cpu, false);

    *r <<= 1;

    cpu_set_z(cpu, *r == 0);
}

void cpu_shift_r_arithm(cpu_t* cpu, uint8_t* r)
{
    unsigned int msb = *r & 0x80;

    cpu_set_c(cpu, *r & 0x01);
    cpu_set_n(cpu, false);
    cpu_set_h(cpu, false);

    *r = msb | (*r >> 1);

    cpu_set_z(cpu, *r == 0);
}

void cpu_shift_r_logic(cpu_t* cpu, uint8_t* r)
{
    cpu_set_c(cpu, *r & 0x01);
    cpu_set_n(cpu, false);
    cpu_set_h(cpu, false);

    *r >>= 1;

    cpu_set_z(cpu, *r == 0);
}

void cpu_daa(cpu_t* cpu)
{
    // We need t to be > 16 bits to hold the carry bit
    unsigned int t = cpu->A;
    
    // Thank you, MAME
    if (cpu_get_n(cpu)) {
        // Last operation was a subtraction
        if (cpu_get_h(cpu))
        {
            t -= 0x6;
            
            if (!cpu_get_c(cpu))
            {
                t &= 0xFF;
            }
        }
        
        if (cpu_get_c(cpu))
        {
            t -= 0x60;
        }
    } else {
        // Last operation was an addition
        if ((t & 0xF) > 0x9 || cpu_get_h(cpu)) {
            t += 0x6;
        }
        
        if (t > 0x9F || cpu_get_c(cpu)) {
            t += 0x60;
        }
    }
    
    cpu->A = t & 0xFF;
    cpu_set_c(cpu, t > 0xFFFF);
    cpu_set_z(cpu, cpu->A == 0);
    cpu_set_h(cpu, false);
}

// __ Flow control ______________________________

void cpu_jump(context_t *ctx)
{
    ctx->cpu.PC = mem_read16(&ctx->mem, ctx->cpu.PC);
}

void cpu_jump_rel(context_t *ctx)
{
    ctx->cpu.PC += ((int8_t)ctx->mem.map[ctx->cpu.PC]) + 1;
}

void cpu_call(context_t *ctx)
{
    cpu_push(ctx, ctx->cpu.PC + 2);
    ctx->cpu.PC = mem_read16(&ctx->mem, ctx->cpu.PC);
}

void cpu_restart(context_t *ctx, uint8_t n)
{
    cpu_push(ctx, ctx->cpu.PC);
    ctx->cpu.PC = n;
}

void cpu_return(context_t *ctx)
{
    ctx->cpu.PC = cpu_pop(ctx);
}
