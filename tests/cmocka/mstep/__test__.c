// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <dse/testing.h>
#include <dse/logger.h>


extern int run_mstep_tests(void);
extern int run_schedule_tests(void);


int main()
{
    int rc = 0;
    rc |= run_mstep_tests();
    rc |= run_schedule_tests();
    return rc;
}
