

void DS3231_setup() {
  Wire.begin();  //setup apenas inciando a conexão e estabelecendo a conexão entre RTC e ESP32
  Rtc.Begin();
}

void DS3231_loop() {
  // Lê a data e hora do RTC DS3231

  RtcDateTime now = Rtc.GetDateTime();  //o loop serve apenas para garantir que em nenhum momento a hora estará desatualizada, porém não estamos usando já que isso está sendo feito nas próprias funções

  //delay(100); // Aguarda 1 segundo antes da próxima leitura
}

//essa é a função que atualiza a data no RTC de acordo com a Tela de Data e hora, ela só ocorre quando entra nessa tela
void RTCDS3231() {

  // Configure a data e hora no RTC DS3231
  RtcDateTime newDateTime(year, month, day, hour, minute, 0);
  Rtc.SetDateTime(newDateTime);


  // Feedback para o operador (pode ser uma mensagem no display ou serial)
  Serial.println("Data e hora definidas no RTC DS3231.");
}
