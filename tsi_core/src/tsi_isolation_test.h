#ifndef TSI_ISOLATION_TEST_H_
#define TSI_ISOLATION_TEST_H_

#include <stdbool.h>
#include <stdint.h>

extern volatile uint32_t tsi_iso_fault_caught;

int tsi_isolation_run_tests(bool expect_secure_isolation);

#endif /* TSI_ISOLATION_TEST_H_ */
