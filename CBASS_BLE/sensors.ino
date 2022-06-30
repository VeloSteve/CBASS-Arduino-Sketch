
void sensorsInit()
{
    // ***** INPUT *****
  // Start up the TempSensor library
  sensors.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement

  for (int i=0; i<NT; i++) {
    // method 1: by index
    if (!sensors.getAddress(Thermometer[i], i)) { Serial.print("Unable to find address for Tank "); Serial.println(i+1);}
  
    // show the address we found on the bus
    Serial.print("Tank "); Serial.print(i+1); Serial.print(" Address: ");
    printAddress(Thermometer[i]);
    Serial.println();
  
    // set the resolution to 12 bit
    sensors.setResolution(Thermometer[i], 12);
  }
}


// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();    
}

// main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
  Serial.print("Device Address: ");
  printAddress(deviceAddress);
  Serial.print(" ");
  printTemperature(deviceAddress);
  Serial.println();
}
