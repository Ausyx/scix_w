#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LittleFS.h>
#include <EEPROM.h>

#define TEMP_ADDR_MEM_EEPROM 10
#define SIZE_MEM_EEPROM 512

//Porta sensor de temperatura 
#define SENSOR_TEMP_DATA 5 //GPIO05
#define SENSOR_POWER_PIN 2 //GPIO02

//Conexão com Central
const char* const ssid = "Ausyx";
const char* const password = "AusyxSolucoes";
const uint16_t esp32ServerPort = 80;

const float TEMPERATURE_THRESHOLD = 1.2;  // Temperature variation to trigger data sending (in Celsius)
const unsigned long DEEP_SLEEP_INTERVAL = 60;  // Deep Sleep interval in seconds (60 seconds = 1 minute)
const unsigned long CONNECTION_CHECK_INTERVAL = 300;  // Connection check interval to verify everything is correct (300 seconds = 5 minutes)
const int VERIFICATION_COUNT_THRESHOLD = 5;  // Number of checks before mandatory connection and sending
const char* COUNT_FILE = "/connectionCount.txt";

OneWire oneWire(SENSOR_TEMP_DATA);
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

  EEPROM.begin(SIZE_MEM_EEPROM);  //EEPROM do ESP32, memória que armazena valores

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  sensors.begin();
  sensors.getAddress(tempDeviceAddress, 0);

  // Read the first temperature
  readTemperature();

  lastTempC = EEPROM.read(TEMP_ADDR_MEM_EEPROM);
  
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

    // Mandatory connection and sending
    //envia_temp(tempC);
    //envia_temp(125);
    performHealthCheck();

  }

   lastTempC = tempC;

  EEPROM.put(TEMP_ADDR_MEM_EEPROM, lastTempC);
  EEPROM.commit();

  // Wait a moment before entering deep sleep
  envia_temp(0);
  delay(1000);
  envia_temp(25);
  delay(1000);
  envia_temp(50);
  delay(1000);
  envia_temp(75);

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
void envia_temp(float temperature) {
  if (client.connect(Server, esp32ServerPort)) {
    toSend = "GET 192.168.4.1/TE="; // Sensor numberGET 192.168.4.1/TM=
    toSend += "&temp=";
    toSend += temperature * 100; // Multiply the temperature by 100 to send as integer
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
