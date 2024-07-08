




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
  for (int i = 200; i < EEPROM_SIZE; i++) {
    int storedSensor = readIntFromEEPROM(i);
    if (storedSensor == sensor) {
      return true;
    }
  }
  return false;
}

void storeSensor(int sensor) {
  for (int i = 200; i < EEPROM_SIZE; i++) {
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
    batteryLevel = getParamValue(req, "batt").toInt();


    sensorNumber = SensorNumber.toInt();

    Serial.print("temperature: ");
    Serial.println (temperature);



    if (!isSensorStored(sensorNumber)) {
      nonStored = true;
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

    }

    
    Serial.print("sensorNumber:");
    Serial.println( sensorNumber);
    Serial.print("sensor1NumberPast:");
    Serial.println( sensor1NumberPast);
    Serial.print("sensor2NumberPast:");
    Serial.println( sensor2NumberPast);
    
    



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

    WiFiClient client = server.available();

      if (client) {
        handleClient(client);
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

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}