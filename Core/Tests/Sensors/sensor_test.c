#include "../Test/test.h"
#include "../../Inc/Sensors/AnalogSensor.h"
#include "../../Inc/Sensors/DigitalSensor.h"

void test_analog_sensor_init(void) {
    AnalogSensor sensor;
    
    AnalogSensorInit(&sensor, "TestAnalog", 100, 0, 4095, 0.0, 100.0);
    
    TEST_ASSERT_STR_EQ("TestAnalog", sensor.base.name, "Analog sensor name set");
    TEST_ASSERT_EQ(100, sensor.base.hz, "Analog sensor frequency set");
    TEST_ASSERT_EQ(0, sensor.min_raw, "Analog sensor min raw set");
    TEST_ASSERT_EQ(4095, sensor.max_raw, "Analog sensor max raw set");
    TEST_ASSERT_FLOAT_EQ(0.0, sensor.min_scaled, 0.01, "Analog sensor min scaled set");
    TEST_ASSERT_FLOAT_EQ(100.0, sensor.max_scaled, 0.01, "Analog sensor max scaled set");
}

void test_analog_sensor_scaling(void) {
    AnalogSensor sensor;
    AnalogSensorInit(&sensor, "TestAnalog", 100, 0, 1000, 0.0, 10.0);
    
    // Test mid-scale
    AnalogSensorSetRaw(&sensor, 500);
    float scaled = AnalogSensorGetScaled(&sensor);
    TEST_ASSERT_FLOAT_EQ(5.0, scaled, 0.01, "Mid-scale conversion correct");
    
    // Test min scale
    AnalogSensorSetRaw(&sensor, 0);
    scaled = AnalogSensorGetScaled(&sensor);
    TEST_ASSERT_FLOAT_EQ(0.0, scaled, 0.01, "Min-scale conversion correct");
    
    // Test max scale
    AnalogSensorSetRaw(&sensor, 1000);
    scaled = AnalogSensorGetScaled(&sensor);
    TEST_ASSERT_FLOAT_EQ(10.0, scaled, 0.01, "Max-scale conversion correct");
}

void test_digital_sensor_init(void) {
    DigitalSensor sensor;
    
    DigitalSensorInit(&sensor, "TestDigital", 50);
    
    TEST_ASSERT_STR_EQ("TestDigital", sensor.base.name, "Digital sensor name set");
    TEST_ASSERT_EQ(50, sensor.base.hz, "Digital sensor frequency set");
    TEST_ASSERT_EQ(0, sensor.state, "Digital sensor initial state false");
}

void test_digital_sensor_state(void) {
    DigitalSensor sensor;
    DigitalSensorInit(&sensor, "TestDigital", 50);
    
    // Test setting state
    DigitalSensorSetState(&sensor, 1);
    TEST_ASSERT_EQ(1, DigitalSensorGetState(&sensor), "Digital sensor state set to true");
    
    DigitalSensorSetState(&sensor, 0);
    TEST_ASSERT_EQ(0, DigitalSensorGetState(&sensor), "Digital sensor state set to false");
}

void sensor_main(void) {
    TEST_SUITE_START("Sensor");
    
    test_analog_sensor_init();
    test_analog_sensor_scaling();
    test_digital_sensor_init();
    test_digital_sensor_state();
    
    TEST_SUITE_END();
}