#include <Wire.h>
#include <Adafruit_INA219.h>
#include <esp_bt.h>
#include <driver/gpio.h>

#define SDA_PIN 18
#define SCL_PIN 19
#define RELAY_PIN 23

#define VOLTAGE_OFF 12.2   // Desliga rele abaixo desse valor
#define VOLTAGE_ON  12.6   // Liga rele acima desse valor (histerese)

#define SLEEP_RELAY_ON_S  60  // Intervalo de check com rele ligado
#define SLEEP_RELAY_OFF_S 30  // Intervalo de check com rele desligado

// Persiste entre deep sleeps
RTC_DATA_ATTR bool relayState = false;

void deepSleep(uint32_t seconds) {
  // Mantém o estado do GPIO durante deep sleep
  gpio_hold_en((gpio_num_t)RELAY_PIN);
  gpio_deep_sleep_hold_en();
  esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  // Desliga Bluetooth pra economizar
  esp_bt_controller_disable();

  // Solta o hold do GPIO pra poder alterar o estado
  gpio_hold_dis((gpio_num_t)RELAY_PIN);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);

  // Init I2C e INA219
  Wire.begin(SDA_PIN, SCL_PIN);
  Adafruit_INA219 ina219;

  if (!ina219.begin()) {
    Serial.println("INA219 nao encontrado! Sleep 10s...");
    deepSleep(10);
  }

  // Le tensao
  float busVoltage = ina219.getBusVoltage_V();
  float shuntVoltage = ina219.getShuntVoltage_mV() / 1000.0;
  float loadVoltage = busVoltage + shuntVoltage;

  // Decide estado do rele com histerese
  if (loadVoltage < VOLTAGE_OFF) {
    relayState = false;
  } else if (loadVoltage > VOLTAGE_ON) {
    relayState = true;
  }
  // Entre VOLTAGE_OFF e VOLTAGE_ON: mantem estado anterior

  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);

  Serial.printf("VIN+: %.3fV | Rele: %s | Sleep: %ds\n",
    loadVoltage,
    relayState ? "ON" : "OFF",
    relayState ? SLEEP_RELAY_ON_S : SLEEP_RELAY_OFF_S);

  // Deep sleep
  uint32_t sleepTime = relayState ? SLEEP_RELAY_ON_S : SLEEP_RELAY_OFF_S;
  deepSleep(sleepTime);
}

void loop() {
  // Nunca chega aqui — deep sleep reseta e roda setup() de novo
}
