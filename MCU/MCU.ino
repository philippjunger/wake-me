/***************************************************
   WakeMe Code

   Written by Lukas Hokenmaier 03-09-2020
   Modified by Philipp Junger 21-01-2021

   Calculation of SpO2 based on j.n.magee 15-10-2019 and MAXIM MAX30105
****************************************************/

#include "MAX30102.h"
#include "Pulse.h"
#include "wakeUser.h"
#include "EEPROM.h"

// Arduino Pins
#define LIPO_PIN A2
#define TOUCH_PIN 5
// #define SPEAKER_PIN 8

// Definitions
#define minCounts 50000 // minimal accepted counts from IR and RED
#define IR_RED_Counts_DIFF 25000 // Difference in counts beween IR and RED to ensure same brightness
#define Totzeit 30000 // Totzeit
#define timeAkkuCheck 60000 // Check Battery Voltage every second
#define IR_RED_COUNTS_FEHLERZEIT 100 // Durchläufe die abgewartet werden nachdem das Gerät kein gültiges IR oder RED Signal empfängt bevor lauter Alarm ertönt

// Class + Appreviations used in code
MAX30102 sensor;
Wecker wake;
SPEAKER speak;

Pulse pulseIR;
Pulse pulseRed;
BpmMAFilter bpm;
MAFilter derivateSpO2;

///////////////////

//spo2_table is approximated as  -45.060 * RX100 * RX100 + 30.354 * RX100 + 94.845 ;

const uint8_t spo2_table[184] PROGMEM =
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100, 100, 100, 100, 100, 100, 100,
  100, 100, 100, 100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 98, 98, 98, 97, 97,
  97, 97, 96, 96, 96, 96, 95, 95, 95, 94, 94, 94, 93, 93, 93, 92, 92, 92, 91, 91,
  90, 90, 89, 89, 89, 88, 88, 87, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81,
  80, 80, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68, 67,
  66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50,
  49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 31, 30, 29,
  28, 27, 26, 25, 23, 22, 21, 20, 19,17, 16, 15, 14, 12, 11, 10, 9, 7, 6, 5,
  3, 2, 1
} ;

// Initialisation
int     beatAvg;
int     SPO2;
int     SPO2counter = 0;
int     derSPO2counter = 0;
int     negderSPO2coursecounter = 0;
int     posderSPO2coursecounter = 0;
int     AlarmReason = 0; //  0:  all good
int     alarm1_saturationdrop = 8; // Number of saturation falls before Alarm1 starts
int     alarm2_below_alarmgrenze = 8; // Number of beats were saturation is below alarmgrenze before Alarm2 starts
int     nobeatcounter = 0;
int     beatcounter = 0;
bool    nobeat = false;
int     ir_red_counter = 0;

int     funktionszustand = 0; // 0: Fehler, 1: Überwachung aktiv, 2: Sensorposition prüfen, 3: Gerät bereit, 4: Alarm, 5: Alarmpause

long    lastBeat = 0;    //Time of the last beat
long    lastRedBeat = 0;

long    lastAkkuCheck = 0;

float   SPO2f = 0;
float   lastSPO2f = 0;
float   derSPO2 = 0;    // zeitliche Änderung von SpO2
float   lastderSPO2 = 0; // vorherige zeitliche Änderung von SpO2
float   perfusionIR = 0;  //
float   perfusionRed = 0;


bool    broken_system = false;
bool    Start = true;
bool    beatRed, beatIR;
bool    Alarm = false;
bool    AlarmReason1 = false;  //  1:  SPO2 fällt ab --> solange über Minimum nicht kritisch//
bool    AlarmReason2 = false;  //  2:  keinen Beat erkannt --> kritisch, da SPO2 nicht mehr gemessen werden kann
bool    AlarmReason3 = false;  //  3:  SPO2 schwankt stark
long    alarm1starttime = 0;
long    alarm2starttime = 0;

int VIBE_FAILED = 0;
int SPEAK_FAILED = 0;
int SENS_FAILED = 0;

int     powerlevel;

// Self-Check
//int   timeForTon = 30000; // Lautsprechercheck wäre jetzt alle 30 Sekunden
//long  lastTon = 0;
long  timeForVibe = 600000; // Check Vibrationsmotor alle 10 Minuten
long  lastVibe = 0;

// Alarmgrenze
char c;
char d;
int alarmgrenze_addr = 10; //EEPROM kann pro Byte nur Werte von 0-255 speichern!
int alarmgrenze = 90; // Set 90 as standard alarmgrenze
int alarmgrenze_BLE;
int alarmgrenze_EEPROM;

// Touch
int istouched = 0;
int lasttouch = 0;
int touchzaehler = 0;
long timebetweentouch = 0;
int touchdelay = 30000; // Duration of sleep in ms after touch button pressed

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////


void setup(void) {
  delay(500); 
  // pinMode
  pinMode(LIPO_PIN, INPUT); // Battery Check
  pinMode(TOUCH_PIN, INPUT); // Touch

  // Start USB Serial Connection
  Serial.begin(57600);

  // Start Bluetooth Serial Connection (between Atmega32u4 and HM17)
  // Set Name to WakeMe so that the App can detect it
  Serial1.begin(9600);
  Serial.print("Bluetooth konfigurieren");
  delay(300);
  Serial1.print("AT+NAMEWakeMe");
  delay(2000);
  Serial1.print("AT+RESET");
  delay(3000);
  Serial.print("Bluetooth konfigurieren abgeschlossen");

  // Initialize DRV2605
  if (!vibe.begin()) {
      delay(1500);
      if (!vibe.begin()) { //zweite Chance
      delay(1500);
        if (!vibe.begin()) { //dritte Chance
            VIBE_FAILED = 1;
            }
      }
  }

  // Initialize MAX30102
  byte powerLevel = 0x1F; //Options: 0=Off to 255=50mA (Voreinstellung: 0x17)
  byte sampleAverage = 8; //Options: 1, 2, 4, 8, 16, 32 (Voreinstellung: 4)
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR
  int sampleRate = 50; //Optioinins: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  if (!sensor.begin()) {
    SENS_FAILED = 1;
  }
  sensor.setup(powerLevel, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with default settings


  // Hier eigentlich noch Speaker Check
  // // //

  // Print Error indication, when Serial port is active.
  if (Serial) {
    Serial.print(F("Init VIBE: "));
    Serial.println(VIBE_FAILED);
    Serial.print(F("Init SPEAK: "));
    Serial.println(SPEAK_FAILED);
    Serial.print(F("Init SENS: "));
    Serial.println(SENS_FAILED);
  }

  // Tell User all Systems are ready and measurement can start or system is defect
  if (VIBE_FAILED == 0 && SENS_FAILED == 0) {
    wake.system_ready();
    Serial.println("Systemcheck bestanden");
    funktionszustand = 3;
    sendData(); // Send Data to BLE Device
  }
  else {
    // If there is a mistake stop code and do not start measuring
    while (1) {
      wake.system_failure();
      funktionszustand = 0;
      sendData(); // Send Data to BLE Device
    }
  }


  ///////////////////////////////////////////////////
  ////////////////  Alarmgrenze ////////////////////
  ///////////////////////////////////////////////////

  // Recieving data from BLE after start and compare it with the alarmgrenze in EEPROM

  alarmgrenze_EEPROM = EEPROM.read(alarmgrenze_addr); // Read from EEPROM
  Serial.print("Alarmgrenze SETUP EEPROM: ");
  Serial.println(alarmgrenze_EEPROM);

  long now = millis();

  while (millis() < (now + 5000)) {
    getData(); // Read from BLE Device
  }

  if (alarmgrenze_EEPROM == 255) { // Default EEPROM Value: 255
    if (alarmgrenze_BLE != 0) {
      EEPROM.write(alarmgrenze_addr, alarmgrenze_BLE); // for the first time, set default alarmgrenze to EEPROM
      alarmgrenze_EEPROM = alarmgrenze_BLE;
      alarmgrenze = alarmgrenze_BLE;
    }
    else {
      EEPROM.write(alarmgrenze_addr, alarmgrenze); // for the first time, set default alarmgrenze to EEPROM
      Serial.println("SETUP EEPROM = 0, schreibe Alarmgrenze = 90");
    }
  }

  else if (alarmgrenze_EEPROM != 0) { // If there is something in EEPROM
    Serial.print("Alarmgrenze SETUP BLE: ");
    Serial.println(alarmgrenze_BLE);

    if ((alarmgrenze_EEPROM != alarmgrenze_BLE) && (alarmgrenze_BLE != 0)) {
      alarmgrenze = alarmgrenze_BLE; // If Alarmgrenze from BLE Device is new, set this
      alarmgrenze_EEPROM = alarmgrenze_BLE; // If Alarmgrenze from BLE Device is new, set this
      EEPROM.write(alarmgrenze_addr, alarmgrenze);  // Save to EEPROM
      Serial.println("Alarmgrenze EEPROM != Alarmgrenze BLE, schreibe Alarmgrenze = Alarmgrenze_BLE");
    }

    else {
      alarmgrenze = alarmgrenze_EEPROM;
      Serial.print("Alarmgrenze EEPROM != 0 und keine neue Alarmgrenze über BLE, wähle alarmgrenze_EEPROM: ");
      Serial.println(alarmgrenze);
    }
  }
  delay(1000);
  sendData(); // send Alarmgrenze to BLE Device

  // Serial Outputs for Debugging
  Serial.println("Totzeit noch aktiv");
}

////////////////////////////////////////////////////////////////
///////////////////////  L O O P  //////////////////////////////
////////////////////////////////////////////////////////////////


void loop()  {
    long now = millis();   //start time of this cycle

  // Battery check if no serial connection is established and after timeAkkuCheck
  if (!Alarm && millis() > lastAkkuCheck + timeAkkuCheck) {
    powerlevel = analogRead(A2);
    powerlevel = powerlevel * 0.0065 * 100;
    Serial.print("Batterieladung: ");
    Serial.print(powerlevel);
    Serial.println(" V");

    // if Bat < 3.4V alarm starts
    if (powerlevel <= 3.4) {
      Serial.println("Batteriezustand zu gering");
      wake.batterylevel_low();
      funktionszustand = 6;
      sendData();
      delay(1000);
      return;
    }
    lastAkkuCheck = millis();
  }


  // Check Vibration if it is not active and after timeForVib
  if (!Alarm && millis() > timeForVibe + lastVibe && vibe.readRegister8(DRV2605_REG_GO) == 0) {
    vibe.runDiagnose();
    bool goodvibe = vibe.runDiagnose();
    while (!goodvibe) {
      wake.penetrant_alarm();
      Serial.println("Vibrationsmotor Fehler");
      funktionszustand = 0;
      sendData(); // Send Data to BLE Device
    }
    lastVibe = millis();
  }

  // DRV2605 in Standby versetzen, wenn inaktiv
  if (vibe.readRegister8(DRV2605_REG_GO) == 0 && vibe.readRegister8(DRV2605_REG_MODE) != B01000000) {
    vibe.standby();
    Serial.println("VIB in Standby setzen");
  }

  // Totzeit for 30 sec. before anything starts (time to put on device)
  if (millis() < Totzeit) {
    return;
  }

  // This is only called once after Totzeit, Tell user system starts now by playing melody
  if (Start) {
    Serial.println("Totzeit vorbei");
    Start = false;
    // Startton ausgeben
    wake.start_measure();
  }

  // If alarm is active, send alarm funktionzustand to device
  if (Alarm) {
    funktionszustand = 4;
    sendData();
  }

//   Touch auslesen: Do something after touched twice in 1.5 seconds
  istouched = digitalRead(TOUCH_PIN);
  if (istouched && !lasttouch) // to avoid counter rising while finger stays on touch
  {
    touchzaehler ++;
    if (touchzaehler == 1) {
      timebetweentouch = now;
    }
    Serial.print("Touch Button pressed");
    Serial.println(touchzaehler);
    lasttouch = 1;
  }
  if (istouched == 0 && lasttouch == 1) // after finger release reset lasttoouch
  {
    lasttouch = 0;
  }
  if ((touchzaehler == 2) && (now < timebetweentouch + 1000)) { // if it was touched twice in 1500 ms
    Serial.println("TOUCH AUSLÖER - - - - - - - - - - - - - - ");
    Serial.println("Alarmpause für touchdelay Sekunden");
    while (millis() < (now + touchdelay)) {
    vibe.stop();
    noTone(8);
    funktionszustand = 5;
    sendData();
    }
    touchzaehler = 0;
    timebetweentouch = 0;
    return;
  }
  if ((touchzaehler > 0) && (now > timebetweentouch + 1000)) {
    Serial.println("TOUCH AUSLÖER ZURÜCKGESETZT  - - - - - - ");
    touchzaehler = 0;
    timebetweentouch = 0;
  }

  // Start Measure function
  measure(now);
  //Serial.println("START MEASURE FUNKTION");


  ///////////////////////////////////////////////
  //////////////// A L A R M S //////////////////
  ///////////////////////////////////////////////

  // Check if any alarm has to start

  ////////////////// A L A R M 1 /////////////////
  // Alarm 1: Saturation falls for alarm1_saturationdrop beats below Alarmgrenze in between 20 seconds
  if (beatIR && (derSPO2 < 0.2) && (lastderSPO2 < 0)) {   // saturation falls negative for at least 0.1
    negderSPO2coursecounter++;
    Serial.print("Saturation fällt ----------------------------- ");
    Serial.println(negderSPO2coursecounter);

    if (negderSPO2coursecounter == 1) { // Set start time of first bad saturation beat
      alarm1starttime = now;
    }

    if ((negderSPO2coursecounter >= alarm1_saturationdrop) || (now > alarm1starttime + 20000)) { // too much time between low saturation beats --> reset counter
      Serial.println("Saturation alarm1_saturationdrop mal gefallen hintereinander, aber mit zu großen Abstand");
      negderSPO2coursecounter = 0;
      posderSPO2coursecounter = 0;
      Alarm = false;
      AlarmReason1 = false;
    }

    else if ((negderSPO2coursecounter >= alarm1_saturationdrop) && (now <= alarm1starttime + 20000)) { // If there are 4 bad saturation beats within 12 secs --> alarm
      Serial.println("Saturation alarm1_saturationdrop mal gefallen innerhalb 10 secs hintereinander, starte Alarm 1");
      negderSPO2coursecounter = 0;
      posderSPO2coursecounter = 0;
      Alarm = true;
      AlarmReason1 = true;
    }
  }

  if (AlarmReason1) {
    wake.faellt_alarm();
    Serial.println(" Alarm weil SPO2 fällt");
  }

  // Reset Alarm 1: SpO2 steigt 3 mal hintereinander positiv an
  if (AlarmReason1 && derSPO2 >= 0 && lastderSPO2 >= 0) {
    posderSPO2coursecounter++;
    if (posderSPO2coursecounter >= 1) {
      posderSPO2coursecounter = 0;
      negderSPO2coursecounter = 0;
      AlarmReason1 = false;
      alarm1starttime = 0;
      Serial.println("Alarm 1 zurückgesetzt - -- - - - - - - - ");
      noTone(8);
    }
  }


  ////////////////// A L A R M 2 /////////////////
  // Alarm 2: Saturation below Alarmgrenze for 10 pulses or within 20 seconds
  if ((SPO2f > 1) && (beatIR || (now > alarm2starttime + 20000))  && (SPO2f < alarmgrenze) && (lastSPO2f < alarmgrenze)) {
    SPO2counter++;
    Serial.println("SPO2 unter Alarmgrenze");
    if (SPO2counter == 1) { // Set start time of first bad saturation beat
      alarm2starttime = now;
    }
  }

  if (SPO2counter >= alarm2_below_alarmgrenze) {
    Serial.println("Alarm 2 - - - - - - - - - ");
    Alarm = true;
    AlarmReason2 = true;
  }

  if ((SPO2counter >= alarm2_below_alarmgrenze) && (now > alarm2starttime + 20000)) { // Too much time in between --> reset
    Serial.println("Saturation alarm1_saturationdrop mal gefallen hintereinander, aber mit zu großen Abstand");
    SPO2counter = 0;
    Alarm = false;
    AlarmReason2 = false;
  }

  if (AlarmReason2) {
    wake.penetrant_alarm();
    Serial.println(" Alarm weil SPO2 unter Alarmgrenze");
  }

  // Reset Alarm 2: SpO2 dreimal über Alarmgrenze
  if (AlarmReason2 && SPO2f >= alarmgrenze && lastSPO2f >= alarmgrenze) {
    SPO2counter--;
    if (SPO2counter < 1) {
      SPO2counter = 0;
      AlarmReason2 = false;
      alarm2starttime = 0;
      Serial.println("Alarm 2 zurückgesetzt - -- - - - - - - - ");
      noTone(8);
    }
  }

  ////////////////// A L A R M 3 /////////////////
  // Alarm 3: No beat or too low HR
  if ((lastBeat > 0) && !beatIR && (now > lastBeat + (nobeatcounter * 4000))) {
    nobeatcounter++;
    Serial.print("No beat detected: ");
    Serial.println(nobeatcounter);
  }
  if (nobeatcounter > 6) {
    Serial.println("Alarm 3 - - - - - - - - - ");
    Alarm = true;
    AlarmReason3 = true;
  }
  
  if (AlarmReason3) {
    wake.penetrant_alarm();
    Serial.println("Alarm weil kein Beat detektiert");
  }

  // Reset Alarm 3: 3 beats innerhalb 8 sekunden
  if (AlarmReason3 && beatIR) {
    beatcounter++;
    if (nobeatcounter >= 2) {
      nobeatcounter = 0;
      beatcounter = 0;
      AlarmReason3 = false;
      Serial.println("Alarm 3 zurückgesetzt - -- - - - - - - - ");
      noTone(8);

    }
  }
  if ((now > lastBeat + 60000) && (nobeatcounter < 10)) {
    nobeatcounter = 0;
    //Serial.println("Alarm 3 zurückgesetzt weil zu lange nix passiert ist - -- - - - - - - - ");
  }

}


////////////////////////////////////////////////////////////////
///////////////////////  M E A S U R E  ////////////////////////
////////////////////////////////////////////////////////////////

void measure(long now) {

  if (!sensor.safeCheck(250)) { //If there is no signal after 250ms the sensor is broken
    broken_system = true;
    Serial.println("Sensor SafeCheck Problem");
    funktionszustand = 0;
    sendData(); // Send Data to BLE Device
    while (millis() < (now + 5000)) {
      wake.penetrant_alarm();
    }
    return;
  }

  // Read IR and RED Values
  unsigned long irValue = sensor.getIR();
  unsigned long  redValue = sensor.getRed();

  sensor.nextSample();


  // Check if IR and RED counts are above the minimum count
  if ((!Alarm) && (irValue < minCounts || redValue < minCounts)) {
      //Serial.println("IR and RED Counts zu gering");
      ir_red_counter ++;
      
      if (ir_red_counter > IR_RED_COUNTS_FEHLERZEIT) {
          Serial.println("IR_RED_COUNTS_FEHLERZEIT erreicht, Alarm wird ausgelöst");
          funktionszustand = 2;
          sendData(); // Send Data to BLE Device
          ir_red_counter = 0;
          wake.softer_alarm();
          return;
      }
    }


  else {
    if (!Alarm) {
      funktionszustand = 1;
      sendData(); // Send Data to BLE Device
    }

    // Perfusionsindex prüfen
    if ((!Alarm) && (millis() > Totzeit) && beatIR) {
      //Serial.println(perfusionIR);
      if (perfusionIR <= 0.10) {
        Serial.println("Perfusionsindex <0.1 und somit zu gering");
        wake.vibe_alarm();
        funktionszustand = 2;
        sendData();
      }
      if (perfusionIR > 0.15) {
        Serial.println("Perfusions in Ordnung");
      }
    }

    ////////////////////////////////////////////////////////////////
    ///////////////////////  S I G N A L  //////////////////////////
    ////////////////// V E R A R B E I T U N G /////////////////////
    ////////////////////////////////////////////////////////////////

    // remove DC element
    int16_t IR_signal, Red_signal;

    IR_signal =  pulseIR.ma_filter(pulseIR.dc_filter(irValue)) ;
    Red_signal = pulseRed.ma_filter(pulseRed.dc_filter(redValue));
    beatRed = pulseRed.isBeat(Red_signal);
    beatIR =  pulseIR.isBeat(IR_signal);

    // check RED for beat
    if (beatRed) {
      lastRedBeat = now;
    }
    
    // check IR for heartbeat
    if (beatIR) {
      long HRV = now - lastBeat;
      long btpm = 60000 / (HRV);
      beatAvg = (btpm > 0 && btpm < 200) ? bpm.filter((int16_t)btpm) : 0;

      // compute SpO2 ratio
      long numerator   = (pulseRed.avgAC() * pulseIR.avgDC()) / 256;
      long denominator = (pulseRed.avgDC() * pulseIR.avgAC()) / 256;

      int RX100 = (denominator > 0 && numerator > 0) ? (numerator * 100) / denominator : 999;
      //Serial.println(RX100);
      // using formula
      SPO2f = (RX100 >= 33 && RX100 <= 100) ? -45.060 * RX100 / 100 * RX100 / 100 + 30.354 * RX100 / 100 + 94.845 : 0; // Kalibrierung erforderlich ?
      // from table
      //if ((RX100>=0) && (RX100<183)) SPO2 = pgm_read_byte_near(&spo2_table[RX100]);
      SPO2 = SPO2f + 0.5; // rundet auf wenn über 0,5 und ab wenn darunter

      // Perfusionsindex [%] kalkulieren
      perfusionIR = (float)pulseIR.avgAC() / pulseIR.avgDC() * 100;
      perfusionRed = (float)pulseRed.avgAC() / pulseRed.avgDC() * 100;

      // SpO2-Verlauf überwachen
      lastderSPO2 = derSPO2;
      derSPO2 = (SPO2f - lastSPO2f); /* / (now - lastBeat) * 1000; */

      lastSPO2f = SPO2f;
      lastBeat = now;


      // Wenn Serieller Port alles in Monitor schreiben
      if (Serial) {
        Serial.print(F("HR: "));
        Serial.println(beatAvg);
        Serial.print(F("SPO2f: "));
        Serial.println(SPO2f);
        Serial.print(F("Änderung SpO2: "));
        Serial.println(derSPO2);
      }
      // Send Data to BLE Device
      sendData(); // Send Data to BLE Device
    }
  }
}



////////////////////////////////////////////////////////////////
///////////////////////  B L U E T O O T H  ////////////////////
////////////////////////////////////////////////////////////////

// sendData(): Send new data to BLE Device
void sendData() {

  if (Serial1) {

    //--> Zeit
    Serial1.print(millis());
    Serial1.print(" ");

    //--> SpO2
    Serial1.print(SPO2);
    Serial1.print(" ");

    //--> BPM
    Serial1.print(beatAvg);
    Serial1.print(" ");

    // Battery
    Serial1.print(powerlevel);
    Serial1.print(" ");

    // Funktionszustand
    Serial1.print(funktionszustand);
    Serial1.print(" ");

    // ALarmgrenze
    Serial1.print(alarmgrenze);
  }
}

// getData(): Search for new data from BLE Device (for Alarmgrenze)
void getData() {
    c = Serial1.read();
    delay(10);
    if (c < 7) {
      return;
    }
    else {
      d = Serial1.read();
      int response = (String(c) + String(d)).toInt();
      if ((response != 0) && (response > alarmgrenze_BLE)) {
        alarmgrenze_BLE = response;
      }
      Serial.print("getData(): ");
      Serial.println(alarmgrenze_BLE);
    }
}
