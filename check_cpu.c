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

extern void _add_sprite_to_table(sprite_table_t *table, uint32_t sprite);

// Graphics
START_TEST (test_gfx_sprite_table)
{
    sprite_table_t table;
    table.length = 0;
    memset(&(table.data), 0, sizeof(table.data));
    
    const uint32_t to_sort[] = {
        0x000A000A, 0x00090009, 0x00080008, 0x00070007, 0x00060006, 0x00050005, 0x00050004, 0x00040003, 0x00030002, 0x00020001, 0x00FF000B, 0x0008000C, 0
    };
    
    int i;
    for (i = 0; to_sort[i] != 0; i++) {
        _add_sprite_to_table(&table, to_sort[i]);
    }
    
    const uint32_t sorted[10] = {
        0x00020001, 0x00030002, 0x00040003, 0x00050005, 0x00050004, 0x00060006,0x00070007, 0x00080008, 0x0008000C, 0x00090009
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
