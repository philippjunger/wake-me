/*
* Filter output from MAX30102 and detect heartbeat
*
* based on j.n.magee 15-10-2019
* 
* changed by Lukas Hokenmaier 03-09-2020
*/
#include "Arduino.h"

#define NSAMPLE 24  //Exponential smoothing DC removal filter alpha = 1/NSAMPLE
#define NSLOT   3   //Moving Average Filter over NSLOT values.  
#define BpmSLOT 5   //Moving Average Filter over NSLOT values.  

class MAFilter {
public:
    MAFilter() {nextslot = 0;}
    
    int16_t filter(int16_t value) {
        buffer[nextslot] = value;
        nextslot = (nextslot+1) % NSLOT;
        int16_t total = 0;
        for(int i=0; i<NSLOT; ++i) total += buffer[i];
        return total/NSLOT;
    }
       
private:   
    int16_t buffer[NSLOT];
    uint8_t nextslot; 
};

class BpmMAFilter {
public:
    BpmMAFilter() {nextslot = 0;}
    
    int16_t filter(int16_t value) {
        buffer[nextslot] = value;
        nextslot = (nextslot+1) % BpmSLOT;
        int16_t total = 0;
        for(int i=0; i<BpmSLOT; ++i) total += buffer[i];
        return total/BpmSLOT;
    }
       
private:   
    int16_t buffer[BpmSLOT];
    uint8_t nextslot; 
};

class DCFilter {
public:
    DCFilter(void) {sample_avg_total = 0;}
       
    //remove dc from sample
    int16_t  filter(int32_t sample) {
        sample_avg_total += ((sample - sample_avg_total)/NSAMPLE);
        return (int16_t)(sample - sample_avg_total);
    }
    // return average dc
    int32_t avgDC() {return sample_avg_total;}
       
private:   
    int32_t sample_avg_total;
};

class Pulse {
public:
    Pulse(void);
    //remove DC 
    int16_t dc_filter(int32_t sample) { return dc.filter(sample);}
    //low pass moving average filter
    int16_t ma_filter(int16_t sample) { return ma.filter(sample);}
    //return true when beat detected
    bool isBeat(int16_t signal);  
    //return average DC
    int32_t avgDC() {return dc.avgDC();}
    //return average AC
    int16_t avgAC() {return amplitude_avg_total;}
    //return Maxima
    int32_t maxima() {return cycle_max;} 
    //return Minima
    int32_t minima() {return cycle_min;}
    //return amplitude
    int16_t retAmplitude() {return amplitude;}
       
private:
    DCFilter dc;
    MAFilter ma;
    int16_t amplitude_avg_total;
    int16_t amplitude;
    int16_t cycle_max;
    int16_t cycle_min;
    bool positive;
    int16_t prev_sig;
};
