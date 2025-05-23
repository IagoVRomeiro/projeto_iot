#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <TMP36.h>  // Inclui a biblioteca TMP36

#define LED_BUILTIN 2
#define PIN_LED 2

#define rele 5  // Pino GPIO para o relé
#define sensor 4  // Pino GPIO para o sensor de solo
bool irrigar = false;


/* Definições para o MQTT */
#define TOPICO_SUBSCRIBE_LED "topico_led"
#define TOPICO_PUBLISH_TEMPERATURA "topico_sensor_temperatura"
#define TOPICO_PUBLISH_UMIDADE "topico_sensor_umidade"
#define TOPICO_PUBLISH_CHUVA "topico_sensor_chuva"
#define ID_MQTT "IoT_PUC_SG_mqtt"  // ID MQTT


const char* SSID = "Ranii";      // Rede Wi-Fi
const char* PASSWORD = "vitoriaa";  // Senha do Wi-Fi
const char* BROKER_MQTT = "test.mosquitto.org";
int BROKER_PORT = 1883;  // Porta do Broker MQTT


// Pinos dos sensores
#define PIN_SENSOR_CHUVA 15    //sensor de chuva
#define PIN_SENSOR_UMIDADE 36  //sensor umidade
#define TMP36_PIN 34           //sensor TMP36

// Variáveis e objetos globais
WiFiClient espClient;
PubSubClient MQTT(espClient);
TMP36 tempSensor(TMP36_PIN, 3.3);  // Instancia o TMP36 no pino 34 e 3.3V

/* Prototypes */
void initWiFi(void);
void initMQTT(void);
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT(void);
void reconnectWiFi(void);
void VerificaConexoesWiFIEMQTT(void);
void verificaChuva(void);
void leUmidade(void);
void leTemperatura(void);

/* Implementações */
void initWiFi(void) {
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  reconnectWiFi();
}

void initMQTT(void) {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(mqtt_callback);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.print("Chegou via MQTT: ");
  Serial.println(msg);

  if (msg.equals("L")) {
    digitalWrite(PIN_LED, HIGH);
    Serial.println("LED aceso");
  } else if (msg.equals("D")) {
    digitalWrite(PIN_LED, LOW);
    Serial.println("LED apagado");
  }
}

void reconnectMQTT(void) {
  while (!MQTT.connected()) {
    Serial.print("Tentando conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("Conectado ao broker MQTT!");
      MQTT.subscribe(TOPICO_SUBSCRIBE_LED);
    } else {
      Serial.println("Falha na conexão. Tentando novamente em 2s");
      delay(2000);
    }
  }
}

void VerificaConexoesWiFIEMQTT(void) {
  if (!MQTT.connected()) {
    reconnectMQTT();
  }
  reconnectWiFi();
}

void reconnectWiFi(void) {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Conectado ao Wi-Fi.");
  Serial.print("IP obtido: ");
  Serial.println(WiFi.localIP());
}

void verificaChuva(void) {
  int estadoChuva = digitalRead(PIN_SENSOR_CHUVA);
  if (estadoChuva == HIGH) {
    MQTT.publish(TOPICO_PUBLISH_CHUVA, "Sem Chuva");
    Serial.println("Publicando no MQTT: Sem Chuva");
  } else {
    MQTT.publish(TOPICO_PUBLISH_CHUVA, "Chuva");
    Serial.println("Publicando no MQTT: Chuva");
  }
}

void leUmidade(void) {
  int umidade = analogRead(PIN_SENSOR_UMIDADE); 
  char umidade_str[10];
  sprintf(umidade_str, "%d", umidade); 

  MQTT.publish(TOPICO_PUBLISH_UMIDADE, umidade_str);
  Serial.print("Publicando umidade: ");
  Serial.println(umidade_str);

}


void leTemperatura(void) {
  float temperatura = tempSensor.getTempC(); 

  char temperatura_str[10];
  sprintf(temperatura_str, "%.2f", temperatura); 

  MQTT.publish(TOPICO_PUBLISH_TEMPERATURA, temperatura_str);
  Serial.print("Publicando temperatura: ");
  Serial.println(temperatura_str);
}

void setup() {
  Serial.begin(9600); 
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SENSOR_CHUVA, INPUT); 

  reconnectWiFi(); // Conecta ao Wi-Fi
  initMQTT(); // Inicializa o MQTT
  reconnectMQTT(); // Conecta ao Broker MQTT

   pinMode(rele, OUTPUT);
  pinMode(sensor, INPUT); 

  digitalWrite(rele, HIGH);
}

void loop() {
  VerificaConexoesWiFIEMQTT(); // Verifica conexões Wi-Fi e MQTT

  verificaChuva(); // Lê o estado do sensor de chuva
  leUmidade(); // Lê a umidade
  leTemperatura(); // Lê a temperatura

  MQTT.loop(); // Mantém a conexão MQTT viva
  //millis(); // Espera 2 segundos antes de repetir

  irrigar = digitalRead(sensor);

  // Verificar e controlar o relé
  if (irrigar) {
    // Baixa umidade: ligar o motor
    digitalWrite(rele, LOW);  // Ativa o relé
    Serial.println("Baixa Umidade: Motor Ligado");

  } else {
    // Alta umidade: desligar o motor
    digitalWrite(rele, HIGH);  // Desativa o relé
    Serial.println("Alta Umidade: Motor Desligado");
  }

  millis();
}


