#include <functional>
#include <driver/touch_pad.h>
#include "soc/rtc.h"
//#include <Ticker.h>
#include "server.h"
#include "constants.h"
#include "GenericClient.h"
#include "curves.h"

// Convention: constants or variables that are only updated slowly will be in all caps

// raw data indexed by ESP32 touch pad number (different from channel A,B,C => 0,1,2)
static uint16_t touch_filtered[TOUCH_PAD_MAX];
static constexpr int filter_period = 4;
static bool s_pad_enabled[TOUCH_PAD_MAX];

// raw touch channels and filtering
static const int CHANNEL_COUNT = 2;
static const int FIR1_LENGTH = 10;
static const float FIR1[] = { 0.024783847522077997,
 0.070755633021442124,
 0.121078146613613533,
 0.161972253261530919,
 0.180445530820012856,
 0.171494409240993462,
 0.136295339829885537,
 0.089243064839404515,
 0.037397135214410099,
 0.006534639636628975 };

// touch measurements, calibration levels, calibrated measurements, calibrated averaged measurements, buffer for rolling average
long measurement[CHANNEL_COUNT];
long calibration[CHANNEL_COUNT];
long calibrated[CHANNEL_COUNT];
float calibrated_average[CHANNEL_COUNT];
long raw[CHANNEL_COUNT];
float raw_average[CHANNEL_COUNT];
//long calibrated_average_buffer[CHANNEL_COUNT][FIR1_LENGTH];
long raw_average_buffer[CHANNEL_COUNT][FIR1_LENGTH];

//additional calibrations
long far_calibration[CHANNEL_COUNT];
double far_touch_sum = 50.0;

// Change touch electrode numbering here
#define CHANNEL_A 0
#define CHANNEL_B 2
#define CHANNEL_A_T TOUCH_PAD_NUM0
#define CHANNEL_B_T TOUCH_PAD_NUM2
//#define CHANNEL_C 3

unsigned int iter = 0; // main loop iteration counter

float predictionTime = 0.04; // latency compensation, by default predicts 40ms in the future 

static const int PRED_AVERAGE_N = 7; // how many previous velocities to average for acceleration based linear extrapolation (prediction)

// touch position on shaft
float touch_position = 0.0;
float raw_touch_position = 0.0;

float last_touch_position = 0.0;
float touch_pos_velocity = 0;
float touch_pos_velocity_average_buffer[PRED_AVERAGE_N];
float touch_pos_velocity_average;
float last_touch_pos_velocity_average;

float last_touch_pos_velocity = 0;
float touch_pos_acceleration = 0;
float touch_pos_velocity_future = 0;

// frequency calculation using zero crossing detection
bool touch_position_isPositive = false;
float threshold = 0.03; // 3 percent zero-cross threshold
int64_t lastZeroCrossingTime = 0; // microseconds
int64_t zeroCrossingTime = 0;

// touch total distance from electrodes
float touch_sum = 0;
float raw_touch_sum = 0;
float last_touch_sum = 0;
float last_raw_touch_sum = 0;
float touch_sum_velocity = 0;
float touch_sum_velocity_average_buffer[PRED_AVERAGE_N];
float touch_sum_velocity_average;
float last_touch_sum_velocity_average;

float last_touch_sum_velocity = 0;
float touch_sum_acceleration = 0;
float touch_sum_velocity_future = 0;

//touch loop iterations
int touch_iter = 0;
//touch buffer iterations
int touch_buffer_index = 0;

int64_t lastTouchIterStart = 0; // microseconds
int64_t touchScanPeriod = -1;
float touchScanPeriodSeconds = -1.0;

// debug print loop timing
int64_t last_loop_time = 0;
static const int64_t LOOP_TIME_US = 40000; //print every 40ms

// calibration flags
bool need_init = true;
bool should_calibrate = true;
bool is_calibrating = false;
int calib_samples = 0;

// far calibration flags
bool should_far_calibrate = false;
bool is_far_calibrating = false;
int far_calib_samples = 0;

// BLE Server stuff
SettingsServer server;

// BLE Client stuff
GenericClient client;

const boolean DEBUG_PRINT = true;
bool debugPrintEnabled = false;

// debug print loop timing
int64_t last_check_time = 0;
static const int64_t CHECK_INTERVAL_US = 200000; // check server updates every 50ms
int SEND_VIBE_EVERY = 8;
bool should_send_vibe = false;
int64_t last_near_time = 0;
static const int64_t NEAR_TIMEOUT = 200000;

int64_t touch_after_cb_index = 0;

bool vibrator_on = false; // off by default

float lastSentVibration = 0.0;

byte strokingCurve = 6; // 3 or 1 is the best currently
float strokingCurve_threshold = 0.1;

float maxVeloScale = 3.0;

float current_prediction_crossover = 1000.0; // crossover point (in current velocity) where we should switch to using predicted velocity

byte sumCurve = 6;
float maxSumScale = 10.0;
float sumCurve_threshold = 1.00;
float sumVelocity_rejection_threshold = -3000.0;

float tip_position_threshold = 0.50;
//float tip_velocity_threshold = 0.07;

float temp_vibe_output = 0.0;

float minimum_output_level = 0.05;

// automatic position range calibration
float max_position = 0.3;
float min_position = -0.3;
double MINMAX_ALPHA = 0.7; // exponential moving average alpha
float temp_max_position = 0.0;
float temp_min_position = 0.0;
static const int64_t MINMAX_UPDATE_LENGTH_US = 6000000; // 6 seconds
int64_t minmax_update_start = 0;
int zero_crossings_since_minmax_start = 0;

static const float MIN_TOUCH_SUM_FOR_ZEROCROSSING = 30.0;

float NEARFAR_ALPHA = 0.7;
float NEAR_PROPORTION_ABOVE_NEUTRAL_LEVEL = 0.5;
float NEAR_FAR_UPDATE_LENGTH_US = 3000000; // 5 seconds
float TOUCH_SUM_VELOCITY_TRIGGER_CALIBRATION = 1000.0;
static const float CALIBRATION_ALLOW_THRESHOLD = 0.2;
int64_t lastNearerThresholdHit = -1;
int64_t lastFartherThresholdHit = -1;
float tmpNeutralLevel[CHANNEL_COUNT];
float tmpNearLevel[CHANNEL_COUNT];
float neutralLevel[CHANNEL_COUNT];
float nearLevel[CHANNEL_COUNT];
float auto_far_touch_sum = 0.0;
float last_abs_sumvelocity = 0.0;
float average_calib_abs_sumvelocity = 0.0;
int calib_average_n = 0;

static char outstr[15];

static const float SECONDARY_MOL_THRESHOLD_HERTZ = 2.0; // 2 hertz
static const float SECONDARY_MOL_MAX_HERTZ = 5.0;
static const float SECONDARY_MOL_LOWERLEVEL = 0.2;
static const float SECONDARY_MOL_HIGHERLEVEL = 0.5;

float raw_pos_zero_level = 0.0;
float ZERO_LEVEL_ALPHA = 0.0005;

static void handleZeroCrossing(bool positive) {
  int64_t t = esp_timer_get_time();
  lastZeroCrossingTime = zeroCrossingTime;
  zeroCrossingTime = t;
  zero_crossings_since_minmax_start++;
}

static inline int mod_floor(int a, int n) {
    return ((a % n) + n) % n;
}

static inline void updateAutoFarTouchSum() {
  auto_far_touch_sum = max(0.0f, (neutralLevel[0] - nearLevel[0]) + (neutralLevel[1] - nearLevel[1]))*NEAR_PROPORTION_ABOVE_NEUTRAL_LEVEL; // min 0
}

static void nearfar_update(int64_t currTime) {
  float absv = fabs(touch_sum_velocity_average);
  
  if (touch_sum_velocity_average > TOUCH_SUM_VELOCITY_TRIGGER_CALIBRATION && lastNearerThresholdHit < 0 && absv > last_abs_sumvelocity) //&& (lastFartherThresholdHit < 0 || currTime < lastFartherThresholdHit+1000000))  // last condition: if farther calib not ongoing or began less than 1 second ago
  {
    lastNearerThresholdHit = currTime;
    for (int i=0; i<CHANNEL_COUNT; i++) {
      tmpNearLevel[i] = INFINITY;
    }
    lastFartherThresholdHit = -1; // abort neutral calibration calibration
    last_abs_sumvelocity = absv;
    average_calib_abs_sumvelocity = 0.0;
    calib_average_n = 0;
  }
  else if (touch_sum_velocity_average < -TOUCH_SUM_VELOCITY_TRIGGER_CALIBRATION && lastFartherThresholdHit < 0 && absv > last_abs_sumvelocity) //&& (lastNearerThresholdHit < 0 || currTime < lastNearerThresholdHit+1000000)) // only allow when not near calibrating
  {
    lastFartherThresholdHit = currTime;
    for (int i=0; i<CHANNEL_COUNT; i++) {
      tmpNeutralLevel[i] = 0.0;
    }
    lastNearerThresholdHit = -1; // abort other calibration
    last_abs_sumvelocity = absv;
    average_calib_abs_sumvelocity = 0.0;
    calib_average_n = 0;
  }
  
  if (lastNearerThresholdHit > 0) {
    if (currTime - lastNearerThresholdHit < NEAR_FAR_UPDATE_LENGTH_US) {
      if (touch_sum_velocity_average > 0 && absv > last_abs_sumvelocity) // track peak velocity in this direction
        last_abs_sumvelocity = absv;

      if (currTime - lastNearerThresholdHit > NEAR_FAR_UPDATE_LENGTH_US*0.2) {
        average_calib_abs_sumvelocity += absv;
        calib_average_n += 1;
      }
      
      for (int i=0; i<CHANNEL_COUNT; i++) {
        if (raw_average[i] < tmpNearLevel[i]) {
          tmpNearLevel[i] = raw_average[i];
        }
      }
    }
    else {
      lastNearerThresholdHit = -1;
      float avg = average_calib_abs_sumvelocity / calib_average_n;
      if (avg < TOUCH_SUM_VELOCITY_TRIGGER_CALIBRATION*CALIBRATION_ALLOW_THRESHOLD) {
        for (int i=0; i<CHANNEL_COUNT; i++) {
          nearLevel[i] = NEARFAR_ALPHA*tmpNearLevel[i] + (1.0-NEARFAR_ALPHA)*nearLevel[i]; // update with EMA
        }
  
        updateAutoFarTouchSum();
        Serial.print("Update allowed, avg=");
        Serial.println(avg, 2);
      }
      else {
        Serial.print("Update NOT allowed, avg=");
        Serial.println(avg, 2);
      }
      last_abs_sumvelocity = 0.0;
    }
  }
  
  if (lastFartherThresholdHit > 0) {    
    if (currTime - lastFartherThresholdHit < NEAR_FAR_UPDATE_LENGTH_US) {
      if (touch_sum_velocity_average < 0 && absv > last_abs_sumvelocity) // track peak velocity in this direction
        last_abs_sumvelocity = absv;

      if (currTime - lastFartherThresholdHit > NEAR_FAR_UPDATE_LENGTH_US*0.2) {
        average_calib_abs_sumvelocity += absv;
        calib_average_n += 1;
      }
      
      for (int i=0; i<CHANNEL_COUNT; i++) {
        if (raw_average[i] > tmpNeutralLevel[i]) {
          tmpNeutralLevel[i] = raw_average[i];
        }
      }
    }
    else {
      lastFartherThresholdHit = -1;
      float avg = average_calib_abs_sumvelocity / calib_average_n;
      if (avg < TOUCH_SUM_VELOCITY_TRIGGER_CALIBRATION*CALIBRATION_ALLOW_THRESHOLD) {
        for (int i=0; i<CHANNEL_COUNT; i++) {
          neutralLevel[i] = NEARFAR_ALPHA*tmpNeutralLevel[i] + (1.0-NEARFAR_ALPHA)*neutralLevel[i]; // update with EMA
        }
  
        updateAutoFarTouchSum();
        Serial.print("Update allowed, avg=");
        Serial.println(avg, 2);
      }
      else {
        Serial.print("Update NOT allowed, avg=");
        Serial.println(avg, 2);
      }
      last_abs_sumvelocity = 0.0;
    }
  }
}

static void minmax_update(int64_t currTime) {
  if (currTime - minmax_update_start > MINMAX_UPDATE_LENGTH_US) {
    minmax_update_start = currTime;
    
    if (zero_crossings_since_minmax_start > 3 && fabs(temp_max_position-temp_min_position) > 0.05) { // update with EMA
      max_position = MINMAX_ALPHA*temp_max_position + (1.0-MINMAX_ALPHA)*max_position;
      min_position = MINMAX_ALPHA*temp_min_position + (1.0-MINMAX_ALPHA)*min_position;
    }

    // reset minmax estimator
    temp_max_position = 0.0;
    temp_min_position = 0.0;
    zero_crossings_since_minmax_start = 0;
  }
  else {
    if (raw_touch_position > temp_max_position) {
      temp_max_position = raw_touch_position;
    }
    if (raw_touch_position < temp_min_position) {
      temp_min_position = raw_touch_position;
    }
  }
}

static float scale_minmax(float pos) {
  return constrain(mapf(pos, min_position, max_position, -1.0, 1.0), -1.0, 1.0);
}

static void touch_loop() {
  if (touch_iter == 999) {
    int64_t currTime = esp_timer_get_time();
    touchScanPeriod = (currTime - lastTouchIterStart)/1000;
    touchScanPeriodSeconds = touchScanPeriod / 1000000.0;
    lastTouchIterStart = currTime;
    touch_iter = 0;
  }
  measurement[0] = (int)touch_filtered[CHANNEL_A];
  measurement[1] = (int)touch_filtered[CHANNEL_B];
  #ifdef CHANNEL_C
  if (CHANNEL_COUNT > 2)
    measurement[2] = (int)touch_filtered[CHANNEL_C];
  #endif

  for (int i=0; i<CHANNEL_COUNT; i++) {
    //calibrated[i] = (int)calibration[i]-measurement[i];
    calibrated[i] = (long)(neutralLevel[i]+0.5)-measurement[i];
    raw[i] = measurement[i];
    /*if (calibrated[i] < 0) { // allow negative levels
      calibrated[i] = 0;
    }*/
    int firStartIndex = touch_buffer_index % FIR1_LENGTH;
    raw_average_buffer[i][firStartIndex] = raw[i]; // not calibrated anymore
    float csum = 0;
    for (int j=0; j<FIR1_LENGTH; j++) {
      csum += raw_average_buffer[i][mod_floor(firStartIndex - j, FIR1_LENGTH)] * FIR1[j];
    }
    raw_average[i] = csum;
    calibrated_average[i] = neutralLevel[i] - raw_average[i];
    //calibrated_average[i] = (float)csum / FIR1_LENGTH;
  }
  last_touch_sum = touch_sum;
  touch_sum = calibrated_average[0]+calibrated_average[1];

  last_raw_touch_sum = raw_touch_sum;
  raw_touch_sum = raw_average[0]+raw_average[1];
  
  last_touch_position = touch_position;
  if (calibrated_average[0] > 0.0 && calibrated_average[1] > 0.0) {
    raw_touch_position = (float)((double)(calibrated_average[0] - calibrated_average[1])/(double)touch_sum);
    touch_position = scale_minmax(raw_touch_position);
  }

  if (need_init) {
    touch_position_isPositive = (raw_touch_position > 0);
    need_init = false;
  }

  if (touch_sum > 0.8*auto_far_touch_sum) { // do not accept zero crossing if hand is too far away
    raw_pos_zero_level = ZERO_LEVEL_ALPHA*raw_touch_position + (1.0-ZERO_LEVEL_ALPHA)*raw_pos_zero_level;
    if (touch_position_isPositive && raw_touch_position < raw_pos_zero_level-threshold) {
      touch_position_isPositive = false;
      handleZeroCrossing(false);
    }
    else if (!touch_position_isPositive && raw_touch_position > raw_pos_zero_level+threshold) {
      touch_position_isPositive = true;
      handleZeroCrossing(true);
    }
  }

  if (touchScanPeriodSeconds > 0) { // if period is determined
      last_touch_sum_velocity = touch_sum_velocity;
      last_touch_sum_velocity_average  = touch_sum_velocity_average;
      
      touch_sum_velocity = -(raw_touch_sum - last_raw_touch_sum)/touchScanPeriodSeconds; // reversed sign for velocity based on raw averaged values

      // LATENCY COMPENSATION START
      touch_sum_velocity_average_buffer[touch_buffer_index % PRED_AVERAGE_N] = touch_sum_velocity;
      float csum = 0;
      for (int j=0; j<PRED_AVERAGE_N; j++) {
        csum += touch_sum_velocity_average_buffer[j];
      }
      touch_sum_velocity_average = csum / PRED_AVERAGE_N;
      
      touch_sum_acceleration = (touch_sum_velocity_average - last_touch_sum_velocity_average)/touchScanPeriodSeconds;
      
      touch_sum_velocity_future = touch_sum_velocity + touch_sum_acceleration*predictionTime; // per frame + per second converted to per frame
      // LATENCY COMPENSATION END
      
      
      last_touch_pos_velocity = touch_pos_velocity;
      last_touch_pos_velocity_average = touch_pos_velocity_average;
      touch_pos_velocity = (touch_position - last_touch_position)/touchScanPeriodSeconds;

      // LATENCY COMPENSATION START
      touch_pos_velocity_average_buffer[touch_buffer_index % PRED_AVERAGE_N] = touch_pos_velocity;
      csum = 0;
      for (int j=0; j<PRED_AVERAGE_N; j++) {
        csum += touch_pos_velocity_average_buffer[j];
      }
      touch_pos_velocity_average = csum / PRED_AVERAGE_N;
      
      touch_pos_acceleration = (touch_pos_velocity_average - last_touch_pos_velocity_average)/touchScanPeriodSeconds;

      touch_pos_velocity_future = touch_pos_velocity + touch_pos_acceleration*predictionTime;
      // LATENCY COMPENSATION END

      // SANITY CHECK PREDICTED VELOCITIES (if predicted is reverse direction, stop over-prediction)
      // changed, allow reverse predictions!
      /*if (touch_sum_velocity*touch_sum_velocity_future < 0.0) {
        touch_sum_velocity_future = 0.0;
      }
      if (touch_pos_velocity*touch_pos_velocity_future < 0.0) {
        touch_pos_velocity_future = 0.0;
      }*/
  }

  touch_iter++;
  touch_buffer_index++;
}

static void after_touch_cb() {
  int64_t currTime = esp_timer_get_time();
  // put your main code here, to run repeatedly:

  minmax_update(currTime);
  nearfar_update(currTime);
  
  if (currTime - last_loop_time >= LOOP_TIME_US) {
    last_loop_time = currTime;
  }

  // neutral calibration
  if (should_calibrate) {
    is_calibrating = true;
    calib_samples = 0;
    should_calibrate = false;
  }
  if (is_calibrating) {
    if (calib_samples < 300) {
      calibration[0] += measurement[0];
      calibration[1] += measurement[1];
      calib_samples++;
    }
    else {
      calibration[0] /= 300;
      calibration[1] /= 300;

      neutralLevel[0] = (float)calibration[0];
      neutralLevel[1] = (float)calibration[1];
      
      far_calibration[0] = calibration[0]-50;
      far_calibration[1] = calibration[1]-50;
      far_touch_sum = 200.0; // reset far calibration as well
      auto_far_touch_sum = 200.0;
      is_calibrating = false;
      Serial.println("Calibrated");
    }
  }

  // far calibration
  if (should_far_calibrate) {
    is_far_calibrating = true;
    far_calib_samples = 0;
    far_touch_sum = 0.0;
    should_far_calibrate = false;
  }
  if (is_far_calibrating) {
    if (far_calib_samples < 300) {
      far_calibration[0] += measurement[0];
      far_calibration[1] += measurement[1];
      far_touch_sum += touch_sum;
      far_calib_samples++;
    }
    else {
      far_calibration[0] /= 300;
      far_calibration[1] /= 300;
      far_touch_sum /= 300.0;
      auto_far_touch_sum = far_touch_sum;
      is_far_calibrating = false;
      Serial.println("Far calibrated");
    }
  }


  

  if (touch_after_cb_index % SEND_VIBE_EVERY == 0) {
    should_send_vibe = true;
  }

  touch_after_cb_index++;
}

static void filter_read_cb(uint16_t *raw_value, uint16_t *filtered_value) {
  for (int i=0; i<TOUCH_PAD_MAX; ++i) {
    touch_filtered[i] = raw_value[i];
  }

  touch_loop();
  after_touch_cb();
}

void touch_setup() {
  // Initialize touch pad peripheral, it will start a timer to run a filter
  Serial.println("Initializing touch sensors");
  touch_pad_init();
  // If use interrupt trigger mode, should set touch sensor FSM mode at 'TOUCH_FSM_MODE_TIMER'.
  touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
  // Set reference voltage for charging/discharging
  // For most usage scenarios, we recommend using the following combination:
  // the high reference valtage will be 2.7V - 1V = 1.7V, The low reference voltage will be 0.5V.
  touch_pad_set_voltage(TOUCH_HVOLT_2V5, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V); // compromise between sensitivity and noise rejection
  //touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_0V); // widest range has the best noise rejection
  //touch_pad_set_voltage(TOUCH_HVOLT_2V4, TOUCH_LVOLT_0V8, TOUCH_HVOLT_ATTEN_1V5);
  
  touch_pad_set_cnt_mode(CHANNEL_A_T, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_HIGH); // high slope and especially high drive voltage seems to have the best noise rejection
  touch_pad_set_cnt_mode(CHANNEL_B_T, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_HIGH);
  #ifdef CHANNEL_C
  if (CHANNEL_COUNT > 2)
    touch_pad_set_cnt_mode(CHANNEL_C_T, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_HIGH);
  #endif
  
  //touch_pad_set_voltage(TOUCH_HVOLT_2V4, TOUCH_LVOLT_0V8, TOUCH_HVOLT_ATTEN_1V5);
  //init RTC IO and mode for touch pad.
}

void touch_begin() {
  s_pad_enabled[CHANNEL_A] = true;
  s_pad_enabled[CHANNEL_B] = true;
  #ifdef CHANNEL_C
  s_pad_enabled[CHANNEL_C] = true;
  #endif

  for (int i=0; i<TOUCH_PAD_MAX; ++i) {
    if (s_pad_enabled[i]) {
      touch_pad_config(static_cast<touch_pad_t>(i), 0); // threshold_inactive=0
    }
  }

  uint16_t sleep_cycle;
  uint16_t meas_cycle;
  touch_pad_get_meas_time(&sleep_cycle, &meas_cycle);
  int rtc = rtc_clk_slow_freq_get_hz();
  //touch_pad_set_meas_time(sleep_cycle, 0xffff);
  Serial.print("TIMINGS sleep_cycle=");
  Serial.print((int)sleep_cycle);
  Serial.print("   meas_cycle=");
  Serial.print(meas_cycle);
  Serial.print("   SLOW_RTC=");
  Serial.println(rtc);
  touch_pad_set_meas_time(150, 34000);
  touch_pad_get_meas_time(&sleep_cycle, &meas_cycle);
  //int rtc = rtc_clk_slow_freq_get_hz();
  //touch_pad_set_meas_time(sleep_cycle, 0xffff);
  Serial.print("TIMINGS sleep_cycle=");
  Serial.print((int)sleep_cycle);
  Serial.print("   meas_cycle=");
  Serial.print(meas_cycle);
  Serial.print("   SLOW_RTC=");
  Serial.println(rtc);

  // Initialize and start a software filter to detect slight change of capacitance.
  touch_pad_filter_start(filter_period);
  touch_pad_set_filter_read_cb(filter_read_cb);
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(500000);
  print_wakeup_reason();
  touch_setup();
  touch_begin();
  
  Serial.println("BLEDevice::init");
  BLEDevice::init("Nokia 3.1");
  Serial.println("Scanning for devices...");

  client.startClient(); // blocks until scanning and connecting is finished
  delay(1000);
  server.startServer();
  debugPrintEnabled = DEBUG_PRINT;
}

void debug_print() {
  Serial.print((int)touchScanPeriod);
  Serial.print("\t");
  Serial.print(calibrated_average[0]);
  Serial.print("\t");
  Serial.print(calibrated_average[1]);
  Serial.print("\t");

  Serial.print((int)touch_sum);
  Serial.print("\t");
  //if (touch_sum>15) {
  Serial.print((int)(raw_touch_position*100.0));
  Serial.print("\t");
  Serial.print((int)(touch_position*100.0)); // convert [-1, 1] to percentage
  //Serial.print("\t");
  //Serial.print(valA - valB);
  Serial.print("\t");
  Serial.print(1.0/(2.0*((zeroCrossingTime - lastZeroCrossingTime)/1000000.0)));
  //Serial.print("\t");
  dtostrf(touch_sum_velocity_average,9, 2, outstr);
  Serial.print(outstr);
  //Serial.print(touch_sum_velocity_average, 2);
  Serial.print("\t");
  dtostrf(touch_sum_velocity_average/auto_far_touch_sum,9, 2, outstr);
  Serial.print(outstr);
  Serial.print("\t\t");
  //Serial.print(touch_sum_velocity_future, 2);
  //Serial.print("\t");
  Serial.print(get_current_velocity(), 2); // dick lengths per second
  Serial.print("\t");
  Serial.print(get_future_velocity(), 2);
  Serial.print("\t");
  Serial.print(min_position, 2);
  Serial.print("\t");
  Serial.print(max_position, 2);
  Serial.print("\t");
  Serial.print(temp_vibe_output*100, 2);
  Serial.print("\t");
  Serial.print(raw_pos_zero_level*100, 2);
  Serial.print("\t");
  if (lastFartherThresholdHit > 0)
    Serial.print("F");
  if (lastNearerThresholdHit > 0)
    Serial.print("N");
  //Serial.print("\t");
  //}
  //
  Serial.println();
}

void process_server_cmd() {
  std::string cmd = server.cmdCharacteristic->getValue();
  if (cmd.length() > 30) {
    return;
  }
  if (cmd.rfind("cn", 0) == 0) { // calibrate neutral
    should_calibrate = true;
    server.cmdCharacteristic->setValue(HELP_STRING);
  }
  else if (cmd.rfind("cf", 0) == 0) { // calibrate far
    should_far_calibrate = true;
    server.cmdCharacteristic->setValue(HELP_STRING);
  }
  else if (cmd.rfind("toggle", 0) == 0) { // toggle vibrator
    vibrator_on = !vibrator_on;
    client.updateVibration(0.0);
    client.updateVibration(0.0);
    server.cmdCharacteristic->setValue(HELP_STRING);
  }
  else if (cmd.rfind("reboot", 0) == 0) { // reboot this esp32
    client.updateVibration(0.0);
    client.updateVibration(0.0);
    delay(1000);
    ESP.restart();
  }
  else if (cmd.rfind("sleep", 0) == 0) { // sleep this esp32
    client.updateVibration(0.0);
    client.updateVibration(0.0);
    delay(1000);
    esp_deep_sleep_start();
  }
  else if (cmd.rfind("scth", 0) == 0) { // stroking curve threshold
    if (cmd.length() >= 6) {
      float th = atof(&cmd.c_str()[5]);
      th = constrain(th, 0.0, 10.0);
      strokingCurve_threshold = th;
      server.cmdCharacteristic->setValue(HELP_STRING);
    } else { server.cmdCharacteristic->setValue("ERROR parsing curve threshold."); }
  }
  else if (cmd.rfind("pcth", 0) == 0) { // pressure curve threshold
    if (cmd.length() >= 6) {
      float th = atof(&cmd.c_str()[5]);
      th = constrain(th, 0.0, 10000.0);
      sumCurve_threshold = th;
      server.cmdCharacteristic->setValue(HELP_STRING);
    } else { server.cmdCharacteristic->setValue("ERROR parsing curve threshold."); }
  }
  else if (cmd.rfind("arth", 0) == 0) { // approach rejection threshold
    if (cmd.length() >= 6) {
      float th = atof(&cmd.c_str()[5]);
      th = constrain(th, 0.0, 10000.0);
      sumVelocity_rejection_threshold = th;
      server.cmdCharacteristic->setValue(HELP_STRING);
    } else { server.cmdCharacteristic->setValue("ERROR parsing rejection threshold."); }
  }
  else if (cmd.rfind("tpth", 0) == 0) { // tip position threshold
    if (cmd.length() >= 6) {
      float th = atof(&cmd.c_str()[5]);
      th = constrain(th, -1.0, 1.0);
      tip_position_threshold = th;
      server.cmdCharacteristic->setValue(HELP_STRING);
    } else { server.cmdCharacteristic->setValue("ERROR tip position threshold."); }
  }
  else if (cmd.rfind("cth", 0) == 0) { // set crossover threshold
    if (cmd.length() >= 5) {
      float th = atof(&cmd.c_str()[4]);
      th = constrain(th, 0.0, 1000.0);
      current_prediction_crossover = th;
      server.cmdCharacteristic->setValue(HELP_STRING);
    } else { server.cmdCharacteristic->setValue("ERROR parsing crossover threshold."); }
  }
  else if (cmd.rfind("ssc", 0) == 0) { // stroking speed curve
    if (cmd.length() >= 5) {
      int curve = atoi(&cmd.c_str()[4]);
      curve = constrain(curve-1, 0, 6);
      strokingCurve = (byte)curve;
      server.cmdCharacteristic->setValue(HELP_STRING);
    } else { server.cmdCharacteristic->setValue("ERROR parsing curve number."); }
  }
  else if (cmd.rfind("gpc", 0) == 0) { // pressure speed curve
    if (cmd.length() >= 5) {
      int curve = atoi(&cmd.c_str()[4]);
      curve = constrain(curve-1, 0, 6);
      sumCurve = (byte)curve;
      server.cmdCharacteristic->setValue(HELP_STRING);
    } else { server.cmdCharacteristic->setValue("ERROR parsing curve number."); }
  }
  else if (cmd.rfind("mv", 0) == 0) { // set max velo
    if (cmd.length() >= 4) {
      float mv = atof(&cmd.c_str()[3]);
      mv = constrain(mv, 0.0, 30.0);
      maxVeloScale = mv;
      server.cmdCharacteristic->setValue(HELP_STRING);
    } else { server.cmdCharacteristic->setValue("ERROR parsing max velocity."); }
  }
  else if (cmd.rfind("mpv", 0) == 0) { // set max pressure velo
    if (cmd.length() >= 5) {
      float mv = atof(&cmd.c_str()[4]);
      mv = constrain(mv, 0.0, 10000.0);
      maxSumScale = mv;
      server.cmdCharacteristic->setValue(HELP_STRING);
    } else { server.cmdCharacteristic->setValue("ERROR parsing max velocity."); }
  }
  else if (cmd.rfind("mol", 0) == 0) { // set min output level
    if (cmd.length() >= 5) {
      float f = atof(&cmd.c_str()[4]);
      f = constrain(f, 0.0, 1.0);
      minimum_output_level = f;
      server.cmdCharacteristic->setValue(HELP_STRING);
    } else { server.cmdCharacteristic->setValue("ERROR parsing min output level."); }
  }
  else if (cmd.rfind("nan", 0) == 0) { // set near above neutral percentage
    if (cmd.length() >= 5) {
      float f = atof(&cmd.c_str()[4]);
      f = constrain(f, 0.0, 1.0);
      NEAR_PROPORTION_ABOVE_NEUTRAL_LEVEL = f;
      updateAutoFarTouchSum();
      server.cmdCharacteristic->setValue(HELP_STRING);
    } else { server.cmdCharacteristic->setValue("ERROR parsing near above neutral."); }
  }
  else if (cmd.rfind("get", 0) == 0) { // get current values
    std::string ret = std::string(String(calibrated_average[0]).c_str()) + " " + std::string(String(calibrated_average[1]).c_str()) + 
    " " + std::string(String(touch_sum).c_str()) + "  " + std::string(String(touch_position).c_str()) + "  " + std::string(String(raw_touch_position).c_str());

    if (lastFartherThresholdHit > 0)
      ret = ret + " F";
    if (lastNearerThresholdHit > 0)
      ret = ret + " N";
    server.cmdCharacteristic->setValue(ret);
  }
  
}

inline float get_future_velocity() {
  return touch_pos_velocity_future/2.0;
}

inline float get_current_velocity() {
  return touch_pos_velocity_average/2.0;
}

inline float get_supercurrent_velocity() {
  return touch_pos_velocity/2.0;
}

inline float get_supercurrent_sumvelocity() {
  return touch_sum_velocity;
}


void update_vibrator(int64_t currTime) {
  float future_velo = fabs(get_future_velocity());
  float current_velo = fabs(get_supercurrent_velocity());
  float estimated_velo = current_velo > current_prediction_crossover ? future_velo : current_velo;

  float frequency = 1.0/(2.0*((zeroCrossingTime - lastZeroCrossingTime)/1000000.0));
  float secondary_mol = mapf(frequency, SECONDARY_MOL_THRESHOLD_HERTZ, SECONDARY_MOL_MAX_HERTZ, SECONDARY_MOL_LOWERLEVEL
  , SECONDARY_MOL_HIGHERLEVEL);
  if (secondary_mol < SECONDARY_MOL_LOWERLEVEL)
    secondary_mol = 0.0;
  if (secondary_mol > SECONDARY_MOL_HIGHERLEVEL)
    secondary_mol = SECONDARY_MOL_HIGHERLEVEL;

  if (currTime - zeroCrossingTime > 500000) //500ms
    secondary_mol = 0.0;
  
  float pos_sum_velo = touch_sum_velocity_average < 0.0 ? 0.0 : touch_sum_velocity_average;

  /*if (isnan(estimated_velo)) {
    return;
  }*/
  
  float value = curve(estimated_velo, strokingCurve_threshold, maxVeloScale, strokingCurve);
  value = constrain(value, 0.0, 1.0);
  
  
  float sumValue = curve(pos_sum_velo/auto_far_touch_sum, sumCurve_threshold, maxSumScale, sumCurve);
  sumValue = constrain(sumValue, 0.0, 1.0);

  // if over tip position threshold and under minimum stroking velocity, pick greater of value and sumValue
  if (touch_position > tip_position_threshold && current_velo < strokingCurve_threshold) {
    value = max(value, sumValue);
  }
  
  bool isClose = touch_sum > auto_far_touch_sum; // switched to use autocalibrated far touch sum
  
  /*if(touch_sum_velocity_average < sumVelocity_rejection_threshold) { // reject when pressing or withdrawing too fast
    isClose = false;
  }*/
  
  //bool isClose = false;
  /*for (int i=0; i<CHANNEL_COUNT; i++) {
    if (measurement[i] < far_calibration[i]) {
      isClose = true;
      last_near_time = currTime;
    }
  }*/
  if (isClose) {
    /*if ((value < 0.05 && lastSentVibration >= 0.05)) {
      //client.updateVibration(lastSentVibration);
      float tmpVal = value;
      value = lastSentVibration;
      //Serial.println("Aliasing");
      lastSentVibration = tmpVal;
    }*/
    last_near_time = currTime;
    //value = max(value, minimum_output_level);
    value = constrain(max(mapf(value, 0.0, 1.0, max(minimum_output_level, secondary_mol), 1.0),
    max(minimum_output_level, secondary_mol)), 0.0, 1.0);
    if (vibrator_on)
      client.updateVibration(value);
    lastSentVibration = value;
    //Serial.println(value);
    temp_vibe_output = value;
  }
  else {
    if (currTime - last_near_time > NEAR_TIMEOUT) {
      if (vibrator_on)
        client.updateVibration(0.0);
      lastSentVibration = 0.0;
      temp_vibe_output = -1;
      //Serial.println("Near timeout");
    }
  }
}

void loop() {
  //Serial.print("Touch: ");
  //iter++;
  
  int64_t currTime = esp_timer_get_time();
  
  if (should_send_vibe) {
    should_send_vibe = false;
    
    update_vibrator(currTime);
    if (debugPrintEnabled)
      debug_print();
    //last_send_time = currTime;
  }
  if (currTime - last_check_time > CHECK_INTERVAL_US) {
    process_server_cmd();
    last_check_time = currTime;
  }
}
