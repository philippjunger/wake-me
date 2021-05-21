/*
* Filter output from MAX30102 and detect heartbeat
*
* based on j.n.magee 15-10-2019
* 
* changed by Lukas Hokenmaier
*/
#include "Pulse.h"

Pulse::Pulse(){
    cycle_max = 20;
    cycle_min = -20;
    positive = false;
    prev_sig = 0;
    amplitude_avg_total = 0;
}


//  Returns true if a beat is detected
bool Pulse::isBeat(int16_t signal) {
  bool beat = false;
  //  positive to negative i.e peak so declare beat
  if (positive && (signal < prev_sig)) {
    cycle_max = prev_sig;
    amplitude = cycle_max - cycle_min;
    
    //Serial.println(amplitude);
    
    if (amplitude > 50 && amplitude < 3000) {
      beat = true;
      // exponential smoothing
      amplitude_avg_total += ((amplitude - amplitude_avg_total)/4); 
    }
    positive = false;
  } 
  //negative to positive i.e valley bottom 
  if (!positive && (signal > prev_sig)) {
     cycle_min = prev_sig; positive = true;
  }
  prev_sig = signal; // save signal
  return beat;
}
