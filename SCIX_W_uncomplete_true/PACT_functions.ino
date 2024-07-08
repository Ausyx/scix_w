void Lenha_mode() {
  if (((SensorTemperaturaE < tempMaxEntrada) || (SensorTemperaturaM < tempMaxMassa))&&(status2 == 0)) {
    FECHANDO();
  }

  if (((SensorTemperaturaE > tempMaxEntrada) || (SensorTemperaturaM >tempMaxMassa))&& (status1 == 0)) {
    ABRINDO();
  }
}


void PACT_functions() {  //função que funciona em segundo plano, faz as funções de um PACT, ou seja, o controle de temperatura para nunca deixar a temperatura ultrapassar demais a faixa de máxima

  EEPROM.get(55, maxTemperatureMassaALL);  //pega a maior temperatura que ja foi colocado o ESP32
  if (SensorTemperaturaM > maxTemperatureMassaALL) {
    maxTemperatureMassaALL = SensorTemperaturaM;
    EEPROM.put(55, maxTemperatureMassaALL);
    EEPROM.commit();  // Garante que a gravação na EEPROM seja realizada
  }
  EEPROM.get(57, maxTemperatureEntradaALL);
  if (SensorTemperaturaE > maxTemperatureEntradaALL) {
    maxTemperatureEntradaALL = SensorTemperaturaE;
    EEPROM.put(57, maxTemperatureEntradaALL);
    EEPROM.commit();  // Garante que a gravação na EEPROM seja realizada
  }

  //máximas temperaturas atingidas pelos sensores, já registradas e salvas na EEPROM do ESP32
  Serial.print("Maxima temperatura atingida da Massa: ");
  Serial.println(maxTemperatureMassaALL);

  Serial.print("Maxima temperatura atingida da Entrada: ");
  Serial.println(maxTemperatureEntradaALL);

  //executando a funcao de pegar a temperatura da do RTC e salvar como a máxima em que colocaram o sensor e salvar na EEPROM
  temperatureRTC = Rtc.GetTemperature().AsFloatDegC();  // Atualiza a temperatura do RTC DS3231

  if (temperatureRTC > maxRTCAll) {
    maxRTCAll = temperatureRTC;
    EEPROM.put(60, maxRTCAll);
    EEPROM.commit();
  }
  Serial.print("Maxima temperatura em que foi colocado o sensor: ");
  Serial.println(maxRTCAll);

  if (SensorTemperaturaE <= tempMaxEntrada && SensorTemperaturaE != 0) {
    muteState0 = 0;  //como está tudo bem com a temperatura, é possível voltar o estado da tela de aviso para 0
  }

  if (SensorTemperaturaE >= tempMinEntrada && SensorTemperaturaE != 0) {
    muteState1 = 0;  //se está tudo bem e a temperatura está maior que a miníma, o muteState também reseta
  }

  if (SensorTemperaturaM <= tempMaxMassa && SensorTemperaturaM != 0) {
    muteState2 = 0;  //mesma coisa dos outros, agora para o aviso de massa acima
  }

  if (SensorTemperaturaM >= tempMinMassa && SensorTemperaturaM != 0) {
    muteState3 = 0;  //mesma coisa dos outros, agora para o aviso de massa abaixo
  }

  if (muteState0 == 0 && SensorTemperaturaE >= (tempMaxEntrada + 7)) {
    Lcm.changePicId(9);
    digitalWrite(BUZINA, HIGH);

    muteState0 = 1;
  }

  if (muteState3 == 0 && SensorTemperaturaM < (tempMinMassa - 7) && tempMinMassa != 0) {
    Lcm.changePicId(8);
    digitalWrite(BUZINA, HIGH);

    muteState3 = 1;
  }

  if (muteState1 == 0 && tempMinEntrada != 0 && SensorTemperaturaE < (tempMinEntrada - 7)) {
    Lcm.changePicId(7);
    digitalWrite(BUZINA, HIGH);

    muteState1 = 1;
  }

  if (muteState2 == 0 && SensorTemperaturaM > (tempMaxMassa + 7)) {
    Lcm.changePicId(10);
    digitalWrite(BUZINA, HIGH);
    muteState2 = 1;
  }


  if (lenhaMode = 1) {
    //Liga o queimador para caso as duas temperaturas estejam abaixo do máximo, para garantir total segurança
    if ((SensorTemperaturaE <= tempMaxEntrada) && (SensorTemperaturaE != 0) && (SensorTemperaturaM <= tempMaxMassa) && (SensorTemperaturaM != 0)) {

      if (queimador == 0) {
        digitalWrite(OUT_QUEIMADOR, HIGH);
      }
      queimador = 1;
    }

    //caso passe da máxima desliga o queimador
    if (SensorTemperaturaE >= (tempMaxEntrada)) {

      if (queimador == 1) {
        digitalWrite(OUT_QUEIMADOR, LOW);
        queimador = 0;
      }
    }

    if (SensorTemperaturaE < (tempMinEntrada)) {

      if (queimador == 0) {
        digitalWrite(OUT_QUEIMADOR, LOW);
        queimador = 0;
      }
    }

    //mesma coisa das outras
    if (SensorTemperaturaM > (tempMaxMassa)) {
      if (queimador == 0) {
        digitalWrite(OUT_QUEIMADOR, LOW);
        queimador = 0;
      }
    }

    //continua sendo a mesma coisa das outras
    if (SensorTemperaturaM < (tempMinMassa)) {

      if (queimador == 0) {
        digitalWrite(OUT_QUEIMADOR, LOW);
        queimador = 0;
      }
    }

  }
  ///////////////////////////////////////////////////////////////////////////////////////// ligação direta //////////////////////////////////////////////////////

  else if (lenhaMode == 2) {
    digitalWrite(OUT_QUEIMADOR, HIGH);
    queimador = 1;
  }
}


//task que visa repetir esse processo sem parar, coloquei para repertir a cada dez segundos, visando não sobrecarregar o core
void PACT_functions_task(void *pvParameters) {
  if (iniciando == 0) {
   // CALIBRA();
    iniciando = 1;
  }


  (void)pvParameters;  // Evita aviso de não utilizado

  while (1) {
    Serial.print(" This taks run on core: ");
    Serial.println(xPortGetCoreID());

    PACT_functions();
    Lenha_mode();

    vTaskDelay(30000 / portTICK_PERIOD_MS);
  }
}
