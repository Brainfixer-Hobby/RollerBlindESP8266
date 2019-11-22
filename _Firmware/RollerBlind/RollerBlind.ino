#define deviceName (const char *)"RollerBlind01"
const char* mqtt_server = "192.168.0.252";

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

ESP8266WebServer server (80);
WiFiClient espClient;
PubSubClient client(espClient);

enum MotorState {
  UP,
  DOWN,
  STOP
};

int lastBlindsPosition;
int blindsPosition;
int newPosition;

int hallEncoder;

String mqttTextIn;

MotorState motorState = STOP;
long upSpeed = 1024, downSpeed = 1024;
int maxCount;
bool savePosition;

void ICACHE_RAM_ATTR hallEncoder_A();
void ICACHE_RAM_ATTR hallEncoder_B();

void setup() {
  Serial.begin(115200);
  while (!Serial);

  EEPROM.begin(512);
  hallEncoder = EEPROM.read(100);
  blindsPosition = EEPROM.read(101);
  newPosition = blindsPosition;
  maxCount = EEPROM.read(501);

  if (maxCount == 0)maxCount = 3;

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
    default:  {
        attachInterrupt(digitalPinToInterrupt(HALL_A), hallEncoder_A, FALLING);
        attachInterrupt(digitalPinToInterrupt(HALL_B), hallEncoder_B, FALLING);
      }
  }

  WiFiManager wifiManager;

  wifiManager.setConfigPortalTimeout(180);

  wifiManager.autoConnect(deviceName);

  Serial.println("WiFi Manager connected.");


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
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed.");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed.");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed.");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed.");
    else if (error == OTA_END_ERROR) Serial.println("End Failed.");
  });

  ArduinoOTA.begin();

  Serial.print("ArduinoOTA ready as ");
  Serial.print(ArduinoOTA.getHostname());
  Serial.println(".");
  
  server.onNotFound(handleNotFound);
  server.on("/", handleRoot);

  server.on("/moveUp", []() {
    newPosition = blindsPosition - 1;
    moveUp();
    server.send(200, "text/plain", "moveUp:OK!");
  });

  server.on("/moveDown", []() {
    newPosition = blindsPosition + 1;
    moveDown();
    server.send(200, "text/plain", "moveDown:OK!");
  });

  server.on("/setMaxCount", []() {
    maxCount = server.arg("state").toInt();
    EEPROM.write(501, maxCount);
    EEPROM.commit();
    server.send(200, "text/plain", "setMaxCount:OK!");
  });

  server.on("/setUpSpeed", []() {
    upSpeed = server.arg("state").toInt();
    server.send(200, "text/plain", "setUpSpeed:OK!");
  });

  server.on("/setDownSpeed", []() {
    downSpeed = server.arg("state").toInt();
    server.send(200, "text/plain", "setDownSpeed:OK!");
  });

  server.on("/stop", []() {
    stop();
    server.send(200, "text/plain", "stop:OK!");
  });

  server.on("/setDefault", []() {
    blindsPosition = 0;
    newPosition = 0;
    EEPROM.write(100, hallEncoder);
    EEPROM.write(101, blindsPosition);
    EEPROM.commit();
    server.send(200, "text/plain", "setDefault:OK!");
  });

  server.on("/newPulse", []() {
    server.send(200, "text/plain", "newPulse: ");
  });

  server.on("/reset", []() {
    server.send(200, "text/plain", "reset:OK!: ");
    ESP.restart();
  });

  server.on("/status", []() {
    StaticJsonDocument<256> root;

    root["HALL_A:"] = (String)digitalRead(HALL_A);
    root["HALL_B:"] = (String)digitalRead(HALL_B);

    root["blindsPosition:"] = (String)blindsPosition;
    root["maxCount:"] = (String)maxCount;
    root["newPosition:"] = (String)newPosition;
    root["hallEncoder:"] = (String)hallEncoder;

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
  client.setCallback(callback);
}

void loop() {
  ArduinoOTA.handle();

  server.handleClient();

  if (!client.connected()) {
    reconnect();
  }
  else
    client.loop();

  if (savePosition) {
    savePosition = false;
    EEPROM.write(100, hallEncoder);
    EEPROM.write(101, blindsPosition);
    EEPROM.commit();
    sendMQTTCurrentPositionBuild();
    sendMQTTTargetPositionBuild();
  }
}

void ICACHE_RAM_ATTR hallEncoder_A() {
  detachInterrupt(digitalPinToInterrupt(HALL_A));
  attachInterrupt(digitalPinToInterrupt(HALL_B), hallEncoder_B, FALLING);

  hallEncoder = 1;

  if (motorState == DOWN) {
    blindsPosition++;
    if (blindsPosition >= maxCount) stop();
  }
  else if (motorState == UP) {
    blindsPosition--;
    if (blindsPosition <= 0) stop();
  }
}

void ICACHE_RAM_ATTR hallEncoder_B() {
  detachInterrupt(digitalPinToInterrupt(HALL_B));
  attachInterrupt(digitalPinToInterrupt(HALL_A), hallEncoder_A, FALLING);

  hallEncoder = 2;

  if (!motorState) {
    blindsPosition++;
    if (blindsPosition >= newPosition || blindsPosition >= maxCount) stop();
  }
  else {
    blindsPosition--;
    if (blindsPosition <= newPosition || blindsPosition <= 0) stop();
  }

}

void start() {
  if (newPosition == blindsPosition || newPosition < 0 || newPosition > maxCount) return;

  if (newPosition > blindsPosition) {
    moveDown();
  }
  else if (newPosition < blindsPosition) {
    moveUp();
  }
}

void moveUp() {
  motorState = UP;
  digitalWrite(INA1, HIGH);
  digitalWrite(INA2, LOW);
  analogWrite(PWMA, upSpeed);
  digitalWrite(STBY, HIGH);
}

void moveDown() {
  motorState = DOWN;
  digitalWrite(INA1, LOW);
  digitalWrite(INA2, HIGH);
  analogWrite(PWMA, downSpeed);
  digitalWrite(STBY, HIGH);

}

void stop() {
  motorState = STOP;
  digitalWrite(STBY, LOW);
  digitalWrite(INA1, LOW);
  digitalWrite(INA2, LOW);

  savePosition = true;
}

int previousMillisReconnect;

void reconnect() {
  if (millis() - previousMillisReconnect < 10000)return;

  Serial.println("Attempting MQTT connection...");
  if (client.connect("RollerBlind01")) {
    Serial.println("MQTT connected.");

    client.publish("homebridge/to/add", "{\"name\":\"shtori_1\",\"service_name\":\"Штора 1\",\"service\":\"WindowCovering\"}");
    client.publish("homebridge/to/set/accessoryinformation", "{\"name\": \"shtori_1\", \"manufacturer\": \"espressif\", \"model\": \"esp12e\", \"serialnumber\": \"1113\"}");

    sendMQTTCurrentPositionBuild();
    sendMQTTTargetPositionBuild();

    client.subscribe("homebridge/from/set");
  }
  else {
    Serial.print("Connection to MQTt failed, rc=");
    Serial.print(client.state());
    Serial.println(".");
  }

  previousMillisReconnect = millis();
}

void sendMQTTCurrentPositionBuild() {
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

void sendMQTTTargetPositionBuild() {

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

void callback(char* topic, byte* payload, unsigned int length) {
  String json;

  Serial.print("MQTT message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    json += (char)payload[i];
  }
  Serial.println();

  mqttTextIn = (String)topic + " " + json;

  StaticJsonDocument<200> root;
  
  DeserializationError error = deserializeJson(root, json);
  
  if (error) {
    Serial.println("Deserialization failed.");
    return;
  }
  
  String nameSubscribe = root["name"];
  String characteristic = root["characteristic"];
  int v = root["value"];

  if (nameSubscribe == deviceName && characteristic == "TargetPosition") {
    v = 100 - v;
    if (v != newPosition) {
      newPosition = v * maxCount / 100;
      Serial.println("newPulse:" + (String)newPosition);
      start();
    }
  }
}

void handleRoot() {
  
  const char webpage[] = R"=====(
<html>
  <head>
    <script src="http://ajax.googleapis.com/ajax/libs/jquery/1.11.2/jquery.min.js"></script>

    <script type="text/javascript">
      $(document).ready(function() {
        $("#up").click(function() {
          $.ajax({
            type: 'POST',
            url: 'moveUp',
            success: function(data) {
              $("p").text(data);
            },
            error: function(result) {
              alert('error');
            }
          });
          });

        $("#down").click(function() {
          $.ajax({
            type: 'POST',
            url: 'moveDown',
            success: function(data) {
              $("p").text(data);
            },
            error: function(result) {
              alert('error');
            }
          });
          });

        $("#stop").click(function() {
          $.ajax({
            type: 'POST',
            url: 'stop',
            success: function(data) {
              $("p").text(data);
            },
            error: function(result) {
              alert('error');
            }
          });
          });
      });
    </script>
  </head>
  
  <body>
    <button type="button" name="up" id="up">Up</button>
    <button type="button" name="down" id="down">Down</button>
    <button type="button" name="stop" id="stop">Stop</button>
    <p></p>
  <body>
<html>
  )=====";
  
  //" Без этой кавычки код не компилируется после вставки raw string literal выше:)

  server.send_P (200, "text/html", webpage);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);

  Serial.println(message);
}
