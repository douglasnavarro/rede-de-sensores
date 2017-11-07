#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

void setup()
{
  Serial.begin(115200);
  Serial.println();

  WiFi.begin("casa", "meiaportuguesameiamucarela");

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  HTTPClient http;
  Serial.println("[HTTP] begin...");
  http.begin("http://oolho.herokuapp.com/api/movements/?format=json");
  //http.begin("http://putsreq.com/en1tLq0W1bDQnTOte05d"); // add /inspect use this to inspect the requests
  http.setAuthorization("eletricademo", "140897hr");
  http.setUserAgent("python-requests/2.2.1 CPython/3.4.3 Linux/4.4.0-43-Microsoft");
  http.addHeader("Accept", "*/*");
  http.addHeader("Accept-Encoding", "gzip, deflate, compress");
  http.addHeader("Content-Type", "application/json");


  String payload = "{\"sensor\": \"13\", \"direction\": \"IN\", \"occurrence_date\": \"2017-11-04T10:11:12\"}";
  Serial.println(payload);

  int httpCode = http.POST(payload);
  
  if(httpCode > 0)
  {
    Serial.print("[HTTP] POST response code: ");
    Serial.println(httpCode);
    Serial.println(http.getString());
  }
  else
  {
    Serial.print("[HTTP] POST failed with response code:");
    Serial.print(http.errorToString(httpCode));
  }
  http.end();
}

void loop() {}
