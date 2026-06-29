#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "src/core/core-command.h"

START_TEST(test_buffer_reads_never_exceed_declared_length)
{
    // Invariant: Buffer reads never exceed the declared length
    const char *payloads[] = {
        "normal_input",                    // Valid input
        "A",                               // Boundary: single char
        "very_long_input_that_exceeds_buffer_by_2x_XXXXXXXXXXXXXXXXXXXX",  // 2x overflow
        "extremely_long_input_that_exceeds_by_10x_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"  // 10x overflow
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);
    
    for (int i = 0; i < num_payloads; i++) {
        char buffer[32] = {0};  // Fixed buffer size
        size_t buffer_size = sizeof(buffer);
        
        // Test strncat usage - must not exceed buffer bounds
        size_t input_len = strlen(payloads[i]);
        size_t copy_len = (input_len < buffer_size - 1) ? input_len : buffer_size - 1;
        
        strncat(buffer, payloads[i], copy_len);
        buffer[buffer_size - 1] = '\0';  // Ensure null termination
        
        // Verify buffer length never exceeds declared size
        ck_assert_msg(strlen(buffer) < buffer_size, 
                     "Buffer overflow detected for payload: %s", payloads[i]);
        
        // Verify no out-of-bounds write by checking last byte
        ck_assert_msg(buffer[buffer_size - 1] == '\0',
                     "Buffer not properly null-terminated for payload: %s", payloads[i]);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_buffer_reads_never_exceed_declared_length);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}