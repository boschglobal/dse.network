// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <dse/testing.h>
#include <dse/logger.h>
#include <dse/network/network.h>
#include <dse/modelc/schema.h>
#include <dse/clib/util/yaml.h>


uint8_t __log_level__ = LOG_QUIET; /* LOG_ERROR LOG_INFO LOG_DEBUG LOG_TRACE */


extern int run_parser_tests(void);
extern int run_loader_tests(void);
extern int run_engine_tests(void);
extern int run_function_tests(void);


int main()
{
    __log_level__ = LOG_QUIET;  // LOG_DEBUG;//LOG_QUIET;

    int rc = 0;
    rc |= run_loader_tests();
    rc |= run_parser_tests();
    rc |= run_engine_tests();
    rc |= run_function_tests();
    return rc;
}
