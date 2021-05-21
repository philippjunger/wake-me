/***************************************************

Different acoustic and haptic alarm settings for the WakeMe Project
Written by Lukas Hokenmaier 21-06-2020

****************************************************/


#include "Arduino.h"
#include "DRV2605.h"
#include "Speaker.h"

int notenNr = 0;
int noten[] = { 2000, 1500, 2000, 1500, 500, 600 };
int dauer[] = { 100, 100, 100, 500, 100, 500 };
unsigned long aktuell = 0;
unsigned long tonBeginn = 0;
unsigned long tonDauer = 0;
DRV2605 vibe;

class Wecker {
public:
  Wecker(void) {;}

  void faellt_alarm() {
    vibe.power();
    vibe.setWaveform(0, 55);
    vibe.setWaveform(1, 0);
    vibe.go();
    tone(8,1800);
    delay(100);
    noTone(8);
    delay(100);
    
    tone(8,1400);
    delay(100);
    noTone(8);
    delay(100);

    tone(8,1800);
    delay(100);
    noTone(8);
    delay(100);
    
    tone(8,1400);
    delay(100);
    noTone(8);
  }

  void lowbeat() {
    vibe.power();
    vibe.setWaveform(0, 3);
    vibe.setWaveform(1, 0);
    vibe.go();
  }

  void middlebeat() {
    vibe.power();
    vibe.setWaveform(0, 2);
    vibe.setWaveform(1, 0);
    vibe.go();
  }

  void goodbeat() {
    vibe.power();
    vibe.setWaveform(0, 1);
    vibe.setWaveform(1, 0);
    vibe.go();
  }

  void start_measure() {
    tone(8, 800); 
    delay(100); 
    noTone(8); 
    delay(100); 
    
    tone(8, 1200);
    delay(100); 
    noTone(8); 
  }


//  void start_measure() {
//    tone(8, 1000); 
//    delay(100); 
//    noTone(8); 
//    delay(50); 
//    
//    tone(8, 1300);
//    delay(100); 
//    noTone(8); 
//    delay(50); 
//    
//    tone(8, 1600); 
//    delay(100); 
//    noTone(8); 
//    delay(200); 
//    
//    tone(8, 1600);
//    delay(100); 
//    noTone(8); 
//    delay(100); 
//    
//    tone(8, 1900); 
//    delay(100); 
//    noTone(8); 
//  }

  void system_ready() {
    vibe.power();
    vibe.setWaveform(0, 3);
    vibe.setWaveform(1, 0);
    vibe.go();
    tone(8, 800);
    delay(100);
    noTone(8);
    delay(100);
    tone(8, 400);
    delay(100);
    noTone(8);
    
  }

  void batterylevel_low() {
    tone(8, 800); 
    delay(100); 
    noTone(8); 
    delay(50); 

    tone(8, 600); 
    delay(100); 
    noTone(8);
    delay(50); 

    tone(8, 400);
    delay(100); 
    noTone(8); 
    delay(50); 
    }

  void system_failure() {
    penetrant_alarm();
  }

  void system_failure_vibe() {
    for (int i = 0; i < 10; i++) {
      toneAC(3000, 10);
      delay(100);
      toneAC(2000, 10);
      delay(100);
    }
    noToneAC();
  }

  void system_failure_speak() {
    vibe.power();
    vibe.setWaveform(0, 84);
    vibe.setWaveform(1, 16);
    vibe.setWaveform(2, vibe.wait500ms); // Waveform 16 mit Delay zu vorheriger Waveform
    vibe.setWaveform(3, 16);
    vibe.setWaveform(4, 0);
    vibe.go();
  }

  void vibe_alarm() {
    vibe.power();
    vibe.setWaveform(0, 11);
    vibe.setWaveform(1, vibe.wait50ms);
    vibe.setWaveform(2, 11); // Waveform 16 mit Delay zu vorheriger Waveform
    vibe.setWaveform(3, 0);
    vibe.go();
  }

  void softer_alarm() {
      vibe.power();
    vibe.setWaveform(0, 11);
    vibe.setWaveform(1, vibe.wait50ms);
    vibe.setWaveform(2, 11); // Waveform 16 mit Delay zu vorheriger Waveform
    vibe.setWaveform(3, 0);
    vibe.go();
      tone(8,202);
      delay(100);
      noTone(8);
//      delay(100);
//      tone(8,202);
//      delay(100);
//    noTone(8);
  }
  
  void soft_alarm() {
    uint16_t v = 600;
    while (v < 900) {
      tone(8,v);
      delay(100);
      tone(8,v);
      delay(100);
      v += 50;
    }
    noTone(8);
  }
    

  void middle_alarm() {
    toneAC(500, 6);
    delay(1000);
    toneAC(600, 6);
    delay(1000);
    noToneAC();
  }
  
  void hard_alarm() {
    f1 = random(500, 3500);
    f2 = random(500, 3500);
    f3 = random(500, 3500);
    f4 = random(500, 3500);
    
    tone(8, f1);
    delay(100);
    tone(8, f2);
    delay(200);
    tone(8, f3);
    delay(100);
    tone(8, f4);
    delay(100);
    noTone(8);
  }



  void penetrant_alarm() {    
    aktuell = millis();
      
    vibe.power();
    vibe.setWaveform(0, 84);
    vibe.setWaveform(1, 3);
    vibe.setWaveform(2, vibe.wait500ms); // Waveform 16 mit Delay zu vorheriger Waveform
    vibe.setWaveform(3, 84);
    vibe.setWaveform(4, 3);
    vibe.setWaveform(5, vibe.wait500ms);
    vibe.setWaveform(6, 16);
    vibe.setWaveform(7, 0);
    vibe.go();

  if (aktuell - tonBeginn > tonDauer )
  {
     tone(8, noten[notenNr]);
     tonDauer = dauer[notenNr];
     notenNr++;
     if( notenNr == 5 )
       notenNr = 0;
     tonBeginn = aktuell;
  }
  }

private:
  int16_t alarmPin, vibePin;
  int16_t f1, f2, f3, f4;
};

class WakeUser { 
public:
  WakeUser(void);

  /*
   * void WakeUser();
   * Funktion um Alarme zu switchen, bei nicht Beachtung
   */
  
private:
};
