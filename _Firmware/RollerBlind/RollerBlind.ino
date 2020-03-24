#define deviceName (const char *)"RollerBlind01"
#define deviceDesc (const char *)"Штора01"
const char *mqtt_server = "192.168.1.1";

#define STBY 16
#define PWMA 13
#define INA1 12
#define INA2 14
#define HALL_A 5
#define HALL_B 4

char mainPage[] PROGMEM = R"=====(
  
<!DOCTYPE html>
<html>
	<head>
		<meta charset="utf-8" />
		<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no" />
		<title>RollerBlind01 - Main Menu</title>

		<script src="http://192.168.1.1/jquery-3.4.1.min.js"></script>
		<style>
			body {
				text-align: center;
				font-family: verdana, sans-serif;
				background: #ffffff;
				user-select: none;
			}

			div {
				font-size: 1em;
				padding: 5px;
			}

			#container {
				text-align: center;
				display: inline-block;
				color: #000000;
				min-width: 340px;
			}

			table {
				border-spacing: 1em 0;
				width: 100%;
			}

			td {
				padding: 0px;
			}

			button {
				border: 0;
				margin: 5px;
				border-radius: 0.3rem;
				background: #1fa3ec;
				color: #ffffff;
				line-height: 3.2rem;
				font-size: 1.2rem;
				transition: 0.4s;
				width: 100%;
				outline: none;
			}

			button:hover {
				background: #0e70a4;
			}

			#stop {
				background: #d43535;
			}

			#stop:hover {
				background: #931f1f;
			}

			#range {
				-webkit-appearance: none;
				appearance: none;
				background: #1fa3ec;
				outline: none;
				border-radius: 0.6rem;
				height: 24px;
				width: 60%;
				margin: 0;
			}

			#range::-webkit-slider-thumb {
				-webkit-appearance: none;
				appearance: none;
				width: 18px;
				height: 40px;
				background: #1fa3ec;
				border: 2px solid black;
				border-radius: 0.4rem;
			}

			#range::-webkit-slider-thumb:hover {
				background: #0e70a4;
			}

			.triangle {
				font-size: 2em;
			}

			footer {
				font-size: 11px;
				margin: 5px 30px;
			}
		</style>

		<script>
			$(() => {
				let changeTimer;
				readStatus();

				$("#manualUp, #open").bind("mousedown touchstart", () => {
					$("#stop").trigger("mousedown");
					$("#range").prop("disabled", true);
					$("#range").css("background", "#931f1f");
					$.ajax({
						type: "POST",
						url: "moveup"
					});
					changeTimer = setInterval(readStatus, 500);
				});

				$("#manualDn, #close").bind("mousedown touchstart", () => {
					$("#stop").trigger("mousedown");
					$("#range").prop("disabled", true);
					$("#range").css("background", "#931f1f");
					$.ajax({
						type: "POST",
						url: "movedown"
					});
					changeTimer = setInterval(readStatus, 500);
				});

				$("#manualUp, #manualDn").bind("mouseup touchend", () => {
					$("#stop").trigger("mousedown");
				});

				$("#range").bind("mousedown touchstart", () => {
					$("#stop").trigger("mousedown");
					clearInterval(changeTimer);
				});

				$("#range").change(() => {
					$("#stop").trigger("mousedown");
					$.ajax({
						type: "POST",
						url: "moveto",
						data: {
							val: $("#range").val() * 10
						}
					});
					changeTimer = setInterval(readStatus, 500);
				});

				$("#stop").bind("mousedown touchstart", () => {
					$("#range").prop("disabled", false);
					$("#range").css("background", "#1fa3ec");
					clearInterval(changeTimer);
					$.ajax({
						type: "POST",
						url: "stop"
					});
					readStatus();
				});

				function readStatus() {
					$.ajax({
						type: "POST",
						url: "status",
						success: answer => {
							$("#range").val(answer.blindsPosition / 10);
						}
					});
				}
			});
		</script>
	</head>

	<body>
		<div id="container">
			<h3>Roller Blind Module</h3>
			<h2>RollerBlind01</h2>

			<table>
				<tbody>
					<tr>
						<td style="width:35%"><button id="manualUp">Man. UP</button></td>
						<td style="width:65%"><button id="open">Open</button></td>
					</tr>
					<tr>
						<td style="width:35%"><button id="manualDn">Man. DN</button></td>
						<td style="width:65%">
							<span class="triangle">&#9660;</span>
							<input type="range" id="range" min="0" max="10" />
							<span class="triangle" style="color: #FFE000">&#9650;</span>
						</td>
					</tr>
					<tr>
						<td style="width:35%"><button id="stop">Stop</button></td>
						<td style="width:65%"><button id="close">Close</button></td>
					</tr>
				</tbody>
			</table>

			<footer>
				<hr />
				<div style="float: left">
					<a href="config.html" style="text-decoration: none;">Config</a>
				</div>
				<div style="text-align: right;  color: #aaa;">
					Home automation for MajorDoMo
				</div>
			</footer>
		</div>
	</body>
</html>

)=====";

char configPage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
	<head>
		<meta charset="utf-8" />
		<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no" />
		<title>RollerBlind01 - Config Menu</title>

		<script src="http://192.168.1.1/jquery-3.4.1.min.js"></script>

		<style>
			* {
				box-sizing: border-box;
			}

			body {
				text-align: center;
				font-family: verdana, sans-serif;
				background: #ffffff;
				user-select: none;
			}

			#container {
				text-align: center;
				display: inline-block;
				color: #000000;
				min-width: 340px;
			}

			table {
				border-spacing: 1em 0;
				width: 100%;
			}

			td {
				padding: 0px;
				width: 50%;
			}

			button {
				border: 0;
				margin: 5px 0;
				border-radius: 0.3rem;
				background: #1fa3ec;
				color: #ffffff;
				line-height: 3.2rem;
				font-size: 1.2rem;
				transition: 0.4s;
				width: 100%;
				outline: none;
				padding: 0;
			}

			button:hover {
				background: #0e70a4;
			}

			#config {
				display: none;
			}

			#reset {
				background: #d43535;
			}

			input[type="number"] {
				line-height: 3rem;
				font-size: 2.4rem;
				width: 100%;
				border-radius: 0.3rem;
				max-width: 140px;
				text-align: center;
			}

			footer {
				font-size: 11px;
				margin: 5px 30px;
			}
		</style>

		<script>
			$(() => {
				$("#enter").click(() => {
					$.ajax({
						type: "POST",
						url: "login",
						data: { login: $("#login").val(), pass: $("#password").val() },
						success: answer => {
							if (answer == "Auth OK") {
								readStatus();
								$("#config").slideDown(400);
								$("#passw").slideUp(400);
							} else {
								$("#login").val("");
								$("#password").val("");
							}
						}
					});
				});

				$("#login,#password").keyup(event => {
					if (event.keyCode == 13) {
						$("#enter").click();
					}
				});

				function readStatus() {
					$.ajax({
						type: "POST",
						url: "status",
						success: answer => {
							$("#maxCount").val(answer.maxCount);
							$("#upSpeed").val(answer.upSpeed);
							$("#downSpeed").val(answer.downSpeed);
						}
					});
				}
			});
		</script>
	</head>

	<body>
		<div id="container">
			<div id="passw">
				<h4 style="text-align: left; float: left; margin: 15px;">Login:</h4>
				<div style="text-align: right;">
					<input type="text" id="login" style="margin: 5px;" />
				</div>
				<h4 style="text-align: left; float: left; margin: 15px;">Password:</h4>
				<div style="text-align: right;">
					<input type="password" id="password" style="margin: 5px;" />
				</div>
				<button id="enter">Enter</button>
			</div>

			<div id="config">
				<h3>Roller Blind Module</h3>
				<h2>RollerBlind01</h2>
				<h1>Config</h1>

				<table>
					<tbody>
						<tr>
							<td>
								<input id="maxCount" type="number" min="0" value="0" />
							</td>
							<td><button id="setMaxCount">Set max count</button></td>
						</tr>
						<tr>
							<td>
								<input id="upSpeed" type="number" min="100" max="1000" step="50" value="1000" />
							</td>
							<td><button id="setUpSpeed">Set UP Speed</button></td>
						</tr>
						<tr>
							<td>
								<input id="downSpeed" type="number" min="100" max="1000" step="50" value="1000" />
							</td>
							<td><button id="setDnSpeed">Set DN Speed</button></td>
						</tr>
						<tr>
							<td colspan="2"><button id="setDefault">Set default</button></td>
						</tr>
						<tr>
							<td colspan="2"><button id="reset">Reset</button></td>
						</tr>
					</tbody>
				</table>
			</div>
			<footer>
				<hr />
				<div style="float: left">
					<a href="/" style="text-decoration: none;">Main</a>
				</div>
				<div style="text-align: right;  color: #aaa;">
					Home automation for MajorDoMo
				</div>
			</footer>
		</div>
	</body>
</html>

)=====";

#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

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
  while (!Serial)
    ;

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
  server.send_P(200, "text/html", mainPage);
}

void handleConfig()
{
  server.send_P(200, "text/html", configPage);
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
