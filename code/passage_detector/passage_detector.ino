#include <ESP8266WiFi.h>
#include <NewPing.h>

// NodeMCU Pin D1 > TRIGGER1 | Pin D2 > ECHO1
#define TRIGGER1 5
#define ECHO1    4
// NodeMCU Pin D5 > TRIGGER2 | Pin D6 > ECHO2
#define TRIGGER2 14
#define ECHO2    12

#define MAX_DISTANCE 200

NewPing sonar1(TRIGGER1, ECHO1, MAX_DISTANCE);
NewPing sonar2(TRIGGER2, ECHO2, MAX_DISTANCE);
int readings = 1;

int contador = 0;

void setup() {
  Serial.begin (9600);
  digitalWrite(BUILTIN_LED, HIGH); //Apaga o LED que é ativo baixo
}

void loop() {
  unsigned int distance1 = sonar1.ping_cm();
  if(distance1 > 0){
    delay(50);
    unsigned int distance2 = sonar2.ping_cm();
    if(distance2 > 0)
    {
      Serial.println("Entry detected!");
      delay(2000);
      Serial.println("Stading by");
    }

  }
  unsigned int distance2 = sonar2.ping_cm();
  if(distance2 > 0){
    delay(50);
    unsigned int distance1 = sonar1.ping_cm();
    if(distance1 > 0)
    {
      Serial.println("Exit detected!");
      delay(2000);
      Serial.println("Stading by");
    }
  }
  delay(50);
}


unsigned int average_distance(NewPing sonar, int samples){
  unsigned int average_distance = 0;
  unsigned int distance = 0;
  for(int i = 0; i < samples; i++)
  {
    distance += sonar.ping_cm();
    delay(10);
  }
  average_distance = distance / samples;
  return average_distance;
}
/**detect_passage
    Detects someone passing by the sensor.

    @param threshold - no-detection distance
    @param debug     - 1 for debug printing
    @return          - 0 for no detection
    @return          - 1 for entry detection
    @return          - 2 for exit detection
*/
int detect_passage(NewPing sonar1, NewPing sonar2){



}

void blink(int duration, int pin){
   digitalWrite(pin, LOW);
   delay(duration);
   digitalWrite(pin, HIGH);
   delay(duration);
}
