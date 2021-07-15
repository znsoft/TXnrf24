// 6 Channel Transmitter | 6 Kanal Verici
#include <SPI.h>
#include <nRF24L01.h>
#include <LM15SGFNZ07.h>
#include <EEPROM.h>
#include <RF24.h>
LM15SGFNZ07 lcd(2, 3, 4);

const uint64_t pipeOut = 0xE9E8F0F0E1LL;   //IMPORTANT: The same as in the receiver 0xE9E8F0F0E1LL | Bu adres alıcı ile aynı olmalı
RF24 radio(9, 10); // select CE,CSN pin | CE ve CSN pinlerin seçimi


enum Modes { SELECTPA, RUN, BUTTONS, SHOWAXIS, DEFAXIS, CONTRAST, DEFINV};

long presstimes[4] = {0, 0, 0, 0};
long pressstarttimes[4];
Modes mode = RUN;

//Select,Down,Enter,Up
int Buttons[4] = {8, 7, 5, 6};

enum Control  {THROTTLE, YAW, PITCH, ROLL, AUX1, AUX2};
struct ControlPoint {
  Control def;
  char name[2];
  bool inv;
  int maxv;
  int minv;

};





int Axispins[6] = {A0, A1, A2, A3, A4, A5};


int BUZZPIN = 1;

// The sizeof this struct should not exceed 32 bytes
struct RF24Data {
  byte throttle;
  byte yaw;
  byte pitch;
  byte roll;
  byte AUX1;
  byte AUX2;
  byte switches;
};


RF24Data data;


//Signal data;
void ResetData()
{
  data.throttle = 12;   // Motor stop | Motor Kapalı (Signal lost position | sinyal kesildiğindeki pozisyon)
  data.pitch = 127;    // Center | Merkez (Signal lost position | sinyal kesildiğindeki pozisyon)
  data.roll = 127;     // Center | merkez (Signal lost position | sinyal kesildiğindeki pozisyon)
  data.yaw = 127;     // Center | merkez (Signal lost position | sinyal kesildiğindeki pozisyon)
  data.AUX1 = 127;    // Center | merkez (Signal lost position | sinyal kesildiğindeki pozisyon)
  data.AUX2 = 127;    // Center | merkez (Signal lost position | sinyal kesildiğindeki pozisyon)

}


struct MyConfig {
  char lcdcontrast;
  //char lcdcontrastonTX;
  rf24_pa_dbm_e level;
  char channel;

  ControlPoint controldefs[6] = {
    {THROTTLE, "t ", true, 1024, 0},
    {YAW,      "y ", false, 1024, 0},
    {PITCH,    "p ", true, 1024, 0},
    {ROLL,     "r ", false, 1024, 0},
    {AUX1,     "1 ", true, 1024, 0},
    {AUX2,     "2 ", true, 1024, 0},
  };
  int sign;
};

MyConfig mconfig;

void ResetConfig() {

  mconfig.lcdcontrast = 37;
  //mconfig.lcdcontrastonTX = 38;
  mconfig.level = RF24_PA_LOW;
  mconfig.channel = 0x5a;
  mconfig.sign = 12031;
}

void saveConfig() {

  EEPROM.put(0, mconfig);
}

void reloadConfig() {


  MyConfig customVar; //Variable to store custom object read from EEPROM.
  EEPROM.get(0, customVar);
  if (customVar.sign == mconfig.sign) {

    EEPROM.get(0, mconfig);
    //controldefs = mconfig.controldefs;
  }
  saveConfig();
}

#define maxcontrast 55
#define mincontrast 10

void selectContrast() {
  lcd.drawString("-Select contrast-", 18, 0,  0x0F0, 0xF1A);

  int scrval = map(mconfig.lcdcontrast, mincontrast, maxcontrast, 0, 97);

  lcd.fillRect(1, 20 - 1, 98, 18, 0x000 );
  lcd.fillRect(1, 20, scrval, 16, 0xF00 );
  lcd.fillRect(scrval, 20, 97 - scrval, 16,  0x1FE);
  lcd.drawString(String(mconfig.lcdcontrast, HEX).c_str(), 10, 25,  0xF00, 0xFEE);


  if (presstimes[1] > 100 && presstimes[1] < 150) {
    mconfig.lcdcontrast++;

    //presstimes[1] = 0;
    lcd.setContrast(mconfig.lcdcontrast);
    //lcd.clear(0xFFF);
    beep(1440, 50);
  }

  if (presstimes[3] > 100 && presstimes[3] < 150) {
    mconfig.lcdcontrast--;
    //lcd.clear(0xFFF);
    //presstimes[3] = 0;
    lcd.setContrast(mconfig.lcdcontrast);
    beep(1440, 50);
  }

  if (mconfig.lcdcontrast < mincontrast)mconfig.lcdcontrast = maxcontrast;
  if (mconfig.lcdcontrast > maxcontrast)mconfig.lcdcontrast = mincontrast;


}


unsigned int currentMovableAxis = 0;
void defineAxisInv() {
  lcd.drawString("-Define inversion-", 18, 0,  0x0F0, 0xF1A);
  int startstr = 10;
  for (int i = 0; i < 6; i += 1) {
    ControlPoint cp = mconfig.controldefs[i];

    Control def = cp.def;
    int val = analogRead(Axispins[i]);


    int maxv = cp.maxv;
    int minv = cp.minv;
    bool inv = cp.inv;
    int midv = (maxv + minv) / 2;
    byte res = mapJoystickValues( val, minv, midv, maxv, inv );

    int scrval = map(res, 0, 255, 0, 98);

    if (i == currentMovableAxis)lcd.fillRect(1, startstr + i * 10 - 1, 98, 11, 0x000 );
    lcd.fillRect(1, startstr + i * 10, scrval, 7, 0xF00 );
    lcd.fillRect(scrval, startstr + i * 10, 98 - scrval, 7,  0x1FE);
    lcd.drawString(String(cp.name[0]).c_str(), 1, startstr + i * 10,  0xF00, 0xFEE);
    lcd.drawString(inv ? "1" : "0", 90, startstr + i * 10,  0xF00, 0xFEE);


  }

  if (presstimes[1] > 100 && presstimes[1] < 150) {
    currentMovableAxis++;

    //presstimes[1] = 0;
    lcd.clear(0xFFF);
    beep(1440, 100);
  }

  if (presstimes[3] > 100 && presstimes[3] < 150) {
    currentMovableAxis--;
    lcd.clear(0xFFF);
    //presstimes[3] = 0;
    beep(1440, 100);
  }

  currentMovableAxis = currentMovableAxis % 6;

  if (presstimes[2] > 100 && presstimes[2] < 150 && presstimes[3] == 0 && presstimes[1] == 0 ) {

    mconfig.controldefs[currentMovableAxis].inv = !mconfig.controldefs[currentMovableAxis].inv;

    lcd.clear(0xFFF);
    beep(440, 100);



  }



}


//unsigned int currentMovableAxis = 0;
void defineAxis() {
  lcd.drawString("-Define axis-", 18, 0,  0x0F0, 0xF1A);
  int startstr = 10;
  for (int i = 0; i < 6; i += 1) {
    ControlPoint cp = mconfig.controldefs[i];

    Control def = cp.def;
    int val = analogRead(Axispins[i]);


    int maxv = cp.maxv;
    int minv = cp.minv;
    bool inv = cp.inv;
    int midv = (maxv + minv) / 2;
    byte res = mapJoystickValues( val, minv, midv, maxv, inv );

    int scrval = map(res, 0, 255, 0, 98);

    if (i == currentMovableAxis)lcd.fillRect(1, startstr + i * 10 - 1, 98, 11, 0x000 );
    lcd.fillRect(1, startstr + i * 10, scrval, 7, 0xF00 );
    lcd.fillRect(scrval, startstr + i * 10, 98 - scrval, 7,  0x1FE);
    lcd.drawString(String(cp.name[0]).c_str(), 1, startstr + i * 10,  0xF00, 0xFEE);



  }

  if (presstimes[1] > 100 && presstimes[1] < 150) {
    currentMovableAxis++;

    //presstimes[1] = 0;
    lcd.clear(0xFFF);
    beep(1440, 100);
  }

  if (presstimes[3] > 100 && presstimes[3] < 150) {
    currentMovableAxis--;
    lcd.clear(0xFFF);
    //presstimes[3] = 0;
    beep(1440, 100);
  }

  currentMovableAxis = currentMovableAxis % 6;

  if (presstimes[2] > 100 && presstimes[2] < 150 && presstimes[3] == 0 && presstimes[1] == 0 ) {

    unsigned int nextMoveAxis = (currentMovableAxis + 1) % 6;
    ControlPoint  cp, cpswp, cpnext = mconfig.controldefs[nextMoveAxis];
    cp = mconfig.controldefs[currentMovableAxis];
    //cpswp = {cp.def,cp.name,cp.maxv,cp.minv.cp.inv,cp.pin};
    mconfig.controldefs[currentMovableAxis] = cpnext;
    mconfig.controldefs[nextMoveAxis] = cp;
    lcd.clear(0xFFF);
    beep(440, 100);



  }



}


rf24_pa_dbm_e levels[3] = {RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX};
String paStings[3] = {"LOW", "HIGH", "MAX"};

void selectPA() {
  unsigned int current;
  lcd.drawString("-Select PA-", 18, 1, 0x000, 0xFFF);
  for (int i = 0; i < 3; i += 1) {
    String pastr = paStings[i];
    if (levels[i] == mconfig.level) {
      current = i;
      lcd.drawString(pastr.c_str(), 18, 16 + i * 17, 0xFFF, 0x0F0);
    } else {
      lcd.drawString(pastr.c_str(), 18, 16 + i * 17, 0x000, 0xFFF);
    }

  }

  if (presstimes[1] > 200) {
    current++;
    mconfig.level = levels[current % 3];
    presstimes[1] = 0;
    radio.setPALevel(mconfig.level);
  }

  if (presstimes[3] > 200) {
    current--;
    mconfig.level = levels[current % 3];
    presstimes[3] = 0;
    radio.setPALevel(mconfig.level);
  }

  if (presstimes[2] > 200) {

    saveConfig();
    //presstimes[0] = 0;
  }

}


// Joystick center and its borders | Joystick merkez ve sınırları
int mapJoystickValues(int val, int lower, int middle, int upper, bool reverse)
{
  val = constrain(val, lower, upper);
  if ( val < middle )
    val = map(val, lower, middle, 0, 128);
  else
    val = map(val, middle, upper, 128, 255);
  return ( reverse ? 255 - val : val );
}



void RunTX() {

  int startstr = 17;
  for (int i = 0; i < 6; i += 1) {
    ControlPoint cp = mconfig.controldefs[i];

    Control def = cp.def;
    int val = analogRead(Axispins[i]);


    int maxv = cp.maxv;
    int minv = cp.minv;
    bool inv = cp.inv;
    int midv = (maxv + minv) / 2;
    byte res = mapJoystickValues( val, minv, midv, maxv, inv );

    int scrval = map(res, 0, 255, 0, 98);


    lcd.fillRect(1, startstr + i * 9, scrval, 7, 0xF00 );
    lcd.fillRect(scrval, startstr + i * 9, 98 - scrval, 7,  0x1FE);
    lcd.drawString(String(cp.name[0]).c_str(), 1, startstr + i * 9,  0xF00, 0xFEE);

    switch (def) {
      case THROTTLE:
        data.throttle = res;
        break;
      case YAW:
        data.yaw = res;
        break;
      case PITCH:
        data.pitch = res;
        break;
      case ROLL:
        data.roll = res;
        break;
      case AUX1:
        data.AUX1 = res;
        break;
      case AUX2:
        data.AUX2 = res;
        break;
    }

  }

  lcd.drawString("txpow", 1, 70, 0x000, 0x5F9);
  for (int i = 0; i < 3; i += 1) {
    String pastr = paStings[i];
    if (levels[i] == mconfig.level) {

      lcd.drawString(pastr.c_str(), 30, 70, 0x00F, 0x9F5);
    }

  }

  if (radio.write(&data, sizeof(RF24Data))) {
    lcd.drawString("Send", 18, 1, 0xFFF, 0xF00);
    //Serial.println("send");
  } else {
    lcd.drawString("Not Send", 18, 1, 0xF00, 0xFEE);
  };
}


void startLCD() {

  long c = millis();

  while (!digitalRead(Buttons[1])) {
    getButtons();
    showButtons();
    if ((millis() - c) > 1000) {
      //currentseq = 4;

      changeMode(CONTRAST);

      break;
    }
  }

  while (!digitalRead(Buttons[0])) {
    getButtons();
    showButtons();

    if ((millis() - c) > 2500) {

      ResetConfig();
      saveConfig();
      //nextMode();

      break;
    }
  }

}


//-----------------------------------
void showAxis() {
  for (int i = 0; i < 4; i += 1) {
    int val = analogRead(Axispins[i]);
    int scrval = map(val, 0, 1024, 0, 98);


    lcd.fillRect(1, i * 16, scrval, 15, 0xF00 );
    lcd.fillRect(scrval, i * 16, 98 - scrval, 15,  0x1FE);
    lcd.drawString(String(val).c_str(), 18, i * 16, 0xFFF, 0xF00);
  }

}

void showButtons() {
  for (int i = 0; i < 4; i += 1) {
    bool but = presstimes[i] > 0;
    lcd.fillRect(1, i * 16, 98, 15, but ? 0xF00 : 0x1FE);
    lcd.drawString(String(presstimes[i]).c_str(), 18, i * 16, 0xFFF, 0xF00);
  }
}
//--------------------------------
void beep(int f, int d) {
  pinMode(BUZZPIN, OUTPUT);
  tone(BUZZPIN, f);
  delay(d);
  noTone(BUZZPIN);
}
//-------------------------------------------------
void getButtons() {
  for (int i = 0; i < 4; i += 1) {
    bool but = digitalRead(Buttons[i]);
    if (presstimes[i] == 0) {
      pressstarttimes[i] = millis();
      if (!but)beep(1000, 100);
    }

    presstimes[i] = millis() - pressstarttimes[i];
    presstimes[i]++;
    if (but)presstimes[i] = 0;
  }
}
//----------------------------------------

Modes seqmode[] = {RUN, DEFAXIS, DEFINV, SELECTPA, CONTRAST};
unsigned int currentseq = 0;
void nextMode() {
  currentseq++;
  changeMode(seqmode[currentseq % (sizeof(seqmode) / sizeof(BUTTONS))]);
  // beep(1300,100);
}

void sequenceMode() {
  if (presstimes[0] > 2000 && presstimes[0] < 3000) {
    nextMode();
    //presstimes[0] = 0;
  }

}

void changeMode(Modes newmode) {
  lcd.init();
  lcd.clear(0xFFF);
  lcd.setContrast(mconfig.lcdcontrast);

  for (int i = 2; i < 80; i += 6) {
    lcd.drawLine(0, i, 100, i, 0x555);
  }

  for (int i = 2; i < 101; i += 6) {
    lcd.drawLine(i, 0, i, 79, 0x555);
  }
  // lcd.fillRect(1,  16, 98, 15,  0x1FE);
  //lcd.drawString(String(newmode).c_str(), 18,  1, 0xFFF, 0xF00);
  mode = newmode;
  beep(1700, 500);
  saveConfig();

}

void modeSwitch() {
  switch (mode) {
    case DEFINV:
      defineAxisInv();
      break;

    case CONTRAST:
      selectContrast();
      break;

    case DEFAXIS:
      defineAxis();

      break;
    case SELECTPA:
      selectPA();
      break;
    case SHOWAXIS:
      showAxis();
      break;
    case BUTTONS:
      showButtons();
      break;
    case RUN:
      RunTX();
      break;

  }
}
//------------------------------------------
void setup()
{
  //Serial.begin(115200);
  //Serial.println("initialized.");
  ResetConfig();
  reloadConfig();
  ResetData();

  //Serial.println("lcd start.");
  changeMode(RUN);
  for (int i = 0; i < 4; i += 1) {
    pinMode(Buttons[i], INPUT_PULLUP);
    digitalWrite(Buttons[i], HIGH);
  }
  startLCD();


  //Start everything up
  if (!radio.begin()) {
    lcd.drawString("ERROR", 25, 20, 0x000, 0xFFF);
    lcd.drawString("no radio found", 18, 35, 0x000, 0xFFF);
    //Serial.println(F("radio hardware is not responding!!"));
    //lcd.
    beep(1500, 3000);
    //while (1) {} // hold in infinite loop
  }
  lcd.drawString("open pipe", 25, 20, 0x000, 0xFFF);

  radio.openWritingPipe(pipeOut);
  radio.setAutoAck(false);
  radio.setChannel(0x5a);
  // save on transmission time by setting the radio to only transmit the
  // number of bytes we need to transmit a float
  radio.setPayloadSize(sizeof(RF24Data)); // float datatype occupies 4 bytes

  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(mconfig.level);
  radio.setRetries(4, 9);
  radio.stopListening(); //start the radio comunication for Transmitter | Verici olarak sinyal iletişimi başlatılıyor
  lcd.drawString("start", 18, 35, 0x000, 0xFFF);
  //Serial.println("start");
}




void loop()
{

  modeSwitch();
  getButtons();
  sequenceMode();
}
