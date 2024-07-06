




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

bool isSensorStored(int sensor) {
  for (int i = 100; i < EEPROM_SIZE; i++) {
    int storedSensor = readIntFromEEPROM(i);
    if (storedSensor == sensor) {
      return true;
    }
  }
  return false;
}

void storeSensor(int sensor) {
  for (int i = 100; i < EEPROM_SIZE; i++) {
    int storedSensor = readIntFromEEPROM(i);
    if (storedSensor == 0) {  // 0 is considered as an empty slot
      writeIntToEEPROM(i, sensor);
      return;
    }
  }
}


int readIntFromEEPROM(int addr) {
  int value;
  EEPROM.get(addr, value);
  return value;
}

void writeIntToEEPROM(int addr, int value) {
  EEPROM.put(addr, value);
  EEPROM.commit();
}

void envia_dados() {
  if (client.connect(centralServer, centralPort)) {

    toSend = "GET /?Central= 1";
    toSend += +sensorNumber;  // Sensor number
    toSend += "&tempMaxEntrada=";
    toSend += tempMaxEntrada;
    toSend += "&tempMaxMassa=";
    toSend += tempMaxMassa;
    toSend += "&muteState0";
    toSend += muteState0;
    toSend += "&muteState1";
    toSend += muteState1;
    toSend += "&muteState2";
    toSend += muteState2;
    toSend += "&muteState3";
    toSend += muteState3;

    toSend += " HTTP/1.1\r\n";
    toSend += "Host: ";
    toSend += WiFi.localIP().toString();  // IP address of the central ESP32
    toSend += "\r\nConnection: close\r\n\r\n";
    client.print(toSend);

    toSend = "";
    lastConnectionTime = millis();
    Serial.println("Temperature sent to the central!");
    Serial.print("Temperature: ");
    Serial.println(temperature);
  }
}



void handleClient(WiFiClient client) {
  String req = client.readStringUntil('\r');
  req = req.substring(req.indexOf("/") + 1, req.indexOf("HTTP") - 1);
  client.flush();

  const unsigned long timeout = 300000;

  if (millis() - lastDataReceived > timeout) {

    ConnectVal = 0;  //se não conectou ele continua cinza
    if (sensor1definedAss == 1) {
      SensorTemperaturaE = 0;
      connectIconEnt.write(ConnectVal);

    } else if (sensor1definedAss == 2) {
      SensorTemperaturaM = 0;
      connectIconMass.write(ConnectVal);
      digitalWrite(OUT_QUEIMADOR, HIGH);
    }

  } else {
    ConnectVal = 1;
  }

  if (millis() - lastDataReceived2 > timeout) {
    lastDataReceived = millis();
    ConnectVal2 = 0;  //se não conectou ele continua cinza
    if (sensor2definedAss == 1) {
      SensorTemperaturaE = 0;
      connectIconEnt.write(ConnectVal2);

    } else if (sensor2definedAss == 2) {
      SensorTemperaturaM = 0;
      connectIconMass.write(ConnectVal2);
    }

    digitalWrite(OUT_QUEIMADOR, HIGH);
    return;
  } else {
    ConnectVal2 = 1;
  }

  if (req.indexOf("sensor=") != -1 && req.indexOf("temp=") != -1 && req.indexOf("batt=") != -1) {
    String SensorNumber = getParamValue(req, "sensor");
    temperature = getParamValue(req, "temp").toFloat() / 100;
    int batteryLevel = getParamValue(req, "batt").toInt();


    sensorNumber = SensorNumber.toInt();



    if (!isSensorStored(sensorNumber)) {
      storeSensor(sensorNumber);

      sensorNumber = SensorNumber.toInt();

      sensorNumberDisp.write(sensorNumber);

      if ((sensorNumber != sensor1Number && sensorNumber != sensor2Number) || trade == true) {
        if (!sensor1Filled) {
          sensor1Number = sensorNumber;
          sensor1Filled = true;
          sensor2Filled = false;
          trade = false;
        } else if (!sensor2Filled) {
          sensor2Number = sensorNumber;
          sensor2Filled = true;
          sensor1Filled = false;
          trade = false;
        }
      }
      
      
       Lcm.changePicId(2); //precisa ativar o pop up e trocar a tela aqui.

      //printSensorDetails(sensorNumber, temperature, batteryLevel);
    }



    if (sensorNumber == sensor1NumberPast) {
      if (sensor1definedAss == 1) {
        ConnectVal = 1;
        SensorTemperaturaE = temperature;
        connectIconEnt.write(ConnectVal);

      } else if (sensor1definedAss == 2) {
        ConnectVal = 1;
        SensorTemperaturaM = temperature;
        connectIconMass.write(ConnectVal);
      }
      lastDataReceived = millis();
    }

    if (sensorNumber == sensor2NumberPast) {
      if (sensor2definedAss == 1) {
        SensorTemperaturaE = temperature;
        ConnectVal2 = 1;
        connectIconEnt.write(ConnectVal2);

      } else if (sensor2definedAss == 2) {
        ConnectVal2 = 1;
        SensorTemperaturaM = temperature;
        connectIconMass.write(ConnectVal2);
      }

      lastDataReceived2 = millis();
    }
  } else {
    client.print("Invalid Request");
  }
  client.flush();
  client.stop();
}


//task criada que tenta executar essa conexão a todo momento
void sensor_task(void *pvParameters) {

  (void)pvParameters;  // Evita aviso de não utilizado

  while (1) {
    
    if (checkingControl == 0) {

      if (sensor1definedAss == 1) {
        sensorNumber = sensor1NumberPast;
      } else if (sensor2definedAss == 1) {
        sensorNumber = sensor2NumberPast;
      }

      envia_dados();

      checkingControl = 1;
    }

    WiFiClient client = server.available();

    if (checkingControl == 1 || checkingControl == 2) {
      if (client) {
        handleClient(client);

        if (sensor2definedAss == 2 && (lastDataReceived2 > lastDataReceived)) {
          checkingControl = 0;
        }
        if (sensor2definedAss == 1 && (lastDataReceived2 > lastDataReceived)) {
          checkingControl = 2;
        }
        if (sensor1definedAss == 2 && (lastDataReceived > lastDataReceived2)) {
          checkingControl = 0;
        }
        if (sensor1definedAss == 1 && (lastDataReceived > lastDataReceived2)) {
          checkingControl = 2;
        }
      }
    }

    if (sensor1definedAss == 1) {
      connectIconEnt.write(ConnectVal);

    } else if (sensor1definedAss == 2) {
      connectIconMass.write(ConnectVal);
    }

    if (sensor2definedAss == 1) {
      connectIconEnt.write(ConnectVal2);

    } else if (sensor2definedAss == 2) {
      connectIconMass.write(ConnectVal);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}