#define deviceName (const char *)"RollerBlind01"
#define deviceDesc (const char *)"Штора01"
const char *mqtt_server = "192.168.1.1";

#define STBY 16
#define PWMA 13
#define INA1 12
#define INA2 14
#define HALL_A 5
#define HALL_B 4

#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <FS.h>

ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

void reconnect();
void handleRoot();
void handleConfig();
void handleNotFound();
void callbackMQTT(char *topic, byte *payload, unsigned int length);
void sendCurrentPositionMQTT();
void sendTargetPositionMQTT();
void start();
void moveUp();
void moveDown();
void stop();

enum MotorState
{
  STOP,
  UP,
  DOWN
};

int blindsPosition;
int newPosition;

int hallEncoder;

String mqttTextIn;

MotorState motorState = STOP;
int upSpeed = 1024, downSpeed = 1024;
int maxCount;
bool savePosition;

void ICACHE_RAM_ATTR hallEncoder_A();
void ICACHE_RAM_ATTR hallEncoder_B();

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  SPIFFS.begin();

  EEPROM.begin(512);
  hallEncoder = EEPROM.read(100);
  blindsPosition = EEPROM.read(101);
  newPosition = blindsPosition;
  maxCount = EEPROM.read(501);
  upSpeed = EEPROM.read(503);
  downSpeed = EEPROM.read(505);

  if (maxCount == 0)
    maxCount = 3;

  pinMode(STBY, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(INA1, OUTPUT);
  pinMode(INA2, OUTPUT);

  digitalWrite(STBY, LOW);
  digitalWrite(INA1, LOW);
  digitalWrite(INA2, LOW);

  pinMode(HALL_A, INPUT_PULLUP);
  pinMode(HALL_B, INPUT_PULLUP);

  switch (hallEncoder)
  {
  case 1:
    attachInterrupt(digitalPinToInterrupt(HALL_B), hallEncoder_B, FALLING);
    break;
  case 2:
    attachInterrupt(digitalPinToInterrupt(HALL_A), hallEncoder_A, FALLING);
    break;
  default:
    attachInterrupt(digitalPinToInterrupt(HALL_A), hallEncoder_A, FALLING);
    attachInterrupt(digitalPinToInterrupt(HALL_B), hallEncoder_B, FALLING);
  }

  // Подключение к точке доступа или запуск втроенной при необходимости - WiFiManager
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180);

  wifiManager.autoConnect(deviceName);
  Serial.println("WiFi Manager connected.");

  // Обновление по воздуху - ArduinoOTA
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(deviceName);
  ArduinoOTA.onStart([]() {
    Serial.println("Begin flashing.");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd flashing.");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed.");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed.");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed.");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed.");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed.");
  });
  ArduinoOTA.begin();
  Serial.print("ArduinoOTA ready as ");
  Serial.print(ArduinoOTA.getHostname());
  Serial.println(".");

  // Обработка запросов HTTP-сервера
  server.on("/", handleRoot);
  server.on("/config.html", handleConfig);
  server.on("/jquery.js", handleJQuery);
  server.onNotFound(handleNotFound);

  server.on("/moveup", []() {
    newPosition = blindsPosition - 1;
    moveUp();
    server.send(200, "text/plain", "moveUp:OK!");
  });

  server.on("/movedown", []() {
    newPosition = blindsPosition + 1;
    moveDown();
    server.send(200, "text/plain", "moveDown:OK!");
  });

  server.on("/login", []() {
    if (server.hasArg("login") && server.hasArg("pass"))
    {
      if (server.arg("login") == "admin" && server.arg("pass") == "Germ1E")
      {
        server.send(200, "text/plain", "Auth OK");
        return;
      }
    }
    server.send(200, "text/plain", "Auth failed");
  });

  server.on("/setmaxcount", []() {
    if (server.hasArg("val"))
    {
      maxCount = server.arg("val").toInt();
      EEPROM.write(501, maxCount);
      EEPROM.commit();
    }
    else
    {
      maxCount = blindsPosition;
    }
    server.send(200, "text/plain", "setMaxCount:OK!");
  });

  server.on("/setupspeed", []() {
    if (server.hasArg("val"))
    {
      upSpeed = server.arg("val").toInt();
      if (upSpeed < 100)
        upSpeed = 100;
      if (upSpeed > 1024)
        upSpeed = 1024;
      EEPROM.write(503, upSpeed);
      EEPROM.commit();
      server.send(200, "text/plain", "setUpSpeed:OK!");
    }
    else
    {
      server.send(200, "text/plain", "No ?val= parameter!");
    }
  });

  server.on("/setdownspeed", []() {
    if (server.hasArg("val"))
    {
      downSpeed = server.arg("val").toInt();
      if (downSpeed < 100)
        downSpeed = 100;
      if (downSpeed > 1024)
        downSpeed = 1024;
      EEPROM.write(505, downSpeed);
      EEPROM.commit();
      server.send(200, "text/plain", "setDownSpeed:OK!");
    }
    else
    {
      server.send(200, "text/plain", "No ?val= parameter!");
    }
  });

  server.on("/stop", []() {
    stop();
    server.send(200, "text/plain", "stop:OK!");
  });

  server.on("/moveto", []() {
    if (server.hasArg("val"))
    {
      stop();
      int v = server.arg("val").toInt();

      v = 100 - v;
      if (v * maxCount / 100 != newPosition)
      {
        newPosition = v * maxCount / 100;
        Serial.println("newPulse:" + (String)newPosition);
        start();
      }
      server.send(200, "text/plain", "moveTo:OK!");
    }
    else
    {
      server.send(200, "text/plain", "No ?val= parameter!");
    }
  });

  server.on("/setdefault", []() {
    blindsPosition = 50;
    newPosition = 50;
    EEPROM.write(100, hallEncoder);
    EEPROM.write(101, blindsPosition);
    EEPROM.write(503, downSpeed);
    EEPROM.write(505, downSpeed);
    EEPROM.commit();
    server.send(200, "text/plain", "setDefault:OK!");
  });

  server.on("/reset", []() {
    server.send(200, "text/plain", "reset:OK!: ");
    ESP.restart();
  });

  server.on("/status", []() {
    StaticJsonDocument<256> root;

    root["HALL_A"] = (String)digitalRead(HALL_A);
    root["HALL_B"] = (String)digitalRead(HALL_B);

    root["hallEncoder"] = (String)hallEncoder;

    root["isMoving"] = (String)motorState;

    root["maxCount"] = (String)maxCount;
    root["upSpeed"] = (String)upSpeed;
    root["downSpeed"] = (String)downSpeed;

    root["blindsPosition"] = (String)blindsPosition;
    root["newPosition"] = (String)newPosition;

    String json;
    serializeJson(root, json);

    server.send(200, "application/json", json);
  });

  server.on("/mqttStatus", []() {
    server.send(200, "application/json", mqttTextIn);
  });

  server.begin();
  Serial.println("HTTP server started!");
  Serial.println("");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callbackMQTT);
}

void loop()
{
  ArduinoOTA.handle();

  server.handleClient();

  if (!client.connected())
  {
    reconnect();
  }
  else
    client.loop();

  if (savePosition)
  {
    savePosition = false;
    EEPROM.write(100, hallEncoder);
    EEPROM.write(101, blindsPosition);
    EEPROM.commit();
    sendCurrentPositionMQTT();
    sendTargetPositionMQTT();
  }
}

void ICACHE_RAM_ATTR hallEncoder_A()
{
  detachInterrupt(digitalPinToInterrupt(HALL_A));
  attachInterrupt(digitalPinToInterrupt(HALL_B), hallEncoder_B, FALLING);

  hallEncoder = 1;

  if (motorState == DOWN)
  {
    blindsPosition++;
    if (blindsPosition >= maxCount)
      stop();
  }
  else if (motorState == UP)
  {
    blindsPosition--;
    if (blindsPosition <= 0)
      stop();
  }
}

void ICACHE_RAM_ATTR hallEncoder_B()
{
  detachInterrupt(digitalPinToInterrupt(HALL_B));
  attachInterrupt(digitalPinToInterrupt(HALL_A), hallEncoder_A, FALLING);

  hallEncoder = 2;

  if (motorState == DOWN)
  {
    blindsPosition++;
    if (blindsPosition >= newPosition || blindsPosition >= maxCount)
      stop();
  }
  else if (motorState == UP)
  {
    blindsPosition--;
    if (blindsPosition <= newPosition || blindsPosition <= 0)
      stop();
  }
}

void start()
{
  if (newPosition == blindsPosition || newPosition < 0 || newPosition > maxCount)
    return;

  if (newPosition > blindsPosition)
  {
    moveDown();
  }
  else if (newPosition < blindsPosition)
  {
    moveUp();
  }
}

void moveUp()
{
  motorState = UP;
  digitalWrite(INA1, HIGH);
  digitalWrite(INA2, LOW);
  analogWrite(PWMA, upSpeed);
  digitalWrite(STBY, HIGH);
}

void moveDown()
{
  motorState = DOWN;
  digitalWrite(INA1, LOW);
  digitalWrite(INA2, HIGH);
  analogWrite(PWMA, downSpeed);
  digitalWrite(STBY, HIGH);
}

void stop()
{
  motorState = STOP;
  digitalWrite(STBY, LOW);
  digitalWrite(INA1, LOW);
  digitalWrite(INA2, LOW);

  savePosition = true;
}

int previousMillisReconnect;

void reconnect()
{
  if (millis() - previousMillisReconnect < 10000)
    return;

  Serial.println("Attempting MQTT connection...");
  if (client.connect(deviceName))
  {
    Serial.println("MQTT connected.");

    client.publish("homebridge/to/add", "{\"name\":\"shtori_1\",\"service_name\":\"Штора 1\",\"service\":\"WindowCovering\"}");
    client.publish("homebridge/to/set/accessoryinformation", "{\"name\": \"shtori_1\", \"manufacturer\": \"espressif\", \"model\": \"esp12e\", \"serialnumber\": \"1113\"}");

    sendCurrentPositionMQTT();
    sendTargetPositionMQTT();

    client.subscribe("homebridge/from/set");
  }
  else
  {
    Serial.print("Connection to MQTT failed, rc=");
    Serial.print(client.state());
    Serial.println(".");
  }

  previousMillisReconnect = millis();
}

void sendCurrentPositionMQTT()
{
  StaticJsonDocument<200> root;

  root["name"] = deviceName;
  root["service_name"] = "Штора 1";
  root["characteristic"] = "CurrentPosition";
  root["value"] = (int)100 - (blindsPosition * 100 / maxCount);

  char json[200];
  serializeJson(root, json);

  if (client.publish("homebridge/to/set", json))
    Serial.println("Отправлено:" + (String)json);
}

void sendTargetPositionMQTT()
{
  StaticJsonDocument<200> root;

  root["name"] = deviceName;
  root["service_name"] = "Штора 1";
  root["characteristic"] = "TargetPosition";
  root["value"] = (int)100 - (blindsPosition * 100 / maxCount);

  char json[200];
  serializeJson(root, json);

  if (client.publish("homebridge/to/set", json))
    Serial.println("Отправлено:" + (String)json);
}

void callbackMQTT(char *topic, byte *payload, unsigned int length)
{
  String json;

  Serial.print("MQTT message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    json += (char)payload[i];
  }
  Serial.println();

  mqttTextIn = (String)topic + " " + json;

  StaticJsonDocument<200> root;

  DeserializationError error = deserializeJson(root, json);

  if (error)
  {
    Serial.println("Deserialization failed.");
    return;
  }

  String nameSubscribe = root["name"];
  String characteristic = root["characteristic"];
  int v = root["value"];

  if (nameSubscribe == deviceName && characteristic == "TargetPosition")
  {
    v = 100 - v;
    if (v * maxCount / 100 != newPosition)
    {
      newPosition = v * maxCount / 100;
      Serial.println("newPulse:" + (String)newPosition);
      start();
    }
  }
}

void handleRoot()
{
  File file = SPIFFS.open("/index.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void handleConfig()
{
  File file = SPIFFS.open("/config.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void handleJQuery()
{
  File file = SPIFFS.open("/jquery.js", "r");
  server.streamFile(file, "application/javascript");
  file.close();
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);

  Serial.println(message);
}
