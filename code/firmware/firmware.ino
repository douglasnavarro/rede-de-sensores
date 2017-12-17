/*
This program is the firmware that runs in every presence sensor.
Please update wifi network information and sensor id accordingly.
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <NewPing.h>
#include <QueueList.h>
#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"

// NodeMCU Pin D7 > TRIGGER1 | Pin D8 > ECHO1
#define TRIGGER1 13
#define ECHO1    15
// NodeMCU Pin D5 > TRIGGER2 | Pin D6 > ECHO2
#define TRIGGER2 10
#define ECHO2    9

#define MAX_DISTANCE 50 //centimeters

int connect_to_wifi(String ssid, String password, int tries, int debug);
int update_counter(int debug, int counter, int rate);
void enqueue_occurrence(int debug);
int post_payload(int debug, String payload);
String stringfy_datetime(DateTime now);
String assemble_payload(int debug, int counter, String date);
void blink_led(int times);

// we will use this to manage detections that haven't been sent away yet
QueueList <String> queue;

// we use the sonars to register people entering or leaving
NewPing sonar1(TRIGGER1, ECHO1, MAX_DISTANCE);
NewPing sonar2(TRIGGER2, ECHO2, MAX_DISTANCE);

// we use the RTC to register when occurrences were registered and to decide when to post them
RTC_DS1307 rtc;

String ssid = "casa";
String password = "meiaportuguesameiamucarela";

// every sensor must have an id
int sensor_id = 14;

// leave this =1 to see messages through the serial port
int debug = 0;

// we assemble a payload every hour and post them every 15 minutes
int timestamp, old_timestamp_hour, old_timestamp_minute;

// set these to change balance frequency and queue decongestion
int balance_period = 3600; //every hour
int decongestion_period = 15*60; // 15 minutes

// variables used in the main execution loop
int counter;
int httpCode;
int wifi_status;
String payload;

// we use this for making http requests easier
HTTPClient http;

void setup()
{
  // we use the onboard led to signal detections
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  if(debug)
  {
    Serial.begin(115200);
    queue.setPrinter (Serial);
  }

  //we can't have the system working without a clock
  if (!rtc.begin())
  {
    if(debug) Serial.println("Couldn't find RTC");
    while (1);
  }

//  if (!rtc.isrunning())
//  {
//    if(debug) Serial.println("RTC is NOT running!");
//    // following line sets the RTC to the date & time this sketch was compiled
//    DateTime compilation = DateTime(F(__DATE__), F(__TIME__));
//    rtc.adjust(compilation);
//    if(debug)
//    {
//      Serial.print("RTC time was adjusted to ");
//      Serial.print(compilation.year()); Serial.print("-");Serial.print(compilation.month());Serial.print("-");Serial.print(compilation.day());
//      Serial.print(" ");Serial.print(compilation.hour());Serial.print(":");Serial.print(compilation.minute());
//      // This line sets the RTC with an explicit date & time, for example to set
//      // January 21, 2014 at 3am you would call:
//      // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
//    }
//  }

  wifi_status = connect_to_wifi(ssid, password, 10, debug);

  DateTime now         = rtc.now();
  timestamp            = now.secondstime();
  old_timestamp_hour   = timestamp;
  old_timestamp_minute = timestamp;

  //used for testing
  // old_timestamp_hour   = timestamp - 3540;
  // old_timestamp_minute = timestamp;
//  DateTime compilation = DateTime(F(__DATE__), F(__TIME__));
//  rtc.adjust(compilation);
  
  counter = 0;
}


void loop()
{
  // we need to continously scan for people entering or leaving
  counter = update_counter(debug, counter, 40);

  DateTime now = rtc.now();
  timestamp    = now.secondstime();
  String date  = stringfy_datetime(now);

  if(timestamp > old_timestamp_hour + balance_period) //we must assemble payload and decide if we post to server or push to queue
  {
    old_timestamp_hour = timestamp;
    payload       = assemble_payload(debug, counter, date);
    wifi_status   = WiFi.status();

    if(wifi_status == WL_CONNECTED)
    {
      int httpCode = post_payload(debug, payload);
      if (httpCode < 0) queue.push(payload);
    }
    else
    {
      if(debug) Serial.println("WiFi was offline! Pushing payload to queue!");
      queue.push(payload);
    }
  }

//  else if(timestamp > old_timestamp_minute + decongestion_period) //we must try to post a payload off the queue
//  {
//    old_timestamp_minute = timestamp;
//    wifi_status = WiFi.status();
//    if(!queue.isEmpty() && wifi_status == WL_CONNECTED)
//    {
//      payload  = queue.pop();
//      httpCode = post_payload(debug, payload);
//      if(httpCode < 0)
//      {
//        queue.push(payload);
//      }
//      else if(wifi_status != WL_CONNECTED)
//      {
//        wifi_status = connect_to_wifi(ssid, password, 3, debug);
//      }
//    }
//  }
  if(debug)
  {
    Serial.println();
    Serial.println("--------");
    Serial.println(date);
    Serial.print("timestamp = "); Serial.println(timestamp);
    Serial.print("next hour = "); Serial.print(old_timestamp_hour+3600); Serial.print(" next minute = "); Serial.println(old_timestamp_minute + 180);
    Serial.println();
    Serial.print("Counter = ");Serial.println(counter);
    Serial.print("Queue now has "); Serial.print(queue.count()); Serial.println(" payloads");
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

int update_counter(int debug, int counter, int rate)
{
  delay(rate); // avoid echo1 caused by trigger2
  unsigned int distance1 = sonar1.ping_cm();
  delay(rate); //avoid echo2 caused by trigger1
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
    for(int i = 0; i < 40; i++)
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
        blink_led(1);
        delay(1000);
        counter = counter + 1;
        if(debug) Serial.println("---");
        return counter;
      }
      delay(rate);
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
    for(int i = 0; i < 40; i++)
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
        blink_led(4);
        delay(1000);
        counter = counter - 1;
        if(debug) Serial.println("---");
        return counter;
      }
      delay(rate);
    }
  }
  return counter;
}

String assemble_payload(int debug, int counter, String date)
{
  String direction, value;
  counter < 0 ? value = String((counter*-1)) : value = String(counter);
  if(counter > 0) direction = "IN";
  else direction = "OUT";
  String payload = "{\"sensor\": \"" + String(sensor_id) + "\", \"direction\": \"" + direction + "\", \"occurrence_date\": \"" + date + "\"" + ", \"value\": \"" + value + "\"" + "}";
  return payload;
}

int post_payload(int debug, String payload)
{
  int httpCode;
  String codeMessage;
  if(debug) Serial.println("[HTTP] begin...");
  http.begin("http://oolho.herokuapp.com/api/movements/?format=json");
  http.setAuthorization("eletricademo", "140897hr");
  http.setUserAgent("python-requests/2.2.1 CPython/3.4.3 Linux/4.4.0-43-Microsoft");
  http.addHeader("Accept", "*/*");
  http.addHeader("Accept-Encoding", "gzip, deflate, compress");
  http.addHeader("Content-Type", "application/json");
  if(debug)
  {
    Serial.println("POSTing occurrence:");
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
      Serial.println(codeMessage);
    }
    http.end();
    return httpCode;
  }
  else
  {
    codeMessage = http.errorToString(httpCode);
    if(debug)
    {
      Serial.print("[HTTP] POST failed with response code:");
      Serial.print(codeMessage);
    }
    http.end();
    return httpCode;
  }
}

String stringfy_datetime(DateTime now)
{
  String year, month, day, hour, minute, second;
  (now.year() < 10)   ? year = "0" + String(now.year()) : year = String(now.year());
  (now.month() < 10)  ? month = "0" + String(now.month()) : month = String(now.month());
  (now.day() < 10)    ? day = "0" + String(now.day()) : day = String(now.day());
  (now.hour() < 10)   ? hour = "0" + String(now.hour()) : hour = String(now.hour());
  (now.minute() < 10) ? minute = "0" + String(now.minute()) : minute = String(now.minute());
  (now.second() < 10) ? second = "0" + String(now.second()) : second = String(now.second());
  String date = year + "-" + month + "-" + day + "T" + hour + ":" + minute + ":" + second;
  return date;
}

void blink_led(int times)
{
  int i;
  for(i=0; i < times; i++)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
  }
}

