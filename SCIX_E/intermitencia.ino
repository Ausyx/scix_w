void UpdateTimeOnVision(unsigned long totalTime) {
  unsigned int minutes = totalTime / 60;
  unsigned int seconds = totalTime % 60;
  char formattedTime[8];

  sprintf(formattedTime, "%02d:%02d", minutes, seconds);

  cronometer.write(formattedTime,25);
}

//função para fazer um update no display das barras e reinicia o tempo de início a cada troca de estado
void UpdateVisionDisplay(unsigned long elapsedTime, bool isOn) {

  unsigned long intermitenciaElapsedTime = elapsedTime;


  if (isOn) {
    progress = map(intermitenciaElapsedTime, 0, ON * 60, 0, 100);

  } else {
    progress = map(intermitenciaElapsedTime, 0, OF * 60, 0, 100);
  }
  

  // Reinicia o tempo e a barra de progresso se o tempo de intermitência for maior ou igual a ON ou OF minutos
  if (intermitenciaElapsedTime >= (ON * 60) && isOn == true) {
    startTime = currentTime;  // Reinicia o tempo inicial da função com base no RTC
  }
  if (intermitenciaElapsedTime >= (OF * 60) && isOn == false) {
   
    startTime = currentTime;  // Reinicia o tempo inicial da função com base no RTC
  }

  UpdateTimeOnVision(intermitenciaElapsedTime); //atualizando o tempo no display e mandando os parâmetros
}

void Intermitencia() {
  // Lê a data e hora do RTC DS3231
  RtcDateTime now = Rtc.GetDateTime();

  currentTime = now.TotalSeconds();  //pegando o tempo atualizado
  elapsedTime = currentTime - startTime;  //vendo o tempo que passou

  if (!isOn) {
    // Estamos na fase OF (repouso)

    if (elapsedTime >= OF * 60) {
      // O tempo OF terminou, mude para ON (ativa)
      isOn = true;
       status.write(Atividade);
       digitalWrite(OUT_TEMPO, HIGH);  // liga
    }
  } else {
    // Estamos na fase ON (ativa)

    if (elapsedTime >= ON * 60) {
      // O tempo ON terminou, mude para OF (desligada)
      isOn = false;
      status.write(Repouso);
      digitalWrite(OUT_TEMPO, LOW);  // desliga
    }
  }
  // Atualiza o display do Nextion para mostrar o tempo decorrido e fazer as mudanças necessárias
  UpdateVisionDisplay(elapsedTime, isOn);

  //apenas printando para conferir os testes
  Serial.print("IS ON: ");
  Serial.println(isOn);
}
