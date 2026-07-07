#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// 1. Configuración de Red WiFi (En Wokwi usa "Wokwi-GUEST")
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// 2. Configuración del Broker MQTT (Ejemplo usando HiveMQ público o tu IP local)
const char* mqtt_server = "mqtt.eclipseprojects.io";
const int mqtt_port = 1883;
const char* mqtt_topic = "upark/acceso/puerta";

// 3. Definición de Pines
const int pinServo = 18;
const int pinLedVerde = 2;
const int pinLedRojo = 4;

WiFiClient espClient;
PubSubClient client(espClient);
Servo barreraServo;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("¡WiFi conectado exitosamente!");
  Serial.print("IP asignada: ");
  Serial.println(WiFi.localIP());
}

// 4. Función que se ejecuta cuando llega el mensaje del Backend FastAPI
void callback(char* topic, byte* payload, unsigned int length) {
  String mensaje = "";
  for (int i = 0; i < length; i++) {
    mensaje += (char)payload[i];
  }
  
  Serial.print("Mensaje MQTT recibido en [" + String(topic) + "]: ");
  Serial.println(mensaje);

  // Lógica de control de la barrera (UPark)
  if (mensaje == "abrir") {
    Serial.println(">>> ACCESO PERMITIDO: Abriendo barrera...");
    digitalWrite(pinLedRojo, LOW);
    digitalWrite(pinLedVerde, HIGH);
    
    barreraServo.write(90); // Levanta el brazo (90 grados)
    delay(5000);            // Espera 5 segundos a que pase el auto
    
    barreraServo.write(0);  // Baja el brazo (0 grados)
    digitalWrite(pinLedVerde, LOW);
    digitalWrite(pinLedRojo, HIGH);
    Serial.println(">>> Barrera cerrada.");
  } 
  else if (mensaje == "denegado") {
    Serial.println(">>> ACCESO DENEGADO");
    // Parpadeo de alerta en el LED rojo
    for(int i = 0; i < 3; i++) {
      digitalWrite(pinLedRojo, LOW);
      delay(200);
      digitalWrite(pinLedRojo, HIGH);
      delay(200);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    // ID único de cliente
    String clientId = "ESP32_UPark_Client_";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("¡Conectado al Broker MQTT!");
      // Suscribirse al tópico de UPark
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(client.state());
      Serial.println(" -> Reintentando en 5 segundos...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Configuración de pines
  pinMode(pinLedVerde, OUTPUT);
  pinMode(pinLedRojo, OUTPUT);
  
  // Estado inicial: Puerta cerrada, LED rojo encendido
  digitalWrite(pinLedVerde, LOW);
  digitalWrite(pinLedRojo, HIGH);
  
  barreraServo.attach(pinServo);
  barreraServo.write(0); // Barrera en posición horizontal (0 grados)

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setKeepAlive(90);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}