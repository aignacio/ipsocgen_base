#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;
const byte interruptPin = 2;
volatile byte start_measuring = LOW;
  int g_counter = 1;
unsigned long sample = 0;

void setup(void) 
{
  Serial.begin(115200);
  while (!Serial) {
      delay(1);
  }

  uint32_t currentFrequency;
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
}
  
void start_meas(void){
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;
  float iter = 1;
  unsigned long timestamp = millis();
  
  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  
  // Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  // Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  // Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  // Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  // Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
  
  for(int i=0; i<10; i++) {
    power_mW += ina219.getPower_mW();
    iter++;
  }
  Serial.print(sample);
  Serial.print(",");
  Serial.print(round(power_mW/iter));
  Serial.print(",");
  delay(100);
  sample++;
}

void loop(void) { 
  start_meas();
}
