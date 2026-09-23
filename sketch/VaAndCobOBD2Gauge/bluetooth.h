/* Bluetooth function */
//----------------------------------
//convert bt address to text
//{0x00,0x1d,0xa5,0x00,0x12,0x92} -> 00:1d:a5:00:12:92
String ByteArraytoString(esp_bd_addr_t bt_address) {
  String txt = "";
  String nib = "";
  for (uint8_t i=0;i<ESP_BD_ADDR_LEN-1;i++) {//0-4
    nib = String(bt_address[i],HEX);
    if (nib.length() < 2) nib = "0"+nib;
    txt = txt + nib+":";
  }//for
    
  nib = String(bt_address[ESP_BD_ADDR_LEN-1],HEX);//5
  if (nib.length() < 2) nib = "0"+nib;
  txt = txt + nib;
  return txt;
}
/*=======================*/

bool hasValidBtAddress(const esp_bd_addr_t bt_address) {
  for (uint8_t i = 0; i < ESP_BD_ADDR_LEN; i++) {
    if (bt_address[i] != 0) return true;
  }
  return false;
}

bool parseBtAddress(String address, esp_bd_addr_t bt_address) {
  for (uint8_t i = 0; i < ESP_BD_ADDR_LEN; i++) {
    int8_t separator = address.indexOf(':');
    String octet = separator == -1 ? address : address.substring(0, separator);
    if (octet.length() != 2) return false;

    char *end;
    long value = strtol(octet.c_str(), &end, 16);
    if (*end != '\0' || value < 0 || value > 0xFF) return false;
    bt_address[i] = value;

    if (separator == -1) return i == ESP_BD_ADDR_LEN - 1;
    address = address.substring(separator + 1);
  }
  return false;
}

bool isObd2Adapter(const String &name, const String &address) {
  if (address.equalsIgnoreCase(client_mac)) return true;

  String normalizedName = name;
  normalizedName.toUpperCase();
  return normalizedName == client_name ||
         normalizedName.indexOf("OBD") >= 0 ||
         normalizedName.indexOf("ELM") >= 0 ||
         normalizedName.indexOf("CBT") >= 0;
}

void scanBTdevice() {//scan bluetooth device
    digitalWrite(LED_BLUE_PIN,LOW);//blue led on
    Serial.println("Scanning for OBDII Adaptor...");
    Terminal("Scanning for OBDII Adaptor...",0,48,320,191);
    btDeviceCount = 0;
    foundOBD2 = false;
 // BTScanResults* btDeviceList = BTSerial.getScanResults();  // maybe accessing from different threads!
  if (BTSerial.discoverAsync([](BTAdvertisedDevice* pDevice) {
      // BTAdvertisedDeviceSet*set = reinterpret_cast<BTAdvertisedDeviceSet*>(pDevice);
      // btDeviceList[pDevice->getAddress()] = * set;
    if (btDeviceCount >= 8) return;
    String txt = pDevice->toString().c_str();
    Serial.printf("Found a new device: %s\n", pDevice->toString().c_str());
    deviceName[btDeviceCount] = pDevice->getName().c_str();
    deviceAddr[btDeviceCount] = pDevice->getAddress().toString().c_str();
    btDeviceCount++; 
    } )
    ) {
    delay(BT_DISCOVER_TIME);
    BTSerial.discoverAsyncStop();
    #ifdef SERIAL_DEBUG
    Serial.println("Discovering stopped");
    #endif
    digitalWrite(LED_BLUE_PIN,HIGH);//blue led off
    delay(1000);//redsicovery delay
  
  } else {
    #ifdef SERIAL_DEBUG
    Serial.println("Error on discovering bluetooth clients.");
    #endif
  }

  String txt = "Found "+String(btDeviceCount)+" device(s)";
  Terminal(txt,0,48,320,191);

  //matching scan obd2 and config obd2
  for (uint8_t i=0;i<btDeviceCount;i++) {
    txt = String(i+1)+". "+deviceName[i]+" - "+deviceAddr[i];
    Terminal(txt,0,48,320,191);//list devices
    delay(100)  ;

    if (isObd2Adapter(deviceName[i], deviceAddr[i])) {
      if (!parseBtAddress(deviceAddr[i], client_addr)) {
        Serial.println(F("Invalid OBDII adaptor address"));
        continue;
      }
      foundOBD2 = true;
      if (ByteArraytoString(client_addr) != ByteArraytoString(recent_client_addr)) {
        pref.putBytes("recent_client",client_addr,sizeof(client_addr));//save new bt addr to pref.
        Serial.println(F("Save a new BT client address"));
      }
      break;
    }
 
  }//for loop list device


  //connect to obd2
  if (foundOBD2) {
    txt = "Connecting to " + client_name +" - " + ByteArraytoString(client_addr);
    Terminal(txt,0,48,320,191);
    Serial.println(txt);
    BTSerial.connect(client_addr, 0, sec_mask, role);//connect to OBDII adaptor
    uint8_t try_count = 0;
    bool blink = false;
    while(!BTSerial.connected(1000) && try_count < 5) {
      Serial.print(F("."));
      blink =! blink;
      digitalWrite(LED_BLUE_PIN,blink);//blue led offset
      if (blink) tft.setTextColor(TFT_BLUE,TFT_BLACK);
      else tft.setTextColor(TFT_BLACK,TFT_BLACK);
      tft.drawRightString("$",319,210,4);
      try_count++;
    }

    if (BTSerial.connected()) {
      Terminal("Connected Successfully!",0,48,320,191);
      Serial.println(F("Connected Successfully!"));
      prompt = true;
      digitalWrite(LED_GREEN_PIN, LOW);//green led
    } else {
      Terminal("OBDII Adaptor connection failed!",0,48,320,191);
      Serial.println(F("OBDII Adaptor connection failed!"));
      BTSerial.disconnect();
      foundOBD2 = false;
    }

  } else {
    Terminal("OBDII Adaptor not found!",0,48,320,191);
    Serial.println(F("OBDII Adaptor not found!"));
  } //foundOBD2
}//scanBTDevice

//---------------------------
//connect to recent obdII for fast connection, skip scanning
void connectLastOBDII() {
  digitalWrite(LED_BLUE_PIN,LOW);//blue led on  
  String txt = "Connecting to " + client_name +" - " + ByteArraytoString(recent_client_addr);
  Terminal(txt,0,48,320,191);
  Serial.println(txt);
  BTSerial.connect(recent_client_addr, 0, sec_mask, role);//connect to OBDII adaptor
  uint8_t try_count = 0;
  bool blink = false;
  while(!BTSerial.connected(1000) && (try_count <3)) {
    Serial.print(F("."));
    blink =! blink;
    digitalWrite(LED_BLUE_PIN,blink);//blue led offset
    try_count++;
  }
  if (try_count == 3) {//cannot connect
    Terminal("OBDII Adaptor not found!",0,48,320,191);
    Serial.println(F("OBDII Adaptor not found!"));
    BTSerial.disconnect();
    foundOBD2 = false;
  } else {  
    Terminal("Connected Successfully!",0,48,320,191);  
    Serial.println(F("Connected Successfully!"));
    prompt = true;
    digitalWrite(LED_GREEN_PIN, LOW);//green led 
    foundOBD2 = true;
  }
 
}//connectLasbtOBDII