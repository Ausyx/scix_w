#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LittleFS.h>
#include <EEPROM.h>


const char* const ssid = "Ausyx";
const char* const password = "AusyxSolucoes";
const uint16_t esp32ServerPort = 80;
const int ONE_WIRE_BUS = 5;  // GPIO05
const int SENSOR_POWER_PIN = 2;  // GPIO02
const float TEMPERATURE_THRESHOLD = 1.2;  // Temperature variation to trigger data sending (in Celsius)
const unsigned long DEEP_SLEEP_INTERVAL = 60;  // Deep Sleep interval in seconds (60 seconds = 1 minute)
const unsigned long CONNECTION_CHECK_INTERVAL = 300;  // Connection check interval to verify everything is correct (300 seconds = 5 minutes)
const int VERIFICATION_COUNT_THRESHOLD = 5;  // Number of checks before mandatory connection and sending
const char* COUNT_FILE = "/connectionCount.txt";

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiClient client;
IPAddress Server(192, 168, 4, 1);
unsigned int verificationCount = 0;

float lastTempC = DEVICE_DISCONNECTED_C;
unsigned long lastConnectionTime = 0;
unsigned long sendCount = 0;

DeviceAddress tempDeviceAddress;
String toSend;

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, LOW);
  WiFi.begin(ssid, password);

  EEPROM.begin(512);  //EEPROM do ESP32, memória que armazena valores

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  sensors.begin();
  sensors.getAddress(tempDeviceAddress, 0);

  // Read the first temperature
  readTemperature();

  lastTempC = EEPROM.read(10);
}

void loop() {
  // Increment the verification counter
  verificationCount++;

  // Turn on the sensor
  digitalWrite(SENSOR_POWER_PIN, HIGH);

  // Wait a moment for the sensor to stabilize (if necessary)
  delay(1000);

  // Read the temperature
  float tempC = readTemperature();

  // Check if the reading is valid before sending
  if (tempC != DEVICE_DISCONNECTED_C) {
    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.println("°C");

    // Turn off the sensor
    digitalWrite(SENSOR_POWER_PIN, LOW);
  } else {
    Serial.println("Sensor reading failed!");
    // In case of failure, turn off the sensor before entering deep sleep
    digitalWrite(SENSOR_POWER_PIN, LOW);
  }



  // Check if it's time for mandatory connection and sending
  if (verificationCount >= VERIFICATION_COUNT_THRESHOLD || 
      (tempC >= lastTempC + TEMPERATURE_THRESHOLD || tempC <= lastTempC - TEMPERATURE_THRESHOLD)) {

    float batteryPercent = readBatteryLevel();

    // Mandatory connection and sending
    envia_temp(tempC, batteryPercent);

    performHealthCheck();

  }

   lastTempC = tempC;

  EEPROM.put(10, lastTempC);
  EEPROM.commit();

  // Wait a moment before entering deep sleep
  delay(500);

  // Enter deep sleep for 1 minute
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_INTERVAL * 1000000);
  esp_deep_sleep_start();
}

void performHealthCheck() {
  verificationCount = 0;
}

float readTemperature() {
  digitalWrite(SENSOR_POWER_PIN, HIGH);
  delay(100);

  if (!sensors.getAddress(tempDeviceAddress, 0)) {
    Serial.println("Sensor not found!");
    digitalWrite(SENSOR_POWER_PIN, LOW);
    delay(100);
    return DEVICE_DISCONNECTED_C;
  }

  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  digitalWrite(SENSOR_POWER_PIN, LOW);
  delay(100);

  if (tempC != DEVICE_DISCONNECTED_C) {
    Serial.print("Read temperature: ");
    Serial.print(tempC);
    Serial.println("°C");
  } else {
    Serial.println("Sensor reading failed!");
  }

  return tempC;
}

//alou socorro
void envia_temp(float temperature, float batteryPercent) {
  if (client.connect(Server, esp32ServerPort)) {
    toSend = "GET /?sensor=9"; // Sensor number
    toSend += "&temp=";
    toSend += temperature * 100; // Multiply the temperature by 100 to send as integer
    toSend += "&batt=";   
    toSend += batteryPercent;
    toSend += " HTTP/1.1\r\n";
    toSend += "Host: ";
    toSend += WiFi.localIP().toString(); // IP address of the central ESP32
    toSend += "\r\nConnection: close\r\n\r\n";
    client.print(toSend);

    toSend = "";
    lastConnectionTime = millis();
    Serial.println("Temperature sent to the central!");
    Serial.print("Temperature: ");
    Serial.println(temperature);
  }
}

int readBatteryLevel() {
  int analogValue = analogRead(A0);
  // Converter o valor analógico para o percentual de bateria
  // Supondo que a leitura analógica varie de 0 a 1023 e corresponde a 0-100%
  int batteryPercent = map(analogValue, 0, 1023, 0, 100);
  return batteryPercent;
}