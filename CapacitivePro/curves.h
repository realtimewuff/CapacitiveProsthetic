#ifndef Curves_h
#define Curves_h
#include "Arduino.h"

inline float mapf(float val, float in_min, float in_max, float out_min, float out_max);

inline float inverseSmoothstep(float x);

inline float inverseSmootherstep(float x);
float curve(float velocityRaw, float threshold, float velocityMaximum, byte curveType);

#endif
