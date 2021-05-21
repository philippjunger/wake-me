/***************************************************

Back EMF Detection for Speaker to check, whether the Speaker works or not
Written by Lukas Hokenmaier 21-06-2020

****************************************************/


#include "Arduino.h"
#include "toneAC.h"


// Speaker must be connected to PIN 9 and 10
#define SPEAKER1 9
#define SPEAKER2 10
#define OPPIN 11
#define READER A0

#define BEMF_ARRAY 10
#define minSignalStrengthTone 25 //min amplitude height, that the Back EMF signal must reach
#define maxSignalStrengthAnalog 25 //amplitude height the analogPin should see without BEMF signal should be under 25


class SPEAKER {
public:

  SPEAKER(void);

  boolean check_Speaker();

  void toncheck();
  void analogReadCheck();

  uint8_t ton_auswertung();
  uint8_t analog_auswertung();
  uint16_t amplituden_check(int16_t signal);

private:
  // Initialize needed Variables for Check
  int BEMF_signal[BEMF_ARRAY];
  int amplitudenArray[BEMF_ARRAY];

  int prev_sig;
  int cycle_max;
  int cycle_min;

  boolean positive;
  
  
};
