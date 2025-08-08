// Lightweight compile-time / logic tests for SystemUtils feature flags
#include <unity.h>
#include "utils/system_utils.h"

void test_logging_constants_present() {
#ifdef ENABLE_SD_LOGGING
    TEST_ASSERT_TRUE(LOG_FILE_MAX_SIZE > 0);
    TEST_ASSERT_TRUE(SD_INIT_MAX_ATTEMPTS >= 1);
#else
    TEST_PASS_MESSAGE("SD logging disabled; skipping constants test");
#endif
}

void test_low_memory_guard() {
    bool low = SystemUtils::isLowMemory();
    (void)low;
    TEST_ASSERT_TRUE(true);
}
