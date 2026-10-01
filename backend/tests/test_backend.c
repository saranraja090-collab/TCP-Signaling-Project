/**
 * @file test_backend.c
 * @brief Standalone test binary entry point for C Networking Backend.
 */

#include <stdio.h>
#include <string.h>

#include "packet.h"
#include "encoder.h"
#include "error_control.h"
#include "sequence_method.h"
#include "window_method.h"
#include "timing_method.h"
#include "decoder.h"
#include "verification.h"
#include "unit_tests.h"

int main(void) {
    return run_backend_unit_tests();
}
