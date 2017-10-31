/*
 * SENTENCES:
 * 
 * 
 * $IIPSP,1,2,3*4
 * $IIPSP = Set pattern
 * 1      = FRONT or BACK
 * 2      = Position (1 to 4)
 * 3      = Pattern (e.g. 1100110011001100)
 * 4      = Checksum
 * 
 * $IIPSI,1,2,3*4
 * $IIPSI = Set intensity
 * 1      = FRONT or BACK
 * 2      = Mode (FLASH, ON or OFF)
 * 3      = Intensity (0 to 255)
 * 4      = Checksum
 * 
 * $IIPSD,1*2
 * $IIPSD = Set cycle duration
 * 1      = Duration in millisecunds (0 - 4000)
 * 2      = Checksum
 * 
 * $IIPRS*1
 * $IIPRS = Request save
 * 1      = Checksum
 * 
 * $IIPRD,1,2,3*4
 * $IIPRD = Request data
 * 1      = Data type (PATTERN, INTENSITY or DURATION)
 * 2      = FRONT og BACK
 * 3      = Position (1 to 4)
 * 4      = Checksum
 * 
 * $IIPRP*1
 * $IIPRP = Request ping
 * 1      = Ping Request
 * 
 * 
 * SENTENCES:
 * Outgoing
 * 
 * $IIPRR,1,2,3,4*5
 * $IIPRR = Request response
 * 1      = Response type (SAVE, PATTERN, INTENSITY og DURATION)
 * 2      = <empty>,FRONT og BACK
 * 3      = <empty> or 1 to 4
 * 4      = <value>
 * 5      = Checksum
 * 
 * $IIPRP,1*2
 * $IIPRP = Request ping
 * 1      = Ping request
 * 2      = Ping responce (OK)
 * 
 */


#include <SoftwareSerial.h>
#include <EEPROM.h>

#define BLETX 8
#define BLERX 12
#define BLEPOWER 13
#define ON_INPUT A1
#define FLASH_INPUT A2
#define SET_DEFAULT A5
#define OUT1 6
#define OUT2 3
#define OUT3 9
#define OUT4 10
#define OUT5 11
#define OUT6 5
#define BACK OUT1
#define DISP OUT2
#define FRONT1 OUT3
#define FRONT2 OUT4
#define FRONT3 OUT5
#define FRONT4 OUT6
#define DEBUG Serial
#define ON HIGH
#define OFF LOW

int const senLenght = 16;
int crcPos = senLenght - 1;
String sentence[senLenght];
bool nmeaSentence = false;
const byte buff_size = 80;
char buffer[buff_size];
byte index = 0;
int start_with = 0;
int end_with = 0;
byte CRC = 0;
long updateTimer = 0;
int cycleStep = 0;
char upToDate[] = "Is up to date";
char invalidSentence[] = "Invalid sentence!!";



SoftwareSerial BLE(BLERX, BLETX);

long baudBLE              = 19200;
long baudDBUG             = 115200;

// Defining the default patterns
String patternF1          = "1010000010100000";
const int patternF1EP[]           = {0,15};
String patternF2          = "0101000001010000";
const int patternF2EP[]           = {16,31};
String patternF3          = "0000010100000101";
const int patternF3EP[]           = {32,47};
String patternF4          = "0000101000001010";
const int patternF4EP[]           = {48,63};
String patternB1          = "1111000011110000";
const int patternB1EP[]           = {64,79};

// Defining the default intensities
int intensityFrontFlash   = 255;
const int intensityFrontFlashEP[] = {80,0};
int intensityFrontOFF     = 0;
const int intensityFrontOFFEP[]   = {81,0};
int intensityFront1ON      = 255;
const int intensityFront1ONEP[]    = {82,0};
int intensityFront2ON      = 255;
const int intensityFront2ONEP[]    = {83,0};
int intensityFront3ON      = 255;
const int intensityFront3ONEP[]    = {84,0};
int intensityFront4ON      = 255;
const int intensityFront4ONEP[]    = {85,0};
int intensityBackFlash    = 60;
const int intensityBackFlashEP[]  = {86,0};
int intensityBackON       = 255;
const int intensityBackONEP[]     = {87,0};
int cycleDuration         = 1000;
const int cycleDurationEP[]       = {88,103};


void setup() {
  pinMode(BLERX, INPUT);
  pinMode(BLETX, OUTPUT);
  pinMode(BLEPOWER, OUTPUT);
  digitalWrite(BLEPOWER, ON);
  BLE.begin(baudBLE);
  DEBUG.begin(baudDBUG);
  delay(500);
  pinMode(SET_DEFAULT, INPUT);
  if(digitalRead(SET_DEFAULT) == HIGH) {
    Serial.println("Setting defaults!!");
    BLE.println("AT+NAMEIIP ATV Light 2.0");
    resetToDefaults();
  } else {
    Serial.println("Booting!!");
    getVariablesFromEEPROM();
  }
  pinMode(FRONT1, OUTPUT);
  pinMode(FRONT2, OUTPUT);
  pinMode(FRONT3, OUTPUT);
  pinMode(FRONT4, OUTPUT);
  pinMode(BACK, OUTPUT);
  pinMode(DISP, OUTPUT);
  pinMode(ON_INPUT, INPUT);
  pinMode(FLASH_INPUT, INPUT);
  Serial.println("Boot sequence completed!!");
}

void loop() {
  if(BLE.available()) {
    DEBUG.println("Reading!!!");
    readSentence();
  }
  manageLight();
}

void readSentence() {
  int si = 0;
  while (BLE.available() > 0){
    char inchar = BLE.read();
    DEBUG.write(inchar);
    if(nmeaSentence) {
      buffer[index]  = inchar;
    }

    if( String(inchar) == "$" && si == 0) {
      nmeaSentence = true;
      buffer[index]  = inchar;
      start_with = index;
      sentence[si] = "";
    } else if(String(inchar) == "," && nmeaSentence) {
      si++;
      sentence[si] = "";
    } else if(String(inchar) == "*" && nmeaSentence) {
      end_with = index;
      si = crcPos;
      sentence[si] = "";
    } else if(nmeaSentence) {
      if(inchar, HEX == 30) {
        sentence[si] += "0";
      } else {
        sentence[si].concat(inchar);
      }
    }
    index++;
     
    delay(1);
  } 
  index=0;
  if(nmeaSentence) {
    DEBUG.println("Calculating checksum!!");
    if(start_with >= end_with) {
      CRC = 0;
      for(int i = 0; i<sizeof(sentence);i++) {
        sentence[i] = "";
      }
      DEBUG.println("Error!!!");
    } else {
      for (byte x = start_with+1; x<end_with; x++){ // XOR every character in between '$' and '*'
        CRC = CRC ^ buffer[x] ;
      }
    }
    
    String temp = "";
    if(CRC > 0){
      if(CRC < 0x10) {
         DEBUG.print(0);
         DEBUG.println(CRC, HEX);
         temp = "0";
         temp += String(CRC, HEX);
      } else {
         DEBUG.println(CRC, HEX);  // print calculated CS in HEX format.
         temp = String(CRC, HEX);
      }
      //CRC = 0; // reset CRC variable
    } else {
      DEBUG.println(invalidSentence);
      for(int i = 0; i<sizeof(sentence);i++) {
        sentence[i] = "";
      }
    }
    //String temp = String(CRC, HEX);
    temp.toUpperCase();
    if(sentence[crcPos] == temp) {
      DEBUG.println("Checksum pass!!");
      CRC = 0;
      exeCommand();
    } else {
      DEBUG.println("Checksum error!!");
      CRC = 0;
      nmeaSentence = false;
      for(int i = 0; i<sizeof(sentence);i++) {
        sentence[i] = "";
      }
      DEBUG.print(sentence[crcPos]);
      DEBUG.print(" = ");
      DEBUG.println(temp);
    }
    nmeaSentence = false;
  }
}

void exeCommand() {
  if(sentence[0] == "IIPSP") {    // Set pattern
    if(sentence[1] == "FRONT") {
      switch(sentence[2].toInt()) {
       case 1:
          patternF1 = sentence[3];
          return;
       case 2:
          patternF2 = sentence[3];
          return;
       case 3:
          patternF3 = sentence[3];
          return;
       case 4:
          patternF4 = sentence[3];
          return;
       default:
          DEBUG.println(invalidSentence);
          return;
      } 
    } else if(sentence[1] == "BACK") {  // Request save
      switch(sentence[2].toInt()) {
       case 1:
          patternB1 = sentence[3];
          return;
       default:
          DEBUG.println(invalidSentence);
          return;
      }
    } else if(sentence[1] == "SPEED") {  // Request save
          cycleDuration = sentence[2].toInt();
    } else if(sentence[1] == "INTENSITY") {  // Request save
      if(sentence[2] == "FRONTFLASH") {
        intensityFrontFlash = sentence[3].toInt();
      } else if(sentence[2] == "BACKFLASH") {
        intensityBackFlash = sentence[3].toInt();
      } else if(sentence[2] == "FRONT1ON") {
        intensityFront1ON = sentence[3].toInt();
      } else if(sentence[2] == "FRONT2ON") {
        intensityFront2ON = sentence[3].toInt();
      } else if(sentence[2] == "FRONT3ON") {
        intensityFront3ON = sentence[3].toInt();
      } else if(sentence[2] == "FRONT4ON") {
        intensityFront4ON = sentence[3].toInt();
      } else if(sentence[2] == "BACKON") {
        intensityBackON = sentence[3].toInt();
      } else {
        DEBUG.println(invalidSentence);
      }
    } else {
      DEBUG.println(invalidSentence);
    }
  } else if(sentence[0] == "IIPSI") {   // Set intensity
    if(sentence[1] == "FRONT") {
      if(sentence[2] == "FLASH") {
        intensityFrontFlash = sentence[3].toInt();
      } else if(sentence[2] == "1ON") {
        intensityFront1ON = sentence[3].toInt();
      } else if(sentence[2] == "2ON") {
        intensityFront2ON = sentence[3].toInt();
      } else if(sentence[2] == "3ON") {
        intensityFront3ON = sentence[3].toInt();
      } else if(sentence[2] == "4ON") {
        intensityFront4ON = sentence[3].toInt();
      } else if(sentence[2] == "OFF") {
        intensityFrontOFF = sentence[3].toInt();
      } else {
        DEBUG.println(invalidSentence);
      }
    } else if(sentence[1] == "BACK") {
      if(sentence[2] == "FLASH") {
        intensityBackFlash = sentence[3].toInt();
      } else if(sentence[2] == "ON") {
        intensityBackON = sentence[3].toInt();
      } else {
        DEBUG.println(invalidSentence);
      }
    } else {
      DEBUG.println(invalidSentence);
    }
  } else if(sentence[0] == "IIPSD") {   // Set duration
    if(sentence[1].toInt() >= 100 && sentence[1].toInt() <= 4000) {
      cycleDuration = sentence[1].toInt();
    } else {
      DEBUG.println(sentence[1].toInt());
      DEBUG.println(invalidSentence);
    }
  } else if(sentence[0] == "IIPRS") {   // Request Save
    DEBUG.println("Saving changes to EEPROM!!");
    saveVariablesToEEPROM();
  } else if(sentence[0] == "IIPPE") {   // Print EEPROM $IIPPE*45
    printEEPROM();
  } else if(sentence[0] == "IIPRR") {   // Request Reboot $IIRR*50
    reboot();
  } else if(sentence[0] == "IIPRP") {   // Request All Data
    String temp = "";
    temp = "IIPRP,OK";
    temp += "*" + getCheckSum(temp);
    DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
  } else if(sentence[0] == "IIPRAD") {   // Request All Data
    String temp = "";
    byte cs = "";
    temp = "IIPRP,OK";
    cs = getCheckSum(temp);
    //DEBUG.println(cs, HEX);
    //DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
    BLE.print("*");
    BLE.print(cs, HEX);
    delay(150);
     
    temp = "IIPRR,FRONT,1,";
    temp += patternF1;
    cs = getCheckSum(temp);
    //DEBUG.println(cs, HEX);
    //DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
    BLE.print("*");
    if(cs < 0x10) {
      BLE.print(0);
    }
    BLE.print(cs, HEX);
    delay(150);
     
    temp = "IIPRR,FRONT,2,";
    temp += patternF2;
    cs = getCheckSum(temp);
    //DEBUG.println(cs, HEX);
    //DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
    BLE.print("*");
    if(cs < 0x10) {
      BLE.print(0);
    }
    BLE.print(cs, HEX);
    delay(150);
     
    temp = "IIPRR,FRONT,3,";
    temp += patternF3;
    cs = getCheckSum(temp);
    //DEBUG.println(cs, HEX);
    //DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
    BLE.print("*");
    if(cs < 0x10) {
      BLE.print(0);
    }
    BLE.print(cs, HEX);
    delay(150);
     
    temp = "IIPRR,FRONT,4,";
    temp += patternF4;
    cs = getCheckSum(temp);
    //DEBUG.println(cs, HEX);
    //DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
    BLE.print("*");
    if(cs < 0x10) {
      BLE.print(0);
    }
    BLE.print(cs, HEX);
    delay(150);
     
    temp = "IIPRR,BACK,1,";
    temp += patternB1;
    cs = getCheckSum(temp);
    //DEBUG.println(cs, HEX);
    //DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
    BLE.print("*");
    if(cs < 0x10) {
      BLE.print(0);
    }
    BLE.print(cs, HEX);
    delay(150);
     
    temp = "IIPRR,SPEED,";
    temp += cycleDuration;
    cs = getCheckSum(temp);
    //DEBUG.println(cs, HEX);
    //DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
    BLE.print("*");
    if(cs < 0x10) {
      BLE.print(0);
    }
    BLE.print(cs, HEX);
    delay(150);
     
    temp = "IIPRR,INTENSITY,FRONTFLASH,";
    temp += intensityFrontFlash;
    cs = getCheckSum(temp);
    //DEBUG.println(cs, HEX);
    //DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
    BLE.print("*");
    if(cs < 0x10) {
      BLE.print(0);
    }
    BLE.print(cs, HEX);
    delay(150);
     
    temp = "IIPRR,INTENSITY,BACKFLASH,";
    temp += intensityBackFlash;
    cs = getCheckSum(temp);
    //DEBUG.println(cs, HEX);
    //DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
    BLE.print("*");
    if(cs < 0x10) {
      BLE.print(0);
    }
    BLE.print(cs, HEX);
    delay(150);
     
    temp = "IIPRR,INTENSITY,FRONT1ON,";
    temp += intensityFront1ON;
    cs = getCheckSum(temp);
    //DEBUG.println(cs, HEX);
    //DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
    BLE.print("*");
    if(cs < 0x10) {
      BLE.print(0);
    }
    BLE.print(cs, HEX);
    delay(150);
     
    temp = "IIPRR,INTENSITY,FRONT2ON,";
    temp += intensityFront2ON;
    cs = getCheckSum(temp);
    //DEBUG.println(cs, HEX);
    //DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
    BLE.print("*");
    if(cs < 0x10) {
      BLE.print(0);
    }
    BLE.print(cs, HEX);
    delay(150);
     
    temp = "IIPRR,INTENSITY,FRONT3ON,";
    temp += intensityFront3ON;
    cs = getCheckSum(temp);
    //DEBUG.println(cs, HEX);
    //DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
    BLE.print("*");
    if(cs < 0x10) {
      BLE.print(0);
    }
    BLE.print(cs, HEX);
    delay(150);
     
    temp = "IIPRR,INTENSITY,FRONT4ON,";
    temp += intensityFront4ON;
    cs = getCheckSum(temp);
    //DEBUG.println(cs, HEX);
    //DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
    BLE.print("*");
    if(cs < 0x10) {
      BLE.print(0);
    }
    BLE.print(cs, HEX);
    delay(150);
     
    temp = "IIPRR,INTENSITY,BACKON,";
    temp += intensityBackON;
    cs = getCheckSum(temp);
    //DEBUG.println(cs, HEX);
    //DEBUG.println("$" + temp);
    BLE.print("$");
    BLE.print(temp);
    BLE.print("*");
    if(cs < 0x10) {
      BLE.print(0);
    }
    BLE.print(cs, HEX);
  } else {
    DEBUG.println(invalidSentence);
    DEBUG.println(sentence[0]);
  }
}

uint8_t getCheckSum(String string) {
  /*DB.println(start_with);
  DB.println(end_with);
  DB.println(string);*/
  //DB.println(F("Calculating checksum!!"));
  int XOR = 0;  
  for (int i = 0; i < string.length(); i++) 
  {
    XOR = XOR ^ char(string[i]);
  }
  return XOR;
}

void manageLight() {
  long now = millis();
  if(updateTimer <= now) {
    updateTimer = now+(double(cycleDuration)/16.00);
    if(digitalRead(ON_INPUT) || digitalRead(FLASH_INPUT)) {
      if(digitalRead(FLASH_INPUT)) {
        analogWrite(FRONT1, (String(patternF1[cycleStep]).toInt()) ? intensityFrontFlash : intensityFrontOFF);
        analogWrite(FRONT2, (String(patternF2[cycleStep]).toInt()) ? intensityFrontFlash : intensityFrontOFF);
        analogWrite(FRONT3, (String(patternF3[cycleStep]).toInt()) ? intensityFrontFlash : intensityFrontOFF);
        analogWrite(FRONT4, (String(patternF4[cycleStep]).toInt()) ? intensityFrontFlash : intensityFrontOFF);
        analogWrite(BACK, (String(patternB1[cycleStep]).toInt()) ? intensityBackFlash : intensityFrontOFF);
      } else {
        analogWrite(FRONT1, intensityFront1ON);
        analogWrite(FRONT2, intensityFront2ON);
        analogWrite(FRONT3, intensityFront3ON);
        analogWrite(FRONT4, intensityFront4ON);
        analogWrite(BACK, intensityBackON);
      }
    } else {
      analogWrite(FRONT1, 0);
      analogWrite(FRONT2, 0);
      analogWrite(FRONT3, 0);
      analogWrite(FRONT4, 0);
      analogWrite(BACK, 0);
    }  
    cycleStep++;
    if(cycleStep > 15) {
      cycleStep = 0;
    }
  }
}

String readEEPROMString(int address[]) {
  String read = "";
  for(int i = address[0]; i <= address[1]; i++) {
    read.concat(char(EEPROM.read(i)));
  }
  return read;
}
bool writeEEPROMString(int address[], String data) {
  if(sizeof(data) <= (address[1]-address[0]+1)) {
    for(int i = address[0]; i <= address[1]; i++) {
      EEPROM.write(i, data[i-address[0]]);
    }
    return true;
  } else {
    return false;
  }
}
int readEEPROMint(int address[]) {
  int read = 0;
  if(address[1] > 0) {
    for(int i = address[0]; i <= address[1]; i++) {
      read += EEPROM.read(i);
    }
  } else {
    read = EEPROM.read(address[0]);
  }
  return read;
}
bool writeEEPROMint(int address[], int data) {
  if(address[1] > 0) {
    if((data/255) <= (address[1]-address[0]+1)) {
      for(int i = address[0]; i <= address[1]; i++) {
        if(data > 255) {
          EEPROM.write(i, 255);
          data -= 255;
        } else {
          EEPROM.write(i, data);
          data -= data;
        }
      }
      return true;
    } else {
      return false;
    }
  } else {
   if(data <= 255) {
     EEPROM.write(address[0], data);
     return true;
   } else {
     return false;
   }
  }
}
void getVariablesFromEEPROM() {
  patternF1 = readEEPROMString(patternF1EP);
  //DEBUG.println(patternF1);
  patternF2 = readEEPROMString(patternF2EP);
  //DEBUG.println(patternF2);
  patternF3 = readEEPROMString(patternF3EP);
  //DEBUG.println(patternF3);
  patternF4 = readEEPROMString(patternF4EP);
  //DEBUG.println(patternF4);
  patternB1 = readEEPROMString(patternB1EP);
  //DEBUG.println(patternB1);
  intensityFrontFlash = readEEPROMint(intensityFrontFlashEP);
  //DEBUG.println(intensityFrontFlash);
  intensityFrontOFF = readEEPROMint(intensityFrontOFFEP);
  //DEBUG.println(intensityFrontOFF);
  intensityFront1ON = readEEPROMint(intensityFront1ONEP);
  //DEBUG.println(intensityFront1ON);
  intensityFront2ON = readEEPROMint(intensityFront2ONEP);
  //DEBUG.println(intensityFront2ON);
  intensityFront3ON = readEEPROMint(intensityFront3ONEP);
  //DEBUG.println(intensityFront3ON);
  intensityFront4ON = readEEPROMint(intensityFront4ONEP);
  //DEBUG.println(intensityFront4ON);
  intensityBackFlash = readEEPROMint(intensityBackFlashEP);
  //DEBUG.println(intensityBackFlash);
  intensityBackON = readEEPROMint(intensityBackONEP);
  //DEBUG.println(intensityBackON);
  cycleDuration = readEEPROMint(cycleDurationEP);
  //DEBUG.println(cycleDuration);
}
void saveVariablesToEEPROM() {
  if(readEEPROMString(patternF1EP) != patternF1) {
    writeEEPROMString(patternF1EP, patternF1);
  } else {
    /*DEBUG.print("PatternF1 ");
    DEBUG.println(upToDate);*/
  }
  if(readEEPROMString(patternF2EP) != patternF2) {
    writeEEPROMString(patternF2EP, patternF2);
  } else {
    //DEBUG.print("patternF2 ");
    //DEBUG.println(upToDate);
  }
  if(readEEPROMString(patternF3EP) != patternF3) {
    writeEEPROMString(patternF3EP, patternF3);
  } else {
    //DEBUG.print("patternF3 ");
    //DEBUG.println(upToDate);
  }
  if(readEEPROMString(patternF4EP) != patternF4) {
    writeEEPROMString(patternF4EP, patternF4);
  } else {
    //DEBUG.print("patternF4 ");
    //DEBUG.println(upToDate);
  }
  if(readEEPROMString(patternB1EP) != patternB1) {
    writeEEPROMString(patternB1EP, patternB1);
  } else {
    //DEBUG.print("patternB1 ");
    //DEBUG.println(upToDate);
  }
  if(readEEPROMint(intensityFrontFlashEP) != intensityFrontFlash) {
    writeEEPROMint(intensityFrontFlashEP, intensityFrontFlash);
  } else {
    //DEBUG.print("intensityFrontFlash ");
    //DEBUG.println(upToDate);
  }
  if(readEEPROMint(intensityFrontOFFEP) != intensityFrontOFF) {
    writeEEPROMint(intensityFrontOFFEP, intensityFrontOFF);
  } else {
    //DEBUG.print("intensityFrontOFF ");
    //DEBUG.println(upToDate);
  }
  if(readEEPROMint(intensityFront1ONEP) != intensityFront1ON) {
    writeEEPROMint(intensityFront1ONEP, intensityFront1ON);
  } else {
    //DEBUG.print("intensityFrontON ");
    //DEBUG.println(upToDate);
  }
  if(readEEPROMint(intensityFront2ONEP) != intensityFront2ON) {
    writeEEPROMint(intensityFront2ONEP, intensityFront2ON);
  } else {
    //DEBUG.print("intensityFrontON ");
    //DEBUG.println(upToDate);
  }
  if(readEEPROMint(intensityFront3ONEP) != intensityFront3ON) {
    writeEEPROMint(intensityFront3ONEP, intensityFront3ON);
  } else {
    //DEBUG.print("intensityFrontON ");
    //DEBUG.println(upToDate);
  }
  if(readEEPROMint(intensityFront4ONEP) != intensityFront4ON) {
    writeEEPROMint(intensityFront4ONEP, intensityFront4ON);
  } else {
    //DEBUG.print("intensityFrontON ");
    //DEBUG.println(upToDate);
  }
  if(readEEPROMint(intensityBackFlashEP) != intensityBackFlash) {
    writeEEPROMint(intensityBackFlashEP, intensityBackFlash);
  } else {
    //DEBUG.print("intensityBackFlash ");
    //DEBUG.println(upToDate);
  }
  if(readEEPROMint(intensityBackONEP) != intensityBackON) {
    writeEEPROMint(intensityBackONEP, intensityBackON);
  } else {
    //DEBUG.print("intensityBackON ");
    //DEBUG.println(upToDate);
  }
  if(readEEPROMint(cycleDurationEP) != cycleDuration) {
    writeEEPROMint(cycleDurationEP, cycleDuration);
  } else {
    //DEBUG.print("cycleDuration ");
    //DEBUG.println(upToDate);
  }
}
void printEEPROM() {
  for(int i = 0; i < 101; i++) {
    Serial.print("#");
    Serial.print(i);
    Serial.print(" = ");
    Serial.println(EEPROM.read(i));
  }
}
void resetToDefaults() {
  saveVariablesToEEPROM();
}
void reboot() {
  asm volatile ("  jmp 0");
  //delay(5000);
}

