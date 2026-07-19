/*
 * Basic RFA archive tests
 */

#include "../dsr/dsr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int passed = 0;
static int failed = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "FAIL: %s (line %d)\n", name, __LINE__); \
        failed++; \
    } else { \
        passed++; \
    } \
} while(0)

static void test_rfa_open_nonexistent(void) {
    rfa_archive rfa;
    rfa_error err = rfa_open(&rfa, "/nonexistent/file.rfa");
    TEST("open nonexistent", err == RFA_ERR_OPEN);
    if (err == RFA_OK) rfa_close(&rfa);
}

static void test_rfa_header(void) {
    /* Create a minimal valid RFA in memory and write to temp file */
    /* For now, skip since we need a real RFA file */
    printf("SKIP: test_rfa_header (requires real .rfa file)\n");
}

static void test_core_types(void) {
    arena a = arena_create(4096);
    TEST("arena created", a.base != NULL);

    int *p = arena_alloc_s(&a, int);
    TEST("arena alloc int", p != NULL);
    *p = 42;
    TEST("arena int value", *p == 42);

    void *p2 = arena_alloc(&a, 100, 8);
    TEST("arena alloc aligned", p2 != NULL);

    arena_reset(&a);
    TEST("arena reset", a.offset == 0);

    arena_destroy(&a);
    TEST("arena destroyed", a.base == NULL);
}

static void test_tick_loop(void) {
    tick_loop tl = tick_loop_make(30.0);
    TEST("tick rate 30Hz", tl.tick_rate == 30.0);
    TEST("dt 1/30", tl.dt == 1.0 / 30.0);

    int r = tick_loop_advance(&tl, 1.0 / 60.0);
    TEST("advance 1/60s", r == 0);
    TEST("tick_id still 0", tl.tick_id == 0);

    r = tick_loop_advance(&tl, 1.0 / 60.0);
    TEST("advance another 1/60s = 1 tick", r == 1);
    TEST("tick_id now 1", tl.tick_id == 1);
}

static void test_entity_array(void) {
    entity_array ea = entity_array_create(10);
    TEST("entity array created", ea.count == 0);
    TEST("entity capacity 10", ea.capacity == 10);

    entity_id e1 = entity_spawn(&ea, OBJ_SIMPLE, vec3_make(1,2,3));
    TEST("spawn entity 1", e1 != ENTITY_NULL);
    TEST("count 1", ea.count == 1);
    TEST("position x=1", ea.positions[0].x == 1.0f);

    entity_id e2 = entity_spawn(&ea, OBJ_VEHICLE, vec3_make(4,5,6));
    TEST("spawn entity 2", e2 != ENTITY_NULL);
    TEST("count 2", ea.count == 2);

    entity_despawn(&ea, e1);
    TEST("despawn e1", ea.count == 1);
    TEST("remaining entity is e2", ea.ids[0] == e2);

    entity_array_destroy(&ea);
    TEST("array destroyed", ea.count == 0 && ea.ids == NULL);
}

int main(void) {
    printf("DSO-Refractor Test Suite\n");
    printf("========================\n\n");

    test_core_types();
    test_tick_loop();
    test_entity_array();
    test_rfa_open_nonexistent();

    printf("\nResults: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
