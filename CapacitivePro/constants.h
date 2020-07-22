#ifndef Constants_h
#define Constants_h

#define SERVICE_UUID        "8a73bbcc-e84a-4075-bb46-ad1c16ff315e"
#define CHAR_COMMANDSTRING "90110621-d087-4aaa-9dd9-729e5d892088"
static const std::string HELP_STRING = "READY\nType commands here."; //\nList of commands:\n\ncn <calibrate neutral>\ncc <calibrate close position>\ncf <calibrate far position>\n"
  //"lcon <turn on latency compensation>\nlcoff <turn off latency compensation>\nssc NUMBER <stroking speed curve, from 1 to 5>\n"
  //"gpc NUMBER <glans pressure speed curve, from 1 to 5>\nsfc NUMBER <stroking frequency curve, from 1 to 5>\nf NUMBER <stimulation scale, from 0.0 to 1.0>\n";
  //"toggle <toggle vibrator on/off>\ncth NUMBER <prediction crossover threshold, number is speed from 0.0 to around 3.0>\n";
  //"scth NUMBER <stroking curve lower threshold, number is speed from 0.0 to around 0.5>\nmv NUMBER <maximum velocity, number from 0.0 to around 4.0>\n";
  //"mpv NUMBER <max pressure velocity>\n";
  //"pcth NUMBER <pressure curve lower threshold, number is pressure velo from 0.0 to around 50.0>\n";
  //"arth NUMBER <approach rejection threshold, number is pressure velo threshold, good around -3000>\n";
  //"tpth NUMBER <tip position threshold, number is position on shaft from -1.0 to 1.0, maybe good around 0.15>\n";
  //"mol 0.05 <set minimum output level from 0.0 to 1.0>";
  
#endif
