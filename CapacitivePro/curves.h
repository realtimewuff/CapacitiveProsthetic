inline float mapf(float val, float in_min, float in_max, float out_min, float out_max) {
    return constrain((val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min, out_min, out_max);
}

inline float inverseSmoothstep(float x) {
    return x + (x - (x * x * (3.0 - 2.0 * x)));
}

inline float inverseSmootherstep(float x) {
    return x + (x - (6*x*x*x*x*x-15*x*x*x*x+10*x*x*x));
}

float curve(float velocityRaw, float threshold, float velocityMaximum, byte curveType)
{
  //Curve Type 0 : Linear
  if (curveType == 0)
  {
    return mapf(velocityRaw, threshold, velocityMaximum, 0.0, 1.0); //mapf the value in linear range 1/127
  }

  //Curve Type 1 : exp 1
  else if (curveType == 1)
  {
    float resF = mapf(velocityRaw, threshold, velocityMaximum, 1, 127);

    if (resF <= 1)
    {
      resF = 1;
    }

    if (resF > 127)
    {
      resF = 127;
    }

    resF = (126 / (pow(1.02, 126) - 1)) * (pow(1.02, resF - 1) - 1) + 1; // 1.02

    return mapf(resF, 1.0, 127.0, 0.0, 1.0);
  }

  //Curve Type 2 : exp 2
  else if (curveType == 2)
  {
    float resF = mapf(velocityRaw, threshold, velocityMaximum, 1, 127);

    if (resF <= 1)
    {
      resF = 1;
    }

    if (resF > 127)
    {
      resF = 127;
    }

    resF = (126 / (pow(1.05, 126) - 1)) * (pow(1.05, resF - 1) - 1) + 1; // 1.05

    return mapf(resF, 1.0, 127.0, 0.0, 1.0);
  }

  //Curve Type 3 : log 1
  else if (curveType == 3)
  {
    float resF = mapf(velocityRaw, threshold, velocityMaximum, 1, 127);

    if (resF <= 1)
    {
      resF = 1;
    }

    if (resF > 127)
    {
      resF = 127;
    }

    resF = (126 / (pow(0.98, 126) - 1)) * (pow(0.98, resF - 1) - 1) + 1; // 0.98

    return mapf(resF, 1.0, 127.0, 0.0, 1.0);
  }

  //Curve Type 4 : log 2
  else if (curveType == 4)
  {
    float resF = mapf(velocityRaw, threshold, velocityMaximum, 1, 127);

    if (resF <= 1)
    {
      resF = 1;
    }

    if (resF > 127)
    {
      resF = 127;
    }

    resF = (126 / (pow(0.95, 126) - 1)) * (pow(0.95, resF - 1) - 1) + 1; // 0.95

    return mapf(resF, 1.0, 127.0, 0.0, 1.0);
  }

  else if (curveType == 5) {
    float resF = mapf(velocityRaw, threshold, velocityMaximum, 0.0, 1.0);

    return constrain(inverseSmoothstep(resF), 0.0, 1.0);
  }

  else if (curveType == 6) {
    float resF = mapf(velocityRaw, threshold, velocityMaximum, 0.0, 1.0);

    return constrain(inverseSmootherstep(resF), 0.0, 1.0);
  }

  else
  {
    return 0;
  }
}
