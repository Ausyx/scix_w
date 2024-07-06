//ABA responsável pela coleta de dados da plalinha e enviar os dados para a plnilha, são as nossas duas tasks que intercalam enntre si, se elas ocorrem ao mesmo tempo, trava legal o código
void sendDataToGoogleSheet() {
  HTTPClient http;
  http.begin(googleScriptURL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

 

  // Prepara os dados a serem enviados, incluindo os estados atuais dos controles
   String httpRequestData = "temperaturaM=" + String(SensorTemperaturaM) +
                             "&temperaturaE=" + String(SensorTemperaturaE) +
                             "&tempMaxMassa=" + String(tempMaxMassa) +
                             "&tempMinMassa=" + String(tempMinMassa) +
                             "&tempMaxEntrada=" + String(tempMaxEntrada) +
                             "&tempMinEntrada=" + String(tempMinEntrada) +
                             "&estadoQueimador=" + String(queimador)+ 
                             "&estadoCilindro=" + String(isOn)+
                             "&tempoAtivo=" + String(ON)+
                             "&tempoRepouso=" + String(OF);


  // Faz a requisição POST
  int httpResponseCode = http.POST(httpRequestData);

  // Verifica a resposta do servidor
  if (httpResponseCode > 0) {
    Serial.println("Dados enviados. Código de resposta: ");
    Serial.println(httpResponseCode);
     Serial.println(http.getString());  // Imprime a resposta do servidor
  } else {
    Serial.println("Erro ao enviar dados: ");
    //Serial.println(httpResponseCode);
  }

  http.end();
}



void receiveCommandsFromGoogleSheet() {
  HTTPClient http;
  http.begin(googleScriptURL);
  delay(100);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpResponseCode = http.GET();
  delay(100);
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Comandos recebidos: ");
    Serial.println(response);

    // Chamar a função para interpretar a resposta JSON
    parseJSON(response);
  } else {
    Serial.print("Erro ao receber comandos: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}


void parseJSON(String jsonResponse) {
  // Alocar o JsonDocument
  // Ajuste a capacidade conforme necessário
  const size_t capacity = JSON_OBJECT_SIZE(5) + 200;
  DynamicJsonDocument doc(capacity);

  // Deserializar o JSON
  DeserializationError error = deserializeJson(doc, jsonResponse);

  if (error) {
    Serial.print(F("deserializeJson() falhou: "));
    Serial.println(error.c_str());
    return;
  }

     

     
      sheetsMassaMax = doc["estadoMassaMax"];
      sheetsMassaMin = doc["estadoMassaMin"];
      sheetsEntradaMax = doc["estadoEntradaMax"];
      sheetsEntradaMin = doc["estadoEntradaMin"];
      sheetsQueimador = doc["estadoQueimador"];
      sheetsisON = doc["estadoCilindro"];
      sheetsON = doc["tempoAtivo"];
      sheetsOF = doc["tempoRepouso"];

    




}


void TaskPlanilhaS(void *pvParameters) {
  for (;;) {                                    // Loop infinito para a task rodar continuamente
    xSemaphoreTake(xSemaphore, portMAX_DELAY);  // Espera pelo semáforo indefinidamente
  

    sendDataToGoogleSheet();

    xSemaphoreGive(xSemaphore);  // Libera o semáforo
    delay(10);
    vTaskDelay(pdMS_TO_TICKS(1));  // Aguarda 10 segundos antes da próxima execução
  }
}

void TaskPlanilhaR(void *pvParameters) {
  for (;;) {                                    // Loop infinito para a task rodar continuamente
    xSemaphoreTake(xSemaphore, portMAX_DELAY);  // Espera pelo semáforo indefinidament
      Teto = sheetsMassaMax + sheetsMassaMin + sheetsEntradaMax + sheetsEntradaMin + sheetsQueimador + sheetsisON + sheetsON + sheetsOF;
    //recebe os comandos da planilha;
    receiveCommandsFromGoogleSheet();

      Telhado = sheetsMassaMax + sheetsMassaMin + sheetsEntradaMax + sheetsEntradaMin + sheetsQueimador + sheetsisON + sheetsON + sheetsOF;

     if (Teto != Telhado){
      changeDisplayConfig();
     }
     
    
    
    xSemaphoreGive(xSemaphore);  // Libera o semáforo
    delay(10);
    vTaskDelay(pdMS_TO_TICKS(1));  // Aguarda 10 segundos antes da próxima execução
  }
}
