//
//  check_cpu.c
//  spielbub
//
//  Created by Lorenz on 07.12.12.
//  Copyright (c) 2012 Lorenz. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <check.h>

#include "context.h"
#include "cpu.h"
#include "cpu_ops.h"
#include "graphics.h"
#include "hardware.h"

#define CONTEXT context_t context; context_t *ctx = &context; \
    context_create(ctx);
#define ENDCONTEXT context_destroy(ctx);

// CONTEXT
START_TEST (test_context_create)
{
    context_t ctx;
    fail_unless(context_create(&ctx), "context_create failed");
    context_destroy(&ctx);
}
END_TEST

START_TEST (test_context_init)
{
    context_t ctx;
    fail_unless(context_init(&ctx), "context_init failed");
    context_destroy(&ctx);
}
END_TEST

// CPU
START_TEST (test_cpu_init)
{
    CONTEXT
    cpu_init(ctx->cpu);
    cpu_t *cpu = ctx->cpu;
    
    fail_unless(cpu->AF.W == 0x01B0, "AF not initialized to 0x01B0");
    
    ENDCONTEXT
}
END_TEST

START_TEST (test_cpu_add)
{
    CONTEXT
    cpu_init(ctx->cpu);
    
    _A = 0;
    _add(ctx, 13, false);
    
    fail_unless(_A == 13, "_add failed, _A = %d", _A);
    fail_unless(GET_Z() == 0, "_add set Z flag");
    fail_unless(GET_N() == 0, "_add failed to reset N flag");
    // TODO: H set if carry from bit 3
    // TODO: C set if carry from bit 7
    
    _add(ctx, 244, false);
    fail_unless(_A == 1, "_add with overflow failed, _A = %d", _A);
    fail_unless(GET_C() == 1, "_add failed to set C flag");
    // TODO: GET_H()
    
    ENDCONTEXT
}
END_TEST

START_TEST (test_cpu_sub)
{
    CONTEXT
    cpu_init(ctx->cpu);
    
    _A = 100;
    _sub(ctx, 10, false);
    
    fail_unless(_A == 90, "_sub failed, _A = %d", _A);
    fail_unless(GET_Z() == 0, "_sub set Z flag");
    fail_unless(GET_N() == 1, "_sub failed to set N flag");
    fail_unless(GET_C() == 0, "_sub set C flag");
    
    _sub(ctx, 90, false);
    fail_unless(_A == 0, "_sub failed, _A = %d", _A);
    fail_unless(GET_Z() == 1, "_sub failed to set Z flag");
    
    _sub(ctx, 1, false);
    fail_unless(_A == 255, "_sub with underflow failed, _A = %d", _A);
    fail_unless(GET_C() == 1, "_sub failed to set C flag");
    
//    fail_unless(_A == 13, "_add failed, _A = %d", _A);
//    fail_unless(GET_Z() == 0, "_add set Z flag");
//    fail_unless(GET_N() == 0, "_add failed to reset N flag");
//    // TODO: H set if carry from bit 3
//    // TODO: C set if carry from bit 7
//    
//    _add(ctx, 244, false);
//    fail_unless(_A == 1, "_add with overflow failed, _A = %d", _A);
//    fail_unless(GET_C() == 1, "_add failed to set C flag");
//    // TODO: GET_H()
    
    ENDCONTEXT
}
END_TEST

// Graphics
START_TEST (test_gfx_sprite_t)
{
    sprite_t sprite = { .raw = 0};
    
    // These tests currently fail on big-endian.
    
    fail_unless(sizeof(sprite_t) == 4, "The compiler seems to pad sprite_t to != 4 bytes");
    
    sprite.y = 0xFF;
    sprite.x = 0xEE;
    sprite.tile_id = 0xDD;
    sprite.flags = 0xCC;
    
    const uint8_t check[] = { 0xFF, 0xEE, 0xDD, 0xCC };
    
    fail_unless(memcmp((void*)&sprite, (void*)check, 4) == 0, "sprite_t packing does not seem to work");
}
END_TEST


extern void _add_sprite_to_table(sprite_table_t *table, sprite_t sprite);
START_TEST (test_gfx_sprite_table)
{
    sprite_table_t table;
    table.length = 0;
    memset(&(table.data), 0, sizeof(table.data));
    
    sprite_t const to_sort[] = {
        { .b = { .x = 0xA, .flags = 0xA}},
        { .b = { .x = 0x9, .flags = 0x9}},
        { .b = { .x = 0x8, .flags = 0x8}},
        { .b = { .x = 0x7, .flags = 0x7}},
        { .b = { .x = 0x6, .flags = 0x6}},
        { .b = { .x = 0x5, .flags = 0x5}},
        { .b = { .x = 0x5, .flags = 0x4}},
        { .b = { .x = 0x4, .flags = 0x3}},
        { .b = { .x = 0x3, .flags = 0x2}},
        { .b = { .x = 0x2, .flags = 0x1}},
        { .b = { .x = 0xF, .flags = 0xB}},
        { .b = { .x = 0x8, .flags = 0xC}},
        { .raw = 0 }
    };
    
    int i;
    for (i = 0; to_sort[i].raw != 0; i++) {
        _add_sprite_to_table(&table, to_sort[i]);
    }
    
    const uint8_t sorted[SPRITES_PER_LINE][4] = {
        {0x00, 0x02, 0x00, 0x01},
        {0x00, 0x03, 0x00, 0x02},
        {0x00, 0x04, 0x00, 0x03},
        {0x00, 0x05, 0x00, 0x05},
        {0x00, 0x05, 0x00, 0x04},
        {0x00, 0x06, 0x00, 0x06},
        {0x00, 0x07, 0x00, 0x07},
        {0x00, 0x08, 0x00, 0x08},
        {0x00, 0x08, 0x00, 0x0C},
        {0x00, 0x09, 0x00, 0x09},
    };
    
    fail_unless(sizeof(table.data) == sizeof(sorted), "sprite_table_t is not of size %d but %d", sizeof(sorted), sizeof(table.data));
    fail_unless(memcmp(sorted, table.data, sizeof(sorted)) == 0, "sprite_table_t is not sorted properly");
}
END_TEST

Suite * spielbub_suite(void)
{
    Suite *s = suite_create("Spielbub");
    
    // Context test cases
    TCase *tc_context = tcase_create("Context");
    tcase_add_test(tc_context, test_context_create);
    //tcase_add_test(tc_context, test_context_init);
    
    suite_add_tcase(s, tc_context);
    
    // CPU test cases
    TCase *tc_cpu = tcase_create("CPU");
    tcase_add_test(tc_cpu, test_cpu_init);
    tcase_add_test(tc_cpu, test_cpu_add);
    tcase_add_test(tc_cpu, test_cpu_sub);
    
    suite_add_tcase(s, tc_cpu);
    
    // Graphics
    TCase *tc_graphics = tcase_create("Graphics");
    tcase_add_test(tc_graphics, test_gfx_sprite_t);
    tcase_add_test(tc_graphics, test_gfx_sprite_table);
    
    suite_add_tcase(s, tc_graphics);
    
    return s;
}

int main(void)
{
    int number_failed = 0;
    Suite *s = spielbub_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    
    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
