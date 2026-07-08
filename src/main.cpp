#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

const int NUM_BARRERAS = 18;

// Tópicos de MQTT para cada barrera (IN y OUT) con sus respectivos UUIDs
const char* topicos_barreras[NUM_BARRERAS] = {
  "upark/acceso/puerta/b62905f1-df85-4353-abed-9e22aeb7f127",  // Puerta 1 - IN
  "upark/acceso/puerta/638bf422-99d0-4cc4-bdb3-50d12b05159a",  // Puerta 1 - OUT
  "upark/acceso/puerta/7763aec5-3fe8-484a-ac57-993d89d51ce7",  // Puerta 2 - IN
  "upark/acceso/puerta/e2d0bdfb-9ce9-4574-9918-b950c5c23d43",  // Puerta 2 - OUT
  "upark/acceso/puerta/ea0fcf4c-710f-4a5a-bcef-c37a3838ac55",  // Puerta 3 - IN
  "upark/acceso/puerta/c2e34a47-22fc-4570-a6c4-1347316ca31e",  // Puerta 3 - OUT D
  "upark/acceso/puerta/3717f1c2-6283-4361-a9f4-84d3ab70437b",  // Puerta 4 - IN
  "upark/acceso/puerta/8f824a1f-3a41-4952-a4de-56d4de441f2c",  // Puerta 4 - OUT
  "upark/acceso/puerta/9d71ebf1-d7bf-4ba2-8417-593e90afcf3a",  // Puerta 5 - IN
  "upark/acceso/puerta/537bbea7-f8ba-4583-a746-59098db8bbb6",  // Puerta 5 - OUT
  "upark/acceso/puerta/1f974843-4f97-4e13-8034-7cd3103767f4",  // Puerta 6 - IN
  "upark/acceso/puerta/c6ed8692-38b7-4f6d-9246-7fed80633359",  // Puerta 6 - OUT
  "upark/acceso/puerta/a574c6b6-e57b-4a20-92f5-790dc0a5fec1",  // Puerta 7 - IN
  "upark/acceso/puerta/eee7e688-9963-4d7a-b6ae-5342f79b598b",  // Puerta 7 - OUT
  "upark/acceso/puerta/18b5f8c6-2337-4ba5-800a-25a8777b31be",  // Puerta 8 - IN
  "upark/acceso/puerta/f2c61ae5-35c1-4c53-ac73-f843d5dd6306",  // Puerta 8 - OUT
  "upark/acceso/puerta/f7eff2fd-b383-4adf-81fe-18f42ba4e10e",  // Puerta 9 - IN
  "upark/acceso/puerta/73a44545-37fe-4579-bf02-f2804de4a097"   // Puerta 9 - OUT D
};

const int PIN_LED_VERDE = 2;
const int PIN_LED_ROJO = 0; 

const int pinesServo[NUM_BARRERAS] = {
  4, 19,   // Puerta 1 (IN, OUT)
  5, 21,   // Puerta 2 (IN, OUT)
  12, -1,  // Puerta 3 (IN, OUT)
  13, 23,  // Puerta 4 (IN, OUT)
  14, 25,  // Puerta 5 (IN, OUT)
  15, 26,  // Puerta 6 (IN, OUT)
  16, 27,  // Puerta 7 (IN, OUT)
  17, 32,  // Puerta 8 (IN, OUT)
  18, -1   // Puerta 9 (IN, OUT)
}; 

Servo barreras[NUM_BARRERAS];

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  Serial.println(" ¡Conectado!");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String mensaje = "";
  for (int i = 0; i < length; i++) {
    mensaje += (char)payload[i];
  }
  
  int barreraIndex = -1;
  for(int i = 0; i < NUM_BARRERAS; i++){
    if(String(topic) == String(topicos_barreras[i])){
      barreraIndex = i;
      break;
    }
  }

  if(barreraIndex == -1) return;

  Serial.print(">>> Acción en BARRERA índice ");
  Serial.print(barreraIndex);
  Serial.print(": ");
  Serial.println(mensaje);

  if (mensaje == "abrir") {
    Serial.println(">>> ACCESO PERMITIDO");
    
    digitalWrite(PIN_LED_ROJO, LOW);
    digitalWrite(PIN_LED_VERDE, HIGH);
    
    // Solo intentamos mover el motor si NO tiene el pin -1
    if (pinesServo[barreraIndex] != -1) {
      barreras[barreraIndex].write(90); 
    } else {
      Serial.println("    (Nota: Barrera visual, no hay motor conectado)");
    }
    
    delay(5000); 
    
    if (pinesServo[barreraIndex] != -1) {
      barreras[barreraIndex].write(0);  
    }
    
    digitalWrite(PIN_LED_VERDE, LOW);
    digitalWrite(PIN_LED_ROJO, HIGH);
    
    Serial.println(">>> Ciclo completado.");
  } 
  else if (mensaje == "denegado") {
    Serial.println(">>> ACCESO DENEGADO");
    for(int i = 0; i < 3; i++) {
      digitalWrite(PIN_LED_ROJO, LOW);
      delay(200);
      digitalWrite(PIN_LED_ROJO, HIGH);
      delay(200);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP32_UPark_Central_";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("¡Conectado a HiveMQ!");
      for(int i = 0; i < NUM_BARRERAS; i++){
         client.subscribe(topicos_barreras[i]);
      }
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_ROJO, OUTPUT);
  digitalWrite(PIN_LED_VERDE, LOW);
  digitalWrite(PIN_LED_ROJO, HIGH);

  // Inicializar los servomotores que tienen un pin asignado
  for(int i = 0; i < NUM_BARRERAS; i++){
    if (pinesServo[i] != -1) {
      barreras[i].attach(pinesServo[i]);
      barreras[i].write(0);
    }
  }

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}



