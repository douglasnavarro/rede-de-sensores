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
  http.begin("http://adeusdentinho.herokuapp.com/api/movements/?format=json");
  http.setAuthorization("eletricademo", "140897hr");
  String payload = "{\"sensor\":\"13\",\"direction\": \"IN\",\"occurrence_date\": \"2017-10-29T15:00:00\"}";
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
