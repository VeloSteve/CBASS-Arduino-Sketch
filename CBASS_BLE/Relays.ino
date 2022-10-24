void RelaysInit()
{
  //-------( Initialize Pins so relays are inactive at reset)----
  for (i = 0; i < NT; i++) {
    // Set relay signal to off
    digitalWrite(HeaterRelay[i], RELAY_OFF);
    digitalWrite(ChillRelay[i], RELAY_OFF);
    //---( THEN set pins as Outputs )----
    pinMode(HeaterRelay[i], OUTPUT);
    pinMode(ChillRelay[i], OUTPUT);
#ifdef LOGANMODE
    digitalWrite(LightRelay[i], RELAY_OFF);
    pinMode(LightRelay[i], OUTPUT);
#endif
  }

}

/**
 * Relay Tests.  Writes 8 lines, numbered 1-8.
 * Each heater and chiller relay is turned on, then off.
 * In LOGANMODE test the light relays separately.
 */
void relayTest() {
  tft.fillScreen(BLACK);
  for (i = 0; i < NT; i++) {
    tft.setCursor(0, 2 * i * LINEHEIGHT3);
    // Test one heater relay
    tft.print(2 * i + 1); tft.print(". T"); tft.print(i + 1); tft.print("Heatr");
    Serial.print(2 * i + 1); Serial.print(". T"); Serial.print(i + 1); Serial.println("Heatr");
    digitalWrite(HeaterRelay[i], RELAY_ON);
    delay(RELAY_PAUSE);
    wdt_reset();
    digitalWrite(HeaterRelay[i], RELAY_OFF);
    
    // Test one chiller relay
    tft.setCursor(0, (2 * i + 1) * LINEHEIGHT3);

    tft.print(2 * i + 2); tft.print(". T"); tft.print(i + 1); tft.print("Chillr");
    Serial.print(2 * i + 2); Serial.print(". T"); Serial.print(i + 1); Serial.println("Chillr");
    digitalWrite(ChillRelay[i], RELAY_ON);
    delay(RELAY_PAUSE);
    wdt_reset();
    digitalWrite(ChillRelay[i], RELAY_OFF);
  }

#ifdef LOGANMODE
  delay(RELAY_PAUSE);
  wdt_reset();
  tft.fillScreen(BLACK);
  for (i = 0; i < NT; i++) {
    Serial.print("a "); Serial.println(millis());
    tft.setCursor(0, i * LINEHEIGHT3);
    // Test one light relay
    tft.print(i + 1); tft.print(". T"); tft.print(i + 1); tft.print("Light");
    Serial.print(i + 1); Serial.print(". T"); Serial.print(i + 1); Serial.println("Light");
        Serial.print("b "); Serial.println(millis());

    digitalWrite(LightRelay[i], RELAY_ON);
        Serial.print("c "); Serial.println(millis());

    delay(RELAY_PAUSE);
    wdt_reset();
        Serial.print("d "); Serial.println(millis());

    digitalWrite(LightRelay[i], RELAY_OFF);
        Serial.print("e "); Serial.println(millis());

  }
#endif // LOGANMODE
}

// This was originally just for turning off heating and cooling, but in LOGANMODE we also
// check lights here.
void updateRelays() {
  for (i=0; i<NT; i++) {
    // We originally checked for TempOutput < 0, but that is never the case
    // now that SetOutputLimits is never called.  TempOutput ranges from 0.0 to 255.0
    if (TempOutput[i] < 0) { //Chilling
      if (TempInput[i] > SetPoint[i] - ChillOffset) {
        digitalWrite(ChillRelay[i], RELAY_ON);
        digitalWrite(HeaterRelay[i], RELAY_OFF);
        strcpy(RelayStateStr[i], "CHL");
      }
      else {
        digitalWrite(ChillRelay[i], RELAY_OFF);
        digitalWrite(HeaterRelay[i], RELAY_OFF);
        strcpy(RelayStateStr[i], "OFF");
      }
    } else { //Heating
      if (TempOutput[i] > 0.0) {  
        if (TempInput[i] > SetPoint[i] - ChillOffset) {
          digitalWrite(HeaterRelay[i], RELAY_ON);
          digitalWrite(ChillRelay[i], RELAY_ON);
          strcpy(RelayStateStr[i], "HTR");
        }
        else {
          digitalWrite(HeaterRelay[i], RELAY_ON);
          digitalWrite(ChillRelay[i], RELAY_OFF);
          strcpy(RelayStateStr[i], "HTR");
        }
      }
      else {
        if (TempInput[i] > SetPoint[i] - ChillOffset) {
          digitalWrite(HeaterRelay[i], RELAY_ON);
          digitalWrite(ChillRelay[i], RELAY_ON);
          strcpy(RelayStateStr[i], "HTR");
        }
        else {
          digitalWrite(HeaterRelay[i], RELAY_OFF);
          digitalWrite(ChillRelay[i], RELAY_ON);
          strcpy(RelayStateStr[i], "OFF");
        }
      }
    }
#ifdef LOGANMODE
    if (lightState[i]) {
      digitalWrite(LightRelay[i], RELAY_ON);
    } else {
      digitalWrite(LightRelay[i], RELAY_OFF);
    }
#endif // LOGANMODE
  }
}
