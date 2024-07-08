#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <RtcDS3231.h>
#include <EEPROM.h>
#include <WebSocketsServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>
#include <UnicViewAD.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>


//inverti as portas 18 e 19
#define OUT_QUEIMADOR 18  //ch3
#define OUT_TEMPO 5       // ch2
#define IN1 15     //abrir wind
#define IN2 13     //fechar wind
#define KeyInt1 22
#define KeyInt2 23
#define keyPACT1 33
#define keyPACT2 32
#define BUZINA 34

int status1 = 0;
int status2 = 0;
int ligando = 0;
bool iniciou = false;
bool changePassword = false;
bool changeLogin = false;
int bateria = 0;


RtcDS3231<TwoWire> Rtc(Wire);

WiFiServer server(80);

WiFiClient client;

const char* centralServer = "192.168.4.2";  // IP da entrada
const int centralPort = 80;
DeviceAddress tempDeviceAddress;
String toSend;

SemaphoreHandle_t xSemaphore;  //Semáforo para que possamos alternar entre as taks sem nenhum problema

const char* googleScriptURL = "https://script.google.com/macros/s/AKfycbzzD5pRw_Zeq5x2QlVsVxs4JFIWPRjMK5fefakBvhcOb8dQIz6bhyoCC3cazeeh9mHknQ/exec";

//variáveis do display

LCM Lcm(Serial2);
LcmString cronometer(1, 48);  //Cronômetro em que vai ser imprimido o tempo no formato de texto para ter os dois pontos, o 1 representa o VP e o 48 o tamanho

LcmString status(50, 48);  //status do processo ativo ou em repouso

LcmVar Wifi(100);

LcmVar on(110);  //tempo ativo do secador no display
LcmVar of(115);  //tempo em repouso do secador no display

LcmVar iniciar(120);  //botão para dar play na contagem
LcmVar tempE(130);   //temperatura da entrada
LcmVar tempM(140);   //temperatura da Massa

LcmVar tempMaxE(132);   //temperatura máxima da entrada
LcmVar tempMinE(134);   //temperatura mínima da entrada
LcmVar tempMaxM(142);  //temperatura maáxima da massa
LcmVar tempMinM(144);  //temperatura máxima da massa

LcmVar connectIconEnt(150);   //ícone de conexão da Entrada
LcmVar connectIconMass(160);  //ícone de conexão da Massa

LcmVar batteryIconEnt(170);   //ícone que mostra a bateria do sensor da entrada
LcmVar batteryIconMass(180);  //ícone que mostra a bateria do sensor da massa

LcmVar tempExt(185);  //temperatura externa vinda do RTC
LcmVar hora(190);     //hora que aparece no display
LcmVar minuto(200);   //minuto que aparece no display
LcmVar dia(210);      //dia que aparece no display
LcmVar mes(220);      //mes que aparece no display
LcmVar ano(230);      //ano que aparece no display

LcmVar horaSet(190);    //hora que aparece no display
LcmVar minutoSet(200);  //minuto que aparece no display
LcmVar diaSet(210);     //dia que aparece no display
LcmVar mesSet(220);     //mes que aparece no display
LcmVar anoSet(230);     //ano que aparece no display
LcmVar changeDataDisp(240); 

int changeData = 0;
int changeWifi = 0;
int batteryLevel = 0;

LcmVar icon1(350);
LcmVar Sensor1(360);
LcmVar icon2(370);
LcmVar Sensor2(380);
LcmVar icon3(390);
LcmVar Sensor3(400);
LcmVar icon4(410);
LcmVar Sensor4(420);

LcmVar mute1(430);
LcmVar mute2(440);
LcmVar mute3(450);
LcmVar mute4(460);

LcmString Senha(480, 48);  //Cronômetro em que vai ser imprimido o tempo no formato de texto para ter os dois pontos, o 1 representa o VP e o 48 o tamanho
LcmString Login(550, 48);  //Cronômetro em que vai ser imprimido o tempo no formato de texto para ter os dois pontos, o 1 representa o VP e o 48 o tamanho

LcmVar WifiChange(600);

LcmVar EntradaConnect(610);
LcmVar MassaConnect(620);
LcmVar NoConnect(630);

LcmVar sensorNumberDisp(720);


int tempMaxEntrada;        //--> Temperatura máxima turbina
int tempMinEntrada;        //--> Temperatura mínima turbina
int tempMaxMassa;          //--> Temperatura máxima massa
int tempMinMassa;          //--> Temperatura mínima massa
int ON = 1;                //--> Tempo ligado
int OF = 1;                //--> Tempo desligado


int tempMaxEntrada2;        //--> Temperatura máxima turbina
int tempMinEntrada2;        //--> Temperatura mínima turbina
int tempMaxMassa2;          //--> Temperatura máxima massa
int tempMinMassa2;          //--> Temperatura mínima massa
int ON2 = 1;                //--> Tempo ligado
int OF2 = 1;                //--> Tempo desligado


int elapsedTime = 0;       //--> Tempo decorrido do processo;
int elapsedPauseTime = 0;  //--> Tempo de pausa decorrido;
int lastUpdateTime = 0;    //--> ultima vez que atualizou as datas na tela
float temperatureRTC = 0.0;     //--> Temperatura do RTC em tempo real;
bool isOn = false;              //--> confere se está em ligado ou desligado;
bool Stop = true;               //-->status no botão pause da intermitência;
unsigned long startTime = 0;    //--> Tempo em que se iniciou o processo;
unsigned long startTime2 = 0;   //--> Tempo em que se iniciou o processo;

//TELA 1
int getStart;        //iniciar processo (START PROCESS)
int getGoSettings;   //Botao tela de ajuste
int getGoData;       //botão tela de data e hora
int getHour;         //Hora do processo
int getMinute;       //minuto do processo
int getDay;          //pega o dia do processo
int getMonth;        //pega o mês do processo
int getYear;         //pega o ano que o processo está acontecendo
int progress = 0;    //progresso da barra de tempo
int queimador = 0;   //estado do queimador, desligado ou ligado
int interStats = 0;  //estado da intermitencia, se está ativada ou se ela foi desativada pela chave

unsigned long currentTime;  //tempo atual do processo

int lastReadTempSensor;  //ultima leitura do sensor
int getActivityTime;     //tempo em que ocorre a fase de ligado na intermitencia
int getRestTime;         //variável para pegar o tempo de em que o processo de intermitência estava pausado

int ConnectVal = 0;  //também funcionam na página 1, servem para mudar o ícone de conectado no display
int ConnectVal2 = 0;

int year; //ano do processo que vai ser pego e salvo
int month; //mês do processo que vai ser pego e salvo
int day; //dia do processo que vai ser pego e salvo
int hour; //hora do processo que vai ser pego e salvo
int minute; //minuto do processo que vai ser pego e salvo



//variáveis da tela de avisos
int getMuteEH;  //sair da tela de aviso entrada Alta
int getMuteEL;  //sair da tela de aviso entrada Baixa
int getMuteMH;  //sair da tela de aviso massa alta
int getMuteML;  //sair da tela de aviso massa baixa

//estado em que se encontra os avisos, caso ele esteja em 1 significa que o produtor já pressionou o botão de mutar e as temperaturas ainda não normalizaram
int muteState0 = 0;
int muteState1 = 0;
int muteState2 = 0;
int muteState3 = 0;

int connectGet1 = 0;
int connectGet2 = 0;
int connectGet3 = 0;
int connectGet4 = 0;

int getBack3 = 0;  //botão para sair da tela de aviso da chave para baixo

int volta1Get = 0;

int slot1 = 0;
int slot2 = 0;
int slot3 = 0;
int slot4 = 0;

//variáveis inicias para os sensores
float SensorTemperaturaE = 0;
String tempEString;  //declarando como global a variável da string da temperatura
float SensorTemperaturaM = 0;

//variáveis para saber quando foi a ultima informação recebida dos sensores, apenas para controlar o funcionamento dos ícones de conectado e desconectado
unsigned long lastDataReceived = 0;
unsigned long lastDataReceived2 = 0;

//temperatura maxima coletada durante todo o período histórico até então
float maxTemperatureMassaALL = -100;
float maxTemperatureEntradaALL = -100;
float maxRTCAll = -100;
//declarando as funções das tasks para não haver erros
void PACT_functions_task(void* pvParameters);
void sensor_task(void* pvParameters);

//handles das tasks, para guia-las
TaskHandle_t Task1;
TaskHandle_t Task2;

//aqui é onde definimos o nome do controlador e a senha
const char* ap_ssid = "Ausyx";
const char* ap_password = "AusyxSolucoes";

//aqui é onde definimos o nome do wifi e a senha que vai se conectar
const char* ssidSoil = "Soil";
const char* passwordSoil = "Soil2024";

int getWifi = 0;

// Buffer para armazenar o texto lido
char buffer[100] = { 0 };  // Certifique-se de que o tamanho do buffer é suficiente para o texto esperado

char buffer2[100] = { 0 };  // Certifique-se de que o tamanho do buffer é suficiente para o texto esperado

//páginas de connect

int connectEntrada = 0;
int connectMassa = 0;
int noConnect = 0;
bool trade = false;

float temperature = 0;

bool sensor1Filled = false;
bool sensor2Filled = false;
int sensorNumber = 0;
int sensor1Number = 0;
int sensor2Number = 0;
int sensor1definedAss = 0;  //1 para entrada e 2 para massa
int sensor2definedAss = 0;
int sensor1NumberPast = 0;
int sensor2NumberPast = 0;

int sheetsturbina = 0;
int sheetsturbinaCaixa = 0;
int sheetsMassaMax;
int sheetsMassaMin;
int sheetsEntradaMax;
int sheetsEntradaMin;
int sheetsQueimador;
int sheetsisON;
int sheetsON;
int sheetsOF;
int loopInterval = 100;

int Teto = 0;
int Telhado = 0;
int Chao = 0;
int Piso = 0;
int Porta = 0;
int Fechadura = 0;
int Arnold = 0;
int schwarzenegger = 0;

int checkingControl = 0;  //se for 0, vai enviar para a entrada, se for 1 vai receber da entrada e se for 2 vai receber da massa.

int EEPROM_SIZE = 512;

int lastConnectionTime =0;

int IntKey = 0;
int lenhaMode = 0;
int keyInt1f = 0;
int keyInt2f = 0;
int keyPACT1f = 0;
int keyPACT2f = 0;
int iniciando = 0;

bool nonStored = false;

unsigned long currentMillis = millis();


void DS3231_setup(void);

void ABRINDO() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);  // abrir

  for (int i = 0; i <= 12000; i += 10) {
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

  for (int l = 0; l <= 6000; l += 5) {
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


void CALIBRA() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);  // fechar

  for (int l = 0; l <= 10000; l += 5) {
    Serial.println("calibra");
    Serial.print("l: ");
    Serial.println(l);
    delay(10);
  }

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);  // fechado

  status1 = 0;
  status2 = 1;
}


void keyFinder() {

  Chao = keyInt1f + keyInt2f;
  Porta = keyPACT1f + keyPACT2f;

  keyInt1f = digitalRead(KeyInt1);
  keyInt2f = digitalRead(KeyInt2);
  keyPACT1f = digitalRead(keyPACT1);
  keyPACT2f = digitalRead(keyPACT2);

  Piso = keyInt1f + keyInt2f;
  Fechadura = keyPACT1f + keyPACT2f;

  if (Chao != Piso) {
    if (keyInt1f == 0 && keyInt2f == 0) {
      IntKey = 0;
    }

    if (keyInt1f == 1 && keyInt2f == 0) {
      IntKey = 1;  //liga a intermitência
    }

    if (keyInt1f == 0 && keyInt2f == 1) {
      IntKey = 2;  //ligação direta
    }
  }

  if (Porta != Fechadura) {
    if (keyPACT1f == 0 && keyPACT2f == 0) {
      lenhaMode = 0;  //modo wind, lenha
      iniciou = false;
    }

    if (keyPACT1f == 1 && keyPACT2f == 0) {
      lenhaMode = 1;  //modo PACT
      iniciou = false;
    }

    if (keyPACT1f == 0 && keyPACT2f == 1) {
      lenhaMode = 2;  //modo ligação direta
    }
  }
}

void confereData() {

  if (horaSet.available()) {
    horaSet.getData();
    hour = horaSet.read();
  }

  if (minutoSet.available()) {
    minutoSet.getData();
    minute = minutoSet.read();
  }

  if (diaSet.available()) {
    diaSet.getData();
    day = diaSet.read();
  }

  if (mesSet.available()) {
    mesSet.getData();
    month = mesSet.read();
  }

  if (anoSet.available()) {
    anoSet.getData();
    year = anoSet.read();
  }
}

void set_display_config() {
 
 

  Arnold = ON2 + OF2 + tempMaxEntrada2 + tempMinEntrada2 + tempMaxMassa2 + tempMinMassa2;


  if (iniciar.available()) {
    iniciar.getData();
    getStart = iniciar.read();
  }

  if (on.available()) {
    on.getData();
    ON2 = on.read();
   
  }

  if (of.available()) {
    of.getData();
    OF2 = of.read();
 
  }

  if (tempMaxE.available()) {
    tempMaxE.getData();
    tempMaxEntrada2 = tempMaxE.read();
    
  }

  if (tempMinE.available()) {
    tempMinE.getData();
    tempMinEntrada2 = tempMinE.read();
   
  }

  if (tempMaxM.available()) {
    tempMaxM.getData();
    tempMaxMassa2 = tempMaxM.read();
  
  }

  if (tempMinM.available()) {
    tempMinM.getData();
    tempMinMassa2 = tempMinM.read();
   
  }

  schwarzenegger = ON2 + OF2 + tempMaxEntrada2 + tempMinEntrada2 + tempMaxMassa2 + tempMinMassa2;

  if (Arnold != schwarzenegger){

    ON = ON2;
    OF = OF2;
    tempMaxEntrada = tempMaxEntrada2;
    tempMinEntrada = tempMinEntrada2;
    tempMaxMassa = tempMaxMassa2;
    tempMinMassa = tempMinMassa2;

    EEPROM.put(40, ON);
    EEPROM.put(50, OF);
    EEPROM.put(5, tempMaxEntrada);
    EEPROM.put(10, tempMinEntrada);
    EEPROM.put(20, tempMaxMassa);
    EEPROM.put(30, tempMinMassa);
    EEPROM.commit();  // Garante que a gravação na EEPROM seja realizada
  }



}

void setMaxAndMin1() {
  //pegando os valores de maximo e minimo definidos anteriormente na pagina de configuracoes da temperatura
  EEPROM.get(5, tempMaxEntrada);
  EEPROM.get(10, tempMinEntrada);
  EEPROM.get(20, tempMaxMassa);
  EEPROM.get(30, tempMinMassa);

  delay(50);
  tempMaxE.write(tempMaxEntrada);  //criar outra função
  delay(20);
  tempMinE.write(tempMinEntrada);
  delay(20);
  tempMaxM.write(tempMaxMassa);
  delay(20);
  tempMinM.write(tempMinMassa);
  delay(20);

  EEPROM.get(40, getActivityTime);
  delay(20);
  EEPROM.get(50, getRestTime);
  delay(20);

  ON = getActivityTime;
  OF = getRestTime;
  //setando nas variáveis do display cada tempo, página main
  on.write(ON);
  delay(20);
  of.write(OF);
}

void atualizaTemp() {

  //setando a temperatura da entrada e da massa para aparecer no display
  tempE.write(SensorTemperaturaE);
  tempM.write(SensorTemperaturaM);
  tempExt.write(temperatureRTC);
}

void setSensorValues() {

  EEPROM.get(65, sensor1definedAss);
  EEPROM.get(68, sensor1NumberPast);
  EEPROM.get(70, sensor2definedAss);
  EEPROM.get(73, sensor2NumberPast);
  
}

void atualizaData() {
  RtcDateTime now;
  now = Rtc.GetDateTime();
  ano.write(now.Year() - 2000);
  mes.write(now.Month());
  dia.write(now.Day());
  hora.write(now.Hour());
  minuto.write(now.Minute());
}

void bottom_reset() {

  connectEntrada = 0;
  connectMassa = 0;
  noConnect = 0;

  connectGet1 = 0;
  connectGet2 = 0;
  connectGet3 = 0;
  connectGet4 = 0;


}

String Atividade = "atividade";
String Repouso = "repouso";
String Direta = "Direta";

void findSensor() {

  slot1 = readIntFromEEPROM(100);
  slot2 = readIntFromEEPROM(101);
  slot3 = readIntFromEEPROM(102);
  slot4 = readIntFromEEPROM(103);

  String sensor1Data = "sensor: " + String(slot1);
  String sensor2Data = "sensor: " + String(slot2);
  String sensor3Data = "sensor: " + String(slot3);
  String sensor4Data = "sensor: " + String(slot4);

  if (slot1 != 0) {
    icon1.write(1);
    Sensor1.write((uint64_t)(sensor1Data.c_str()));
  } else {
    icon1.write(0);
    Sensor1.write((uint64_t)(""));
  }

  if (slot2 != 0) {
    icon2.write(1);
    Sensor2.write((uint64_t)(sensor2Data.c_str()));
  } else {
    icon2.write(0);
    Sensor2.write((uint64_t)(""));
  }

  if (slot3 != 0) {
    icon3.write(1);
    Sensor3.write((uint64_t)(sensor3Data.c_str()));
  } else {
    icon3.write(0);
    Sensor3.write((uint64_t)(""));
  }

  if (slot4 != 0) {
    icon4.write(1);
    Sensor4.write((uint64_t)(sensor4Data.c_str()));
  } else {
    icon4.write(0);
    Sensor4.write((uint64_t)(""));
  }
}



void setup() {

  Lcm.begin();  // Inicia e configura o LCM

  

  Serial.begin(115200);

  Serial2.begin(115200);

  IPAddress staticIP(192, 168, 4, 2);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  //atribuindo o nome do servidor definido inicialmente do código e a senha para conectar

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssidSoil, passwordSoil);
  WiFi.softAP(ap_ssid, ap_password);

  server.begin();

  EEPROM.begin(EEPROM_SIZE);  //EEPROM do ESP32, memória que armazena valores

  ON = EEPROM.read(40);  //lendo o tempo de Atividade diretamente da EEPROM para garantir que o tempo vai iniciar como o ultimo salvo na memória
  OF = EEPROM.read(50);  //lendo o tempo de Descanso diretamente da EEPROM para garantir que o tempo vai iniciar como o ultimo salvo na memória

  //declaração de pinos, OUTPUT para aqueles que precisam mandar algum sinal e INPUT para aqueles que precisam receber um sinal, no caso o sinal GND
  pinMode(OUT_QUEIMADOR, OUTPUT);
  pinMode(OUT_TEMPO, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(BUZINA, OUTPUT);

  //iniciando todas as variáveis de saída como desligadas para não começar com algum problema
  digitalWrite(OUT_QUEIMADOR, LOW);
  digitalWrite(OUT_TEMPO, LOW);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(BUZINA, LOW);


  xSemaphore = xSemaphoreCreateBinary();
  // Certifique-se de que o semáforo foi criado com sucesso
  if (xSemaphore != NULL) {
    // Disponibiliza o semáforo pela primeira vez
    xSemaphoreGive(xSemaphore);
  }

  //tasks que executam as funções do código em segundo plano (Core 1 do esp32)
  xTaskCreatePinnedToCore(&sensor_task,  /* Função que implementa a task */
                          "task sensor", /* Nome da task */
                          4800,          /* Número de palavras de 32 bits a serem alocadas para uso como pilha da task */
                          NULL,          /* Parâmetro passado para a task - não estamos usando nenhum */
                          2,             /* Prioridade da task */
                          &Task2,        /* Handle da task*/
                          1);            //core em que a task opera (existem dois cores para ESP32, core 0 e 1);

  xTaskCreatePinnedToCore(&PACT_functions_task, /* Função que implementa a task */
                          "PACT Task",          /* Nome da task */
                          4800,                 /* Número de palavras de 32 bits a serem alocadas para uso como pilha da task */
                          NULL,                 /* Parâmetro passado para a task - não estamos usando nenhum */
                          5,                    /* Prioridade da task */
                          &Task1,               /* Handle da task*/
                          0);                   //core em que a task opera (existem dois cores para ESP32, core 0 e 1);


  xTaskCreatePinnedToCore(&TaskPlanilhaS,  /* Função que implementa a task */
                          "TaskPlanilhaS", /* Nome da task */
                          32000,           /* Número de palavras de 32 bits a serem alocadas para uso como pilha da task */
                          NULL,            /* Parâmetro passado para a task - não estamos usando nenhum */
                          3,               /* Prioridade da task */
                          &Task1,          /* Handle da task*/
                          1);              //core em que a task opera (existem dois cores para ESP32, core 0 e 1);

  xTaskCreatePinnedToCore(&TaskPlanilhaR,  /* Função que implementa a task */
                          "TaskPlanilhaR", /* Nome da task */
                          32000,           /* Número de palavras de 32 bits serem alocadas para uso como pilha da task */
                          NULL,            /* Parâmetro passado para a task - não estamos usando nenhum */
                          6,               /* Prioridade da task */
                          &Task2,          /* Handle da task*/
                          1);              //core em que a task opera (existem dois cores para ESP32, core 0 e 1);

  delay(100);  //pequeno delay para evitar travamentos desnecessários

  DS3231_setup();  //fazendo o setup do DS3231 que permite a conexão do ESP32 ao RTC
  setMaxAndMin1();
  setSensorValues();

  readCredentialsFromEEPROM();
  RtcDateTime now = Rtc.GetDateTime();
  
}

void loop() {

 currentMillis = millis();

  if(nonStored ==  true){
    
    Page3();
    
  }
  else
  {
  keyFinder();
  confereData();
  set_display_config();
  Page1();
  Page2();
  Page4();
  Page5();

  Page7();
  Page8();
  Page9();
  Page10();
  }

 

 
  if (currentMillis - lastUpdateTime >= 30000) {
    lastUpdateTime = currentMillis;
    atualizaTemp();
    atualizaData();
    findSensor();
  }
  



  
  delay(50);
}
