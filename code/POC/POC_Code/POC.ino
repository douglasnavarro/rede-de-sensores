#include <ESP8266WiFi.h>

#define TRIGGER 5
#define ECHO    4
// NodeMCU Pin D1 > TRIGGER | Pin D2 > ECHO

const char* ssid = "codedecay1";
const char* password = "codecode";
char enderecoThingspeak[] = "api.thingspeak.com"; // endereço do Thingspeak
String keyThingspeak = "TM5QCZ16XSFD9TW5"; // a chave de escrita do canal, necessario para autorização
int contador = 0;

void setup() {

  Serial.begin (9600);
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH); //Apaga o LED que é ativo baixo

 WiFi.begin(ssid, password);
 Serial.println("");
 Serial.println("");
 Serial.print("Conectando a rede ");
 Serial.println(ssid);

 WiFi.begin(ssid, password);

 while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
 }

 Serial.println("");
 Serial.println("Conectado ao WiFi!");
 Serial.println("Endereço IP: ");
 Serial.println(WiFi.localIP());
 Serial.println("");
}

void loop() {

  if(detecta_passagem(100, 25, 1))
  {
    contador++;
    Serial.print("Detectou passagem! Contador =  ");
    Serial.println(contador);
    pisca_led(100, LED_BUILTIN);
    if(contador%5 == 0){
      pisca_led(500, LED_BUILTIN);
      Serial.println("Registrando no Thingspeak!");
     registra_contador(++contador);
    }
  }
}

void pisca_led(int tempo, int pino){
   digitalWrite(pino, LOW);
   delay(tempo);
   digitalWrite(pino, HIGH);
   delay(tempo);
}

void registra_contador(int contador){

  Serial.print("Conectando a ");
  Serial.println(enderecoThingspeak);

  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(enderecoThingspeak, httpPort)) {
    Serial.println("Conexao com o servidor falhou!");
    return;
  }

  String cabecalho = "GET /update?key=" + keyThingspeak + "&field1=" + contador + " HTTP/1.1";
  client.println(cabecalho);
  client.println("Host: api.thingspeak.com");
  client.println("Connection: close");
  client.println();

  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
}

int detecta_passagem(int threshold, int media, int debug){
  long duration, distance, distanceSum;

  distanceSum = 0;
  for(int i=0; i < media; i++)
  {
    digitalWrite(TRIGGER, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIGGER, HIGH);
    delayMicroseconds(10);

    digitalWrite(TRIGGER, LOW);
    duration = pulseIn(ECHO, HIGH);
    distance = (duration/2) / 29.1;
    if(debug) Serial.println(distance);
    distanceSum += distance;
    delay(40);
  }

  distanceSum = distanceSum/media;
  if(debug)
  {
    Serial.print("Distancia media: ");
    Serial.println(distanceSum);
  }
  return(distanceSum < threshold);

}
