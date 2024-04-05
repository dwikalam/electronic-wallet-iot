#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#define BUTTON 0
#define LED 2

const char* SSID = "xxx"; // YOUR WIFI SSID
const char* SSID_PW = "xxx"; // YOUR WIFI PASSWORD

const char* MQTT_BROKER = "xxx"; // YOUR MQTT BROKER IP ADDRESS
const int MQTT_PORT = 1883;
const char* MQTT_USER = "xxx"; // YOUR MQTT USER
const char* MQTT_PW = "xxx"; // YOUR MQTT PASSWORD

const char* PUBLISH_TOPIC = "esp32/electronic-wallet";
const char* BASE_INITIAL_BALANCE_MESSAGE = "SALDO AWAL Rp";
const char* BASE_TRANSACTION_SUCCESS_MESSAGE = "TRANSAKSI BERHASIL, SISA SALDO Rp";
const char* TRANSACTION_FAILED_MESSAGE = "SALDO TIDAK MENCUKUPI";

const int PAYMENT_PROCESS_INTERVAL = 5000;

const int LED_BLINK_INTERVAL = 500;

const int DEBOUNCE_DELAY = 50; 

WiFiClient espClient;
PubSubClient client(espClient);

unsigned int balance = (210 + 100) * 1000;

bool isPaymentProcessingOnGoing = false;
bool isTransactionSuccess = false;

bool previousLedState = LOW;

bool buttonState;        
bool previousButtonState = LOW;

unsigned long currentMillis = 0;
unsigned long previousPaymentMillis = 0;
unsigned long previousLedMillis = 0;

unsigned long previousDebounceTime = 0; 

void setup() {
  Serial.begin(115200);

  setPins();

  connectToWiFi();

  connectToMqttBroker();

  publish((String(BASE_INITIAL_BALANCE_MESSAGE) + String(balance)).c_str());
}

void loop() {
  currentMillis = millis();

  if (isPaymentButtonPressed()) {
    processPayment();

    isPaymentProcessingOnGoing = true;

    previousPaymentMillis = currentMillis;

    return;
  }

  if (isPaymentProcessingOnGoing) {
    if (! hasPaymentProcessingPassedInterval()) {
      updateLedState();

      return;  
    }

    isPaymentProcessingOnGoing = false;
  
    ledOff();

    Serial.println("T/O: LED OFF");
  }
}

void setPins() {
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
}

void connectToWiFi() {
  Serial.print("Connecting to '" + String(SSID) + "'...");

  WiFi.begin(SSID, SSID_PW);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected. (Local IP: " + String(WiFi.localIP()) + ")\n");
}

void connectToMqttBroker() {
  String clientId = "esp32-client-" + String(WiFi.macAddress());
  
  client.setServer(MQTT_BROKER, MQTT_PORT);

  bool isConnected = client.connect(clientId.c_str(), MQTT_USER, MQTT_PW);

  while (! isConnected) {
    Serial.println("Failed to connect to MQTT broker. State: " + String(client.state()));

    Serial.println("Reconnecting...");

    delay(2000);

    isConnected = client.connect(clientId.c_str(), MQTT_USER, MQTT_PW);
  }  

  Serial.println("Connected to MQTT broker.");
}

void processPayment() {
  if (! isBalanceSufficient()) {
    publish(TRANSACTION_FAILED_MESSAGE);

    isTransactionSuccess = false;

    ledOn();

    Serial.println("T/F: LED ON");

    return;
  }

  balance -= 20000;
  publish((String(BASE_TRANSACTION_SUCCESS_MESSAGE) + String(balance)).c_str());

  isTransactionSuccess = true;

  ledOn();

  return;
}

void updateLedState() {
  if (isTransactionSuccess) {
    Serial.println("T/S: LED ON");

    return;
  }

  if (hasBlinkLedPassedInterval()) {
    previousLedMillis = currentMillis;

    setLedState(! previousLedState);
  }
}

void setLedState(bool level) {
  digitalWrite(LED, level);

  previousLedState = level;

  String terminalOut = level
    ? "T/F: LED ON"
    : "T/F: LED OFF";

  Serial.println(terminalOut);
}

void ledOn() {
  digitalWrite(LED, HIGH);
}

void ledOff() {
  digitalWrite(LED, LOW);
}


void publish(const char* message) {
  client.publish(PUBLISH_TOPIC, message);
}

bool isBalanceSufficient() {
  return balance >= 20000;
}

bool hasPaymentProcessingPassedInterval() {
  return (currentMillis - previousPaymentMillis) >= PAYMENT_PROCESS_INTERVAL;
}

bool hasBlinkLedPassedInterval() {
  return (currentMillis - previousLedMillis) >= LED_BLINK_INTERVAL;
}

bool isPaymentButtonPressed() {
  int reading = digitalRead(BUTTON);

  if (reading != previousButtonState) {
    previousDebounceTime = millis();
  }

  previousButtonState = reading;

  if ((millis() - previousDebounceTime) <= DEBOUNCE_DELAY) {
    return false;
  }

  if (reading == buttonState) {
    return false;
  }

  buttonState = reading;

  return buttonState == LOW;
}