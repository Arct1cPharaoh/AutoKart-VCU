#include "../Test/test.h"
#include "../../Inc/Outputs/AnalogOutput.h"
#include "../../Inc/Outputs/DigitalOutput.h"

void test_analog_output_init(void) {
    AnalogOutput output;
    
    AnalogOutputInit(&output, "TestAnalogOut", 0.0, 100.0, 0, 4095);
    
    TEST_ASSERT_STR_EQ("TestAnalogOut", output.base.name, "Analog output name set");
    TEST_ASSERT_FLOAT_EQ(0.0, output.min_value, 0.01, "Analog output min value set");
    TEST_ASSERT_FLOAT_EQ(100.0, output.max_value, 0.01, "Analog output max value set");
    TEST_ASSERT_EQ(0, output.min_raw, "Analog output min raw set");
    TEST_ASSERT_EQ(4095, output.max_raw, "Analog output max raw set");
}

void test_analog_output_scaling(void) {
    AnalogOutput output;
    AnalogOutputInit(&output, "TestAnalogOut", 0.0, 10.0, 0, 1000);
    
    // Test mid-scale
    AnalogOutputSetValue(&output, 5.0);
    int raw = AnalogOutputGetRaw(&output);
    TEST_ASSERT_EQ(500, raw, "Mid-scale output conversion correct");
    
    // Test min scale
    AnalogOutputSetValue(&output, 0.0);
    raw = AnalogOutputGetRaw(&output);
    TEST_ASSERT_EQ(0, raw, "Min-scale output conversion correct");
    
    // Test max scale
    AnalogOutputSetValue(&output, 10.0);
    raw = AnalogOutputGetRaw(&output);
    TEST_ASSERT_EQ(1000, raw, "Max-scale output conversion correct");
}

void test_digital_output_init(void) {
    DigitalOutput output;
    
    DigitalOutputInit(&output, "TestDigitalOut");
    
    TEST_ASSERT_STR_EQ("TestDigitalOut", output.base.name, "Digital output name set");
    TEST_ASSERT_EQ(0, output.state, "Digital output initial state false");
}

void test_digital_output_state(void) {
    DigitalOutput output;
    DigitalOutputInit(&output, "TestDigitalOut");
    
    // Test setting state
    DigitalOutputSetState(&output, 1);
    TEST_ASSERT_EQ(1, DigitalOutputGetState(&output), "Digital output state set to true");
    
    DigitalOutputSetState(&output, 0);
    TEST_ASSERT_EQ(0, DigitalOutputGetState(&output), "Digital output state set to false");
}

void output_main(void) {
    TEST_SUITE_START("Output");
    
    test_analog_output_init();
    test_analog_output_scaling();
    test_digital_output_init();
    test_digital_output_state();
    
    TEST_SUITE_END();
}