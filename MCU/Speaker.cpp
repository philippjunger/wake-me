/***************************************************

Back EMF Detection for Speaker to check, whether the Speaker works or not
Written by Lukas Hokenmaier 21-06-2020

****************************************************/


#include "Speaker.h"

SPEAKER::SPEAKER() {
  // Constructor
  // Initialize needed Variables for Check
  for (int i = 0; i < BEMF_ARRAY; i++) {
    BEMF_signal[i] = 0;
    amplitudenArray[i] = 0;
  }
  prev_sig = 0;
  cycle_max = 0;
  cycle_min = 0;

  positive = false;
}

boolean SPEAKER::check_Speaker() {

  bool speakerStatus = false;
  
  // get speaker Back EMF (BEMF) signal
  toncheck();

  // assign first signal to Variables for amplitudencheck
  prev_sig = BEMF_signal[0];
  cycle_max = BEMF_signal[0];
  cycle_min = BEMF_signal[0];

  // check the amplitudes of the BEMF signal
  for (int i = 0; i < BEMF_ARRAY; i++) {
    amplitudenArray[i] = amplituden_check(BEMF_signal[i]);
  }

  // save the number of amplitudes with an appropriate height 
  int ton = ton_auswertung();

  delay(10); // Back-EMF Signal vollends ausklingen lassen

  // check the analog signal without BEMF signal
  analogReadCheck();

  prev_sig = BEMF_signal[0];
  cycle_max = BEMF_signal[0];
  cycle_min = BEMF_signal[0];

  // check the amplitudes of the signal (indicator for shorted Ground)
  for (int i = 0; i < BEMF_ARRAY; i++) {
    amplitudenArray[i] = amplituden_check(BEMF_signal[i]);
  }

  // save the number of amplitudes with an appropriate height
  int analog = analog_auswertung();

  // set SPEAKER Pins to INPUT to save energy
  pinMode(SPEAKER1, INPUT);
  pinMode(SPEAKER2, INPUT);
  
  if (Serial) {

    if (ton > 1 && analog == 0) {
      Serial.println(F("Funktioniert!!!"));
    }

    if (analog >= 1) {
      Serial.println(F("Reader defekt!!!"));
    }

    if (ton == 0) {
      Serial.println(F("Tongeber defekt!!!"));
    }
    
  }

  if (ton > 1 && analog == 0) {
    speakerStatus = true;
  }
  return speakerStatus;  
}

uint8_t SPEAKER::ton_auswertung() {
  int count = 0;
  for (int i = 0; i < BEMF_ARRAY; i++) {
      if (amplitudenArray[i] > minSignalStrengthTone) {
        count++;
      }
  }
  return count;
}

uint8_t SPEAKER::analog_auswertung() {
  int count = 0;
  for (int i = 0; i < BEMF_ARRAY; i++) {
      if (amplitudenArray[i] > maxSignalStrengthAnalog) {
        count++;
      }
  }
  return count;
}

void SPEAKER::analogReadCheck() {
  
  pinMode(SPEAKER1, INPUT);
  pinMode(SPEAKER2, OUTPUT);
  digitalWrite(SPEAKER2, LOW);
  
  pinMode(OPPIN, OUTPUT);
  digitalWrite(OPPIN, HIGH);

  for (int i = 0; i < BEMF_ARRAY; i++) {
    BEMF_signal[i] = analogRead(READER);
  }
  
  digitalWrite(OPPIN, LOW);
  pinMode(OPPIN, INPUT);
}

void SPEAKER::toncheck() {
  
  toneAC(978, 1);

  delay(1);

  toneAC();
  
  pinMode(SPEAKER1, INPUT);
  pinMode(SPEAKER2, OUTPUT);
  digitalWrite(SPEAKER2, LOW);
  
  pinMode(OPPIN, OUTPUT);
  digitalWrite(OPPIN, HIGH);
  
  for (int i = 0; i < BEMF_ARRAY; i++) {
    BEMF_signal[i] = analogRead(READER);
  }

  digitalWrite(OPPIN, LOW);
  pinMode(OPPIN, INPUT);

}

uint16_t SPEAKER::amplituden_check(int16_t signal) {
  //  positive to negative i.e peak so declare beat
  if (positive && (signal < prev_sig)) {
    cycle_max = prev_sig;
    positive = false;
    int amplitude = cycle_max - cycle_min;
    if (amplitude > 0 && amplitude < 1000) {
      prev_sig = signal; // save signal
      return amplitude;
    }
  } 
  //negative to positive i.e valley bottom 
  if (!positive && (signal > prev_sig)) {
     cycle_min = prev_sig; positive = true;
  } 
  prev_sig = signal; // save signal
  return 0;
}
