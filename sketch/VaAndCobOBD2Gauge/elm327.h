//elm327 interpreter

//DTC prefex code mapping
const String dtcMap[16] = {"P0","P1","P2","P3","C0","C1","C2","C3","B0","B1","B2","B3","U0","U1","U2","U3"};

//get A B from a Mode 01 response return in global A B variables
void getAB2(const String& elm_rsp, const String& mode, const String& para) {
  String normalized = "";
  for (uint16_t index = 0; index < elm_rsp.length(); index++) {
    char character = elm_rsp[index];
    if (isxdigit(character)) normalized += (char)toupper(character);
  }

  String responsePrefix = mode + para;
  int responseIndex = normalized.indexOf(responsePrefix);
  if (responseIndex < 0 || responseIndex + responsePrefix.length() + 2 > normalized.length()) {
    A = 0xFF;
    B = 0;
  } else {
    int valueIndex = responseIndex + responsePrefix.length();
    A = strtol(normalized.substring(valueIndex, valueIndex + 2).c_str(), nullptr, 16);
    B = (valueIndex + 4 <= normalized.length()) ? strtol(normalized.substring(valueIndex + 2, valueIndex + 4).c_str(), nullptr, 16) : 0;
  }

  Serial.printf("ELM %s%s raw: [%s] normalized: [%s]\n", mode.c_str(), para.c_str(), elm_rsp.c_str(), normalized.c_str());
}//getAB
/*------------------*/ 
String getPID(const String& pid) {//function getpid response from elm327
  while (BTSerial.available() > 0) {//clear rx buffer
    BTSerial.read();
  }

  String bt_response = "";
  BTSerial.print(pid + "\r");
  unsigned long startedAt = millis();
  bool receivedPrompt = false;

  while (millis() - startedAt < 2000 && !receivedPrompt) {
    while (BTSerial.available() > 0) {
      char incomingChar = BTSerial.read();
      if (incomingChar == '>') {
        receivedPrompt = true;
        break;
      }
      bt_response.concat(incomingChar);
    }
    if (!receivedPrompt) delay(5);
  }

  bt_response.trim();
  Serial.printf("ELM %s %s: [%s]\n", pid.c_str(), receivedPrompt ? "response" : "timeout", bt_response.c_str());
  return bt_response;
}
/*------------------*/ 
/* VIN reading
String resp = "014 
0: 49 02 01 4D 50 42 
1: 41 4D 46 30 36 30 4E 
2: 58 34 33 37 30 39 33 "; */
//convert ascii code to Charactor
const char asciiTable[0x60]= " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}`";
char HextoChar(uint8_t asciiCode) {
  if ((asciiCode > 0x20) && (asciiCode < 0x80)) {
      return(asciiTable[asciiCode-32]);
  }
  else return '\0';//null-terminator
}
//----------------------------
//function get VIN   Serial.println(getVIN(getPID(0902)));
String getVIN(String elm_rsp) {
  String VIN = "";
  uint8_t byteCount = 0;
  elm_rsp.trim();
  while (elm_rsp.length() > 0) {//keep reading each char
    int index = elm_rsp.indexOf(' ');//check space
    String getByte =  elm_rsp.substring(0, index);//get first byte
    if (index == -1)  {//no space found
        byte ascii = strtol(getByte.c_str(), NULL, 16);//read ascii
        VIN.concat(HextoChar(ascii));//last char
        //check correct VIN
        if (VIN.length() == byteCount) {
          VIN = VIN.substring(3);//remove first 3 string
          Serial.println("VIN: " + VIN);
          return VIN;//return VIN number
        } else {
          return "Cannot read VIN";
        }

    } else {//found space
      if (getByte.indexOf(':') == -1 ) {
        if (getByte.length() >= 3) {
          byteCount = strtol(getByte.c_str(), NULL, 16);//get byte count
        } else {//skip ':' byte
          byte ascii = strtol(getByte.c_str(), NULL, 16);//read ascii
          VIN.concat(HextoChar(ascii));//convert to hex charactor
        }
      } 
      elm_rsp = elm_rsp.substring(index + 1);//copy the rest behind space to elm_rsp.
    }//else
  }  //while
  return "Cannot read VIN";
}//getVIN
//----------------------------
