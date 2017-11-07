/*
This program is the firmware that runs in every presence sensor.
Please update wifi network information and sensor id accordingly.
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <NewPing.h>
#include <QueueList.h>

// NodeMCU Pin D1 > TRIGGER1 | Pin D2 > ECHO1
#define TRIGGER1 5
#define ECHO1    4
// NodeMCU Pin D5 > TRIGGER2 | Pin D6 > ECHO2
#define TRIGGER2 14
#define ECHO2    12

#define MAX_DISTANCE 200 //centimeters

void enqueue_occurrence(QueueList <String> queue);
String detect_passage();
void connect_to_wifi(String ssid, String password, int debug);
String post_payload(QueueList <String> queue);

// we will use this to manage detections that haven't been sent away yet
QueueList <String> queue;

// we use the sonars to register people entering or leaving
NewPing sonar1(TRIGGER1, ECHO1, MAX_DISTANCE);
NewPing sonar2(TRIGGER2, ECHO2, MAX_DISTANCE);

// every sensor must have an id
int sensor_id = 13;

// leave this =1 to see messages through the serial port
int debug = 1;

// we use this for making http requests easier
HTTPClient http;

void setup()
{
  // this is used for debugging
  if(debug = 1)
  {
    Serial.begin(115200);
    queue.setPrinter (Serial);
  }

  // we need internet connection to register people leving or entering the space
  connect_to_wifi("casa", "meiaportuguesameiamucarela", debug);

}

void connect_to_wifi(String ssid, String password, int debug)
{
  WiFi.begin(ssid, password);
  if(debug) Serial.print("Connecting to " + ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    if(debug) Serial.print(".");
  }
  if(debug)
  {
    Serial.println();
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void enqueue_occurrence(QueueList <String> queue)
{
  String direction;
  String payload;
  direction = detect_passage();
  if(direction != "")
  {
    payload = "{\"sensor\": \"" + String(sensor_id) + "\", \"direction\": \"" + direction + "\", \"occurrence_date\": \"" + date + "\"" + "}";
    queue.push(payload);
  }
}

String post_payload(QueueList <String> queue)
{
  String payload;
  int httpCode;
  String codeMessage;
  if(!queue.isEmpty())
  {
    // we need to add these headers otherwise the request won't be accepted by the server
    if(debug) Serial.println("[HTTP] begin...");
    http.begin("http://oolho.herokuapp.com/api/movements/?format=json");
    http.setAuthorization("eletricademo", "140897hr");
    http.setUserAgent("python-requests/2.2.1 CPython/3.4.3 Linux/4.4.0-43-Microsoft");
    http.addHeader("Accept", "*/*");
    http.addHeader("Accept-Encoding", "gzip, deflate, compress");
    http.addHeader("Content-Type", "application/json");

    payload = queue.pop();
    if(debug)
    {
      Serial.println("POSTing occurrence from queue:");
      Serial.println(payload);
    }

    httpCode = http.POST(payload);
    if(httpCode > 0)
    {
      codeMessage = http.getString();
      if(debug)
      {
        Serial.print("[HTTP] POST response code: ");
        Serial.println(httpCode);
        Serial.println(http.getString());
      }
    }
    else
    {
      codeMessage = http.errorToString(httpCode);
      if(debug)
      {
        Serial.print("[HTTP] POST failed with response code:");
        Serial.print(http.errorToString(httpCode));
      }
    }
    http.end();
  }
  else
  {
    if(debug) Serial.println("Queue empty! No request was made.");
  }
}

String detect_passage()
{
  String direction = "";
  unsigned int distance1 = sonar1.ping_cm();
  if(distance1 > 0){
    delay(50); //wait a little so the target moves away from the first sensor's direction  into the second sensor direction
    unsigned int distance2 = sonar2.ping_cm();
    if(distance2 > 0)
    {
      if(debug)
      {
        Serial.println("Entry detected!");
        Serial.println("Stading by");
      }
      delay(2000);
      direction = "IN";
      return direction;
    }
  }
  unsigned int distance2 = sonar2.ping_cm();
  if(distance2 > 0){
    delay(50); //wait a little so the target moves away from the second sensor's direction  into the first sensor's direction
    unsigned int distance1 = sonar1.ping_cm();
    if(distance1 > 0)
    {
      if(debug)
      {
        Serial.println("Exit detected!");
        Serial.println("Stading by");
      }
      delay(2000);
      direction = "OUT";
      return direction;
    }
  }
  return direction;
  delay(50);
}
