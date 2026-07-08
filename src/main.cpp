#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

const int NUM_PUERTAS = 9;

// Tópicos de MQTT
const char* topicos_puertas[NUM_PUERTAS] = {
  "upark/acceso/puerta/c50e4adb-e5bf-47e7-b297-7be877789b44", "upark/acceso/puerta/2", "upark/acceso/puerta/3",
  "upark/acceso/puerta/4", "upark/acceso/puerta/5", "upark/acceso/puerta/6",
  "upark/acceso/puerta/7", "upark/acceso/puerta/8", "upark/acceso/puerta/9"
};

// Pines
const int PIN_LED_VERDE = 2;
const int PIN_LED_ROJO = 4;
const int pinesServo[NUM_PUERTAS] = {12, 13, 14, 15, 16, 17, 18, 19, 21}; 

Servo barreras[NUM_PUERTAS];
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
  
  // Identificar qué puerta recibió el comando
  int puertaIndex = -1;
  for(int i = 0; i < NUM_PUERTAS; i++){
    if(String(topic) == String(topicos_puertas[i])){
      puertaIndex = i;
      break;
    }
  }

  if(puertaIndex == -1) return;

  Serial.print(">>> Acción en PUERTA ");
  Serial.print(puertaIndex + 1);
  Serial.print(": ");
  Serial.println(mensaje);

  if (mensaje == "abrir") {
    // ESTADO GENERAL: Abriendo (Verde ON, Rojo OFF)
    digitalWrite(PIN_LED_ROJO, LOW);
    digitalWrite(PIN_LED_VERDE, HIGH);
    
    // Abrir barrera específica
    barreras[puertaIndex].write(90); 
    
    delay(5000); // Espera 5 segundos
    
    // Cerrar barrera específica
    barreras[puertaIndex].write(0);  
    
    // ESTADO GENERAL: Cerrado (Verde OFF, Rojo ON)
    digitalWrite(PIN_LED_VERDE, LOW);
    digitalWrite(PIN_LED_ROJO, HIGH);
    
    Serial.println("Barrera cerrada.");
  } 
}

void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP32_UPark_Central_";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("¡Conectado a HiveMQ!");
      // Suscribirse a las 9 puertas
      for(int i = 0; i < NUM_PUERTAS; i++){
         client.subscribe(topicos_puertas[i]);
      }
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Configurar LEDs Globales
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_ROJO, OUTPUT);
  
  // Estado inicial: Todo cerrado
  digitalWrite(PIN_LED_VERDE, LOW);
  digitalWrite(PIN_LED_ROJO, HIGH);

  // Inicializar Servos
  for(int i = 0; i < NUM_PUERTAS; i++){
    barreras[i].attach(pinesServo[i]);
    barreras[i].write(0);
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