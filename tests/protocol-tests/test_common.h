/* Tiny assertion harness for the host-side protocol tests. */
#ifndef TEST_COMMON_H
#define TEST_COMMON_H
#include <stdio.h>
#include <stdlib.h>

static int g_checks, g_failures;

#define CHECK(cond) do { \
    g_checks++; \
    if (!(cond)) { g_failures++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define TEST_MAIN_END(name) \
    printf("%s: %d checks, %d failures\n", name, g_checks, g_failures); \
    return g_failures ? 1 : 0

/* deterministic PRNG so failures are reproducible */
static unsigned g_seed = 12345u;
static unsigned test_rand(void) { g_seed = g_seed * 1664525u + 1013904223u; return g_seed >> 8; }
#endif
