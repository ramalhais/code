#ifndef _BLUESENSE_SENSORS_
#define _BLUESENSE_SENSORS_

int getAmbientTemperature() {
  static int v = 0;
  //get temperature from chip internal sensor
  return v;
}

int getEngineTemperature() {
  static int v = 0;

  int r = analogRead(PIN_TEMPERATURE_ENGINE);
  // Correct value
  v = r;

  return v;
}

int getEngineRpm() {
  static int v = 0;

  //main chip:LM393,3144 Hall sensor
  int r = analogRead(PIN_HALL_RPM);
  // Correct value
  v = r;

  return v;
}

#define RIMSIZE 18
#define TIRESIZE 80
#define HEIGHTPERCENT 100
#define INCH2MM 25.4
#define R_TIRE_MM ((RIMSIZE*INCH2MM)/2)+(TIRESIZE*(HEIGHTPERCENT/100))
int bikeKPH = 0;
int bikeKPHReadMillis = millis();
int bikeKPHReadDiffMillis = 0;
int updateBikeKPH() {
  //main chip:LM393,3144 Hall sensor
  int r = digitalRead(PIN_HALL_SPEED);
  int tr = millis();
  int diffMillis = tr-bikeKPHReadMillis;
  if (r == LOW) {
    // Correct value
    // Each turn is a complete wheel perimeter: 2*PI*r with r=(rimsize/2)+(tiresize*heightpercent)
    bikeKPH = 2*PI*((R_TIRE_MM)/(diffMillis))*((1000*60*60)/1000000);
    bikeKPHReadDiffMillis = diffMillis;
  }   
  else if (diffMillis > bikeKPHReadDiffMillis) {
    bikeKPH = 2*PI*((R_TIRE_MM)/(diffMillis))*((1000*60*60)/1000000);
  }   
  else {
    return -1;
  }

  return bikeKPH;
}

int getBikeKPH() {
  return bikeKPH;
}
#endif

