#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>

#define IN1 15     //abrir wind
#define IN2 13     //fechar wind
#define Buzina 12  //buzina

int status1 = 0;
int status2 = 0;
int ligando = 0;

// Configurações Wi-Fi
const char* const ssid = "Ausyx";
const char* const password = "AusyxSolucoes";

// Configurações do servidor e porta
const uint16_t esp8266ServerPort = 80;
IPAddress Server(192, 168, 4, 1);

// Pinos e configurações do sensor
const int ONE_WIRE_BUS = 5;                           // GPIO0 (D3)
const int SENSOR_POWER_PIN = 4;                       // GPIO2 (D4)
const float TEMPERATURE_THRESHOLD = 1.2;              // Variação de temperatura para disparar envio de dados (em Celsius)
const unsigned long CONNECTION_CHECK_INTERVAL = 300;  // Intervalo de verificação de conexão (300 segundos = 5 minutos)
const int VERIFICATION_COUNT_THRESHOLD = 5;           // Número de verificações antes de envio obrigatório
const char* COUNT_FILE = "/connectionCount.txt";

// Instâncias de sensores e cliente Wi-Fi
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiClient client;
AsyncWebServer server(esp8266ServerPort);

// Variáveis globais
unsigned int verificationCount = 0;
float lastTempC = DEVICE_DISCONNECTED_C;
unsigned long lastConnectionTime = 0;
unsigned long sendCount = 0;
DeviceAddress tempDeviceAddress;
String toSend;

// Variáveis para o temporizador
unsigned long lastReceiveTime = 0;
const unsigned long receiveInterval = 100;  // Intervalo de 100 milissegundos

int tempMaxEntrada = 0;
int tempMaxMassa = 0;
int muteState0 = 0;
int muteState1 = 0;
int muteState2 = 0;
int muteState3 = 0;


void CALIBRA() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);  // fechar

  for (int l = 0; l <= 5000; l += 5) {
    Serial.println("fechando");
    Serial.print("l: ");
    Serial.println(l);
    delay(10);
  }

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);  // fechado

  status1 = 0;
  status2 = 1;
}

void setup() {
  Serial.begin(115200);
  ligando = 1;
  CALIBRA();

  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, LOW);
  pinMode(Buzina, OUTPUT);
  digitalWrite(Buzina, LOW);

  // Inicialização do Wi-Fi
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  WiFi.softAP(ssid, password);

  EEPROM.begin(512);  // Inicializa a EEPROM do ESP8266

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  sensors.begin();
  sensors.getAddress(tempDeviceAddress, 0);

  // Lê a primeira temperatura
  readTemperature();

  lastTempC = EEPROM.read(10);

  // Configuração do servidor para receber dados
  setupServer();

  // Inicia o servidor
  server.begin();
}

void loop() {
  unsigned long currentMillis = millis();

  // Chama a função de recebimento a cada 100 milissegundos
  if (currentMillis - lastReceiveTime >= receiveInterval) {
    lastReceiveTime = currentMillis;
    receiveData();
    Lenha_mode();
  }

  // Incrementa o contador de verificações
  verificationCount++;

  // Liga o sensor
  digitalWrite(SENSOR_POWER_PIN, HIGH);

  // Aguarda um momento para o sensor estabilizar
  delay(1000);

  // Lê a temperatura
  float tempC = readTemperature();

  // Verifica se a leitura é válida antes de enviar
  if (tempC != DEVICE_DISCONNECTED_C) {
    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.println("°C");

    // Desliga o sensor
    digitalWrite(SENSOR_POWER_PIN, LOW);
  } else {
    Serial.println("Sensor reading failed!");
    // Em caso de falha, desliga o sensor antes de entrar em deep sleep
    digitalWrite(SENSOR_POWER_PIN, LOW);
  }

  // Verifica se é hora de conexão e envio obrigatórios
  if (verificationCount >= VERIFICATION_COUNT_THRESHOLD || (tempC >= lastTempC + TEMPERATURE_THRESHOLD || tempC <= lastTempC - TEMPERATURE_THRESHOLD)) {

    // Conexão e envio obrigatórios
    sendTemperature(tempC);
    performHealthCheck();
  }

  lastTempC = tempC;
  EEPROM.put(10, lastTempC);
  EEPROM.commit();

  // Aguarda um momento antes de entrar em deep sleep
  delay(500);
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

void sendTemperature(float temperature) {
  if (client.connect(Server, esp8266ServerPort)) {
    toSend = "GET /?sensor=9";  // Número do sensor
    toSend += "&temp=";
    toSend += temperature * 100;  // Multiplica a temperatura por 100 para enviar como inteiro
    toSend += " HTTP/1.1\r\n";
    toSend += "Host: ";
    toSend += WiFi.localIP().toString();  // Endereço IP do ESP8266 central
    toSend += "\r\nConnection: close\r\n\r\n";
    client.print(toSend);

    toSend = "";
    lastConnectionTime = millis();
    Serial.println("Temperature sent to the central!");
    Serial.print("Temperature: ");
    Serial.println(temperature);
  }
}

void ABRINDO() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);  // abrir

  for (int i = 0; i <= 3000; i += 10) {
    Serial.println("abrindo");
    Serial.print("i: ");
    Serial.println(i);
    delay(15);
  }

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);  // aberto

  status1 = 1;
  status2 = 0;
}

void FECHANDO() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);  // fechar

  for (int l = 0; l <= 3500; l += 5) {
    Serial.println("fechando");
    Serial.print("l: ");
    Serial.println(l);
    delay(15);
  }

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);  // fechado

  status1 = 0;
  status2 = 1;
}

void Lenha_mode() {
  if ((lastTempC <= tempMaxEntrada) && (status2 == 0)) {
    FECHANDO();
  }

  if ((lastTempC > tempMaxEntrada) && (status1 == 0)) {
    ABRINDO();
  }

  if (muteState0 == 0 && lastTempC >= (tempMaxEntrada + 7)) {
    digitalWrite(Buzina, HIGH);
  } else if (muteState0) {
    digitalWrite(Buzina, LOW);
  }

  if (muteState2 == 0 && lastTempC > (tempMaxMassa + 7)) {
    digitalWrite(Buzina, HIGH);
  } else if (muteState2) {
    digitalWrite(Buzina, LOW);
  }

  if (muteState1) {
    digitalWrite(Buzina, LOW);
  }

  if (muteState3) {
    digitalWrite(Buzina, LOW);
  }
}

// Configuração do servidor para receber dados
void setupServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("sensor") && request->hasParam("tempMaxEntrada") && request->hasParam("tempMaxMassa") && request->hasParam("muteState0") && request->hasParam("muteState1") && request->hasParam("muteState2") && request->hasParam("muteState3")) {

      String sensorNumber = request->getParam("sensor")->value();
      String tempMaxEntrada = request->getParam("tempMaxEntrada")->value();
      String tempMaxMassa = request->getParam("tempMaxMassa")->value();
      String muteState0 = request->getParam("muteState0")->value();
      String muteState1 = request->getParam("muteState1")->value();
      String muteState2 = request->getParam("muteState2")->value();
      String muteState3 = request->getParam("muteState3")->value();

      // Imprime os valores recebidos no Serial Monitor
      Serial.println("Dados recebidos:");
      Serial.println("Sensor: " + sensorNumber);
      Serial.println("Temp Max Entrada: " + tempMaxEntrada);
      Serial.println("Temp Max Massa: " + tempMaxMassa);
      Serial.println("Mute State 0: " + muteState0);
      Serial.println("Mute State 1: " + muteState1);
      Serial.println("Mute State 2: " + muteState2);
      Serial.println("Mute State 3: " + muteState3);

      request->send(200, "text/plain", "Dados recebidos com sucesso!");
    } else {
      request->send(400, "text/plain", "Parâmetros inválidos");
    }
  });
}

// Função para receber dados
void receiveData() {
  String req = "";  // Aqui você precisa definir a string recebida ou o objeto de requisição, dependendo do seu contexto

  // Exemplo de lógica para processar os parâmetros recebidos
  if (req.indexOf("Central=") != -1 && req.indexOf("tempMaxEntrada=") != -1 && req.indexOf("tempMaxMassa=") != -1 && req.indexOf("muteState0=") != -1 && req.indexOf("muteState1=") != -1 && req.indexOf("muteState2=") != -1 && req.indexOf("muteState3=") != -1) {

    int CentralNumber = getParamValue(req, "Central").toInt();
    tempMaxEntrada = getParamValue(req, "tempMaxEntrada").toInt();
    tempMaxMassa = getParamValue(req, "tempMaxMassa").toInt();
    muteState0 = getParamValue(req, "muteState0").toInt();
    muteState1 = getParamValue(req, "muteState1").toInt();
    muteState2 = getParamValue(req, "muteState2").toInt();
    muteState3 = getParamValue(req, "muteState3").toInt();

    // Aqui você processa os dados recebidos conforme necessário
    // Por exemplo:
    Serial.println("Dados recebidos:");
    Serial.print("Sensor: ");
    Serial.println(CentralNumber);
    Serial.print("Temp Max Entrada: ");
    Serial.println(tempMaxEntrada);
    Serial.print("Temp Max Massa: ");
    Serial.println(tempMaxMassa);
    Serial.print("Mute State 0: ");
    Serial.println(muteState0);
    Serial.print("Mute State 1: ");
    Serial.println(muteState1);
    Serial.print("Mute State 2: ");
    Serial.println(muteState2);
    Serial.print("Mute State 3: ");
    Serial.println(muteState3);

    // Agora você pode realizar qualquer outra lógica de aplicação necessária com os dados recebidos
  }
}

String getParamValue(String req, String param) {
  delay(10);
  int startIndex = req.indexOf(param + "=");
  if (startIndex == -1) {
    return "";
  }
  startIndex += param.length() + 1;
  int endIndex = req.indexOf("&", startIndex);
  if (endIndex == -1) {
    endIndex = req.length();
  }
  return req.substring(startIndex, endIndex);
}