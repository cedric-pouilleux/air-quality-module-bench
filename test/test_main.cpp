#include <Arduino.h>
#include <unity.h>

// Example logic test
void test_sanity_check() {
    TEST_ASSERT_EQUAL_STRING("ESP32", "ESP32");
    TEST_ASSERT_TRUE(true);
}

// Example: could test data formatting logic if extracted
void test_data_format() {
    char buffer[32];
    int value = 42;
    snprintf(buffer, sizeof(buffer), "Value: %d", value);
    TEST_ASSERT_EQUAL_STRING("Value: 42", buffer);
}

void setup() {
    // Wait for board to be ready
    delay(2000);
    
    UNITY_BEGIN();
    RUN_TEST(test_sanity_check);
    RUN_TEST(test_data_format);
    UNITY_END();
}

void loop() {
    // Nothing to do here
}
