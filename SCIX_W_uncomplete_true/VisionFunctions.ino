int contadorWifi = 0;
const int SSID_MAX_LENGTH = 64;      // Maximum length of SSID
const int PASSWORD_MAX_LENGTH = 64;  // Maximum length of password

//Aqui temos alguns arrays para salvar na EEPROM o SSID e a senha, esses são os que são salvos e logo abaixo temos os que vão pra senha
char ssidEEPROM[SSID_MAX_LENGTH + 1];          // Add 1 for null terminator
char passwordEEPROM[PASSWORD_MAX_LENGTH + 1];  // Add 1 for null terminator

char ssid[SSID_MAX_LENGTH + 1];          // Add 1 for null terminator
char password[PASSWORD_MAX_LENGTH + 1];  // Add 1 for null terminator

String tempSSID;      //SSID temporário coletado do display
String tempPassword;  //PASSWORD temporário coletado do display

const int TEMP_SSID_MAX_LENGTH = 64;      // Tamanho máximo do SSID temporário
const int TEMP_PASSWORD_MAX_LENGTH = 64;  // Tamanho máximo da senha temporária

// Converte tempSSID para um array de caracteres
void tempSSIDToArray(char* ssidArray, const String& tempSSID) {
  tempSSID.toCharArray(ssidArray, TEMP_SSID_MAX_LENGTH);
}

// Converte tempPassword para um array de caracteres
void tempPasswordToArray(char* passwordArray, const String& tempPassword) {
  tempPassword.toCharArray(passwordArray, TEMP_PASSWORD_MAX_LENGTH);
}


// Função para ler as credenciais da EEPROM
void readCredentialsFromEEPROM() {
  // Lê o SSID da EEPROM
  for (int i = 100; i < 132; ++i) {
    ssidEEPROM[i] = EEPROM.read(0 + i);
    if (ssidEEPROM[i] == '\0') break;  // Terminador de string encontrado
  }

  // Lê a senha da EEPROM
  for (int i = 132; i < 178; ++i) {
    passwordEEPROM[i] = EEPROM.read(32 + i);
    if (passwordEEPROM[i] == '\0') break;  // Terminador de string encontrado
  }
}

//lógica para salvar o login e a senha na EEPROM, para não termos problema ao ligar e desligar o ESP32
void saveCredentialsToEEPROM(const char* ssid, const char* password) {
  // Salva o SSID na EEPROM
  for (int i = 100; i < strlen(ssid) + 100; ++i) {
    EEPROM.write(0 + i, ssid[i]);
  }
  EEPROM.write(0 + strlen(ssid), '\0');  // Terminador de string

  // Salva a senha na EEPROM
  for (int i = 132; i < strlen(password) + 132; ++i) {
    EEPROM.write(32 + i, password[i]);
  }
  EEPROM.write(32 + strlen(password), '\0');  // Terminador de string

  EEPROM.commit();
}





void changeWifiDisplay() {
  //se tanto senha quanto login foram atualizados, podemos trocar de rede wifi.
  if (changePassword && changeLogin) {
    WiFi.begin(ssid, password);


    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      Serial.println("Connecting to WiFi...");
      contadorWifi++;

      if (contadorWifi >= 100) {
        contadorWifi = 0;
        break;
      }
    }

    Serial.println("Connected to WiFi");  //não é pq apareceu que ele conectou, só mostra que terminou a tentativa, para conferir se conectou ou não, só olhar no display ou apenas pegar o valor do connectwifi.

    //volto as variáveis para falso para a próxima execução
    changePassword = false;
    changeLogin = false;

    //função que salva na EEPROM ssid e password
    saveCredentialsToEEPROM(ssid, password);
  }
}


void changeDisplayConfig() {

  simpleVConfig();

}
void simpleVConfig() {
  //variaveis do display na page 

  tempMaxE.write(sheetsEntradaMax);  //criar outra função
  tempMinE.write(sheetsEntradaMin);
  tempMaxM.write(sheetsMassaMax);
  tempMinM.write(sheetsMassaMin);


  ON = sheetsON;
  OF = sheetsOF;
  //setando nas variáveis do display cada tempo, página main
  on.write(ON);
  of.write(OF);

  EEPROM.put(5, sheetsEntradaMax);
  EEPROM.put(10, sheetsEntradaMin);
  EEPROM.put(20, sheetsMassaMax);
  EEPROM.put(30, sheetsMassaMin);

  EEPROM.put(40, ON);
  EEPROM.put(50, OF);

  EEPROM.commit();

  tempMaxEntrada = sheetsEntradaMax;
  tempMinEntrada = sheetsEntradaMin;
  tempMaxMassa = sheetsMassaMax;
  tempMinMassa = sheetsMassaMin;
  
}






void Page1()  //PAGE MAIN
{
  if (WiFi.status() == WL_CONNECTED) {
    Wifi.write(1);
  } else {
    Wifi.write(0);
  }

  bateria = map(batteryLevel, 0, 5, 0, 4);

  batteryIconMass.write(bateria);

  if (IntKey == 1) {
    //pegando o valor do botão play, se clicou é pra intermitência funcionar, caso a chave esteja no meio ou para baixo ele vai avisar
    if (getStart == 1) {
      //se a pessoa apertou play e antes estava pausado, ele volta a contar do tempo onde parou

      if (Stop == true) {
        isOn = true;
        Stop = false;
      }

      Intermitencia();  //função que faz  intermitência

    }
    //caso esteja pausado
    else if (getStart == 0) {

      isOn = false;
      status.write(Repouso);
      digitalWrite(OUT_TEMPO, HIGH);         // desliga
      if (Stop == false) {
        Stop = true;
      }
    }
  }

  if (IntKey == 2) {
    if (!iniciou) {

      digitalWrite(OUT_TEMPO, HIGH);  // liga
      status.write(Direta);

    }
  }
}


void Page2()  //Tela de data
{

   if (changeDataDisp.available()) {
    changeDataDisp.getData();
    changeData = changeDataDisp.read();
  }

   if (changeData == 1)
  {
  RTCDS3231(); 
  changeData = 0;
  changeDataDisp.write(0);
  }
}


void Page10()  //Tela massa acima
{
   if (mute1.available()) {
    mute1.getData();
    getMuteMH = mute1.read();
  }

  if (getMuteMH == 1) {
    muteState2 = 1;
    queimador = 0;
    digitalWrite(BUZINA, LOW);
    mute1.write(0);
    getMuteMH = 0;
  }
}

void Page9()  //Tela entrada acima
{
   if (mute2.available()) {
    mute2.getData();
    getMuteEH = mute2.read();
  }

  if (getMuteEH == 1) {
    muteState0 = 1;
    queimador = 0;
    digitalWrite(BUZINA, LOW);
    mute2.write(0);
    getMuteEH = 0;
  }
}

void Page8()  //Tela massa abaixo
{

  if (mute3.available()) {
    mute3.getData();
    getMuteML = mute3.read();
  }

  if (getMuteML == 1) {
    muteState3 = 1;
    queimador = 0;
    digitalWrite(BUZINA, LOW);
    mute3.write(0);
    getMuteML = 0;
  }
}

void Page7()  //Tela entrada abaixo
{
   if (mute4.available()) {
    mute4.getData();
    getMuteEL = mute4.read();
  }

  if (getMuteEL == 1) {

    muteState1 = 1;
    queimador = 0;
    digitalWrite(BUZINA, LOW);
    mute4.write(0);
    getMuteEL = 0;
  }
}

void Page4()  //página de wifi
{
   if (Login.available()) {
    tempSSID = "";  //limpando a variável para a nova leitura

    // Enquanto o login estiver disponível, atualize o ssid
    while (Login.available()) {
      tempSSID += (char)Login.getData();
    }

    tempSSIDToArray(ssid, tempSSID);

    //exibimos o ssid no monitor Serial para conferir se está tudo bem
    Serial.println("login: ");
    Serial.print(ssid);
    changeLogin = true;  //passando a variável para true, pois só atualizamos de verdade o login e senha quando os dois forem atualizados
  }

  if (Senha.available()) {
    tempPassword = "";  //limpanda a variável para a nova leitura

    // Se a Senha estiver disponível, atualize a senha

    //faz  a mesma coisa do outro ali em cima
    while (Senha.available()) {
      tempPassword += (char)Senha.getData();
    }
    tempPasswordToArray(password, tempPassword);
    Serial.print("Senha: ");
    Serial.println(password);

    changePassword = true;
  }


  changeWifiDisplay();
}


//acontece quando chega valor de sensor novo
void Page3() {
  
  Serial.print("aqui dentro");
   if (EntradaConnect.available()) {
    EntradaConnect.getData();
    connectEntrada = EntradaConnect.read();
  }
   if (MassaConnect.available()) {
    MassaConnect.getData();
    connectMassa = MassaConnect.read();
  }
   if (NoConnect.available()) {
    NoConnect.getData();
    noConnect = NoConnect.read();
  }

  if (connectEntrada) {
    if (sensor1Filled) {
      sensor1definedAss = 1;
      sensor1NumberPast = sensorNumber;
      EEPROM.put(65, sensor1definedAss);
      EEPROM.put(68, sensor1NumberPast);
      EEPROM.commit();  // Garante que a gravação na EEPROM seja realizada
    }

    if (sensor2Filled) {
      sensor2definedAss = 1;
      sensor2NumberPast = sensorNumber;
      EEPROM.put(70, sensor2definedAss);
      EEPROM.put(73, sensor2NumberPast);
      EEPROM.commit();  // Garante que a gravação na EEPROM seja realizada
    }
    EntradaConnect.write(0);
    connectEntrada = 0;
    nonStored = false;
     Lcm.changePicId(0);
  }

  if (connectMassa) {
    if (sensor1Filled) {
      sensor1definedAss = 2;
      sensor1NumberPast = sensorNumber;
      EEPROM.put(65, sensor1definedAss);
      EEPROM.put(68, sensor1NumberPast);
      EEPROM.commit();  // Garante que a gravação na EEPROM seja realizada
    }

    if (sensor2Filled) {
      sensor2definedAss = 2;
      sensor2NumberPast = sensorNumber;
      EEPROM.put(70, sensor2definedAss);
      EEPROM.put(73, sensor2NumberPast);
      EEPROM.commit();  // Garante que a gravação na EEPROM seja realizada
    }

    MassaConnect.write(0);
    connectMassa = 0;
    nonStored = false;
     Lcm.changePicId(0);
  }

  if (noConnect) {

    if (sensor1Filled) {
      sensor1Number = sensor1NumberPast;
    }

    if (sensor2Filled) {
      sensor2Number = sensor2NumberPast;
    }
    NoConnect.write(0);
    noConnect = 0;
    nonStored = false;
    Lcm.changePicId(0);
  }
}


void Page5() {

  if (Sensor1.available()) {
    Sensor1.getData();
    connectGet1 = Sensor1.read();
  }
  if (Sensor2.available()) {
    Sensor2.getData();
    connectGet2 = Sensor2.read();
  }
  if (Sensor3.available()) {
    Sensor3.getData();
    connectGet3 = Sensor3.read();
  }
  if (Sensor4.available()) {
    Sensor4.getData();
    connectGet4 = Sensor4.read();
  }

  if (connectGet1 == 1) {
    if (slot1 != 0) {
      trade = true;
      sensorNumber = slot1;
      sensorNumberDisp.write(sensorNumber);
      Sensor1.write(0);
      connectGet1 = 0;
      //Lcm.changePicId(0);
    }
  }

  if (connectGet2 == 1) {
    if (slot2 != 0) {
      trade = true;
      sensorNumber = slot2;
      sensorNumberDisp.write(sensorNumber);
      Sensor2.write(0);
      connectGet2 = 0;
      //Lcm.changePicId(0);
    }
  }

  if (connectGet3 == 1) {
    if (slot3 != 0) {
      trade = true;
      sensorNumber = slot3;
      sensorNumberDisp.write(sensorNumber);
      Sensor3.write(0);
      connectGet3 = 0;
      //Lcm.changePicId(0);
    }
  }

  if (connectGet4 == 1) {
    if (slot4 != 0) {
      trade = true;
      sensorNumber = slot4;
      sensorNumberDisp.write(sensorNumber);
      Sensor4.write(0);
      connectGet4 = 0;
      //Lcm.changePicId(0);
    }
  }
}