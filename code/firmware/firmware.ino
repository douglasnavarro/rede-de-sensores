/*
This program is the firmware that runs in every presence sensor.
Please update wifi network information and sensor id accordingly.
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <NewPing.h>
#include <QueueList.h>
#include <Arduino.h>

// NodeMCU Pin D1 > TRIGGER1 | Pin D2 > ECHO1
#define TRIGGER1 5
#define ECHO1    4
// NodeMCU Pin D5 > TRIGGER2 | Pin D6 > ECHO2
#define TRIGGER2 14
#define ECHO2    12

#define MAX_DISTANCE 100 //centimeters

int connect_to_wifi(String ssid, String password, int tries, int debug);
String detect_passage(int debug);
void enqueue_occurrence(int debug);
String post_payload(int debug);

// we will use this to manage detections that haven't been sent away yet
QueueList <String> queue;

// we use the sonars to register people entering or leaving
NewPing sonar1(TRIGGER1, ECHO1, MAX_DISTANCE);
NewPing sonar2(TRIGGER2, ECHO2, MAX_DISTANCE);

String ssid = "casa";
String password = "meiaportuguesameiamucarela";

// every sensor must have an id
int sensor_id = 13;

// leave this =1 to see messages through the serial port
int debug = 1;

// we use this for making http requests easier
HTTPClient http;

void setup()
{
  if(debug)
  {
    Serial.begin(115200);
    queue.setPrinter (Serial);
  }
  connect_to_wifi(ssid, password, 10, 1);
}

void loop()
{
  // we need to continously scan for people entering or leaving
  enqueue_occurrence(debug);

  // we need to have wifi online in order do register the occurrences
  int status = WiFi.status();
  if(status != WL_CONNECTED)
  {
    if(debug) Serial.println("WiFi offline! Trying to reconnect!");
    // we can't stop scanning for people. try reconnecting but not too hard
    status = connect_to_wifi(ssid, password, debug, 1);
  }
  else
  {
    Serial.println(post_payload(debug));
    if(debug)
    {
      Serial.println();
      Serial.print("Queue now has "); Serial.print(queue.count()); Serial.println(" payloads");
    }
  }
}

int connect_to_wifi(String ssid, String password, int tries, int debug)
{
  if(debug) Serial.println();
  WiFi.begin(ssid.c_str(), password.c_str());
  if(debug) Serial.print("Connecting to " + ssid);
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter < tries)
  {
    delay(500);
    if(debug) Serial.print(".");
    counter++;
  }
  int status = WiFi.status();
  if(status == WL_CONNECTED)
  {
    if(debug)
    {
      Serial.println();
      Serial.print("Connected, IP address: ");
      Serial.println(WiFi.localIP());
    }
    return status;
  }
  else
  {
    if(debug) Serial.println("Unable to connect to Wifi!");
    return status;
  }
}

String detect_passage(int debug)
{
  String direction = "";
  delay(150); // avoid echo1 caused by trigger2
  unsigned int distance1 = sonar1.ping_cm();
  delay(150); //avoid echo2 caused by trigger1
  unsigned int distance2 = sonar2.ping_cm();
  if(distance1 > 0)
  {
    if(debug)
    {
      Serial.println();
      Serial.print("Sensor 1 enabled!");
      Serial.print("("); Serial.print(distance1); Serial.print(")");
      Serial.print("Waiting for sensor 2");
    }
    for(int i = 0; i < 10; i++)
    {
      Serial.print(".");
      distance2 = sonar2.ping_cm();
      if(distance2 > 0)
      {
        if(debug)
        {
          Serial.println();
          Serial.println("Entry detected!");
          Serial.println("Waiting for person to leave sensor 2...");
        }
        delay(500);
        direction = "IN";
        if(debug) Serial.println("---");
        return direction;
      }
      delay(100);
    }
  }
  if(distance2 > 0)
  {
    if(debug)
      {
        Serial.println();
        Serial.print("Sensor 2 enabled!");
        Serial.print("("); Serial.print(distance2); Serial.print(")");
        Serial.print("Waiting for sensor 1");
      }
    for(int i = 0; i < 10; i++)
    {
      Serial.print(".");
      distance1 = sonar1.ping_cm();
      if(distance1 > 0)
      {
        if(debug)
        {
          Serial.println();
          Serial.println("Exit detected!");
          Serial.println("Waiting for person to leave sensor 1...");
        }
        delay(500);
        direction = "OUT";
        if(debug) Serial.println("---");
        return direction;
      }
      delay(100);
    }
  }
  return direction;
}

void enqueue_occurrence(int debug)
{
  String direction;
  String payload;
  direction = detect_passage(debug);
  String date = "2017-11-05T12:13:14";
  if(direction != "")
  {
    payload = "{\"sensor\": \"" + String(sensor_id) + "\", \"direction\": \"" + direction + "\", \"occurrence_date\": \"" + date + "\"" + "}";
    if(debug)
    {
      Serial.print("Pushing ");
      Serial.println(payload);
    }
    queue.push(payload);
  }
}

String post_payload(int debug)
{
  if(WiFi.status() != WL_CONNECTED) return "Wifi not connected!";
  String payload;
  int httpCode;
  String codeMessage;
  if(!queue.isEmpty())
  {
    // we need to add these headers otherwise the request won't be accepted by the server
    if(debug) Serial.println("[HTTP] begin...");
    http.begin("http://adeusdentinho.herokuapp.com/api/movements/?format=json");
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
      return codeMessage;
    }
    else
    {
      codeMessage = http.errorToString(httpCode);
      if(debug)
      {
        Serial.print("[HTTP] POST failed with response code:");
        Serial.print(http.errorToString(httpCode));
      }
      return codeMessage;
    }
    http.end();
  }
  else
  {
    if(debug) Serial.println("Queue empty! No request was made.");
    return "Queue empty!";
  }
}
