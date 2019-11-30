/*
  robot-esp - Standalone firmware for robot based on ESP family; ESP32 or ESP8266
  -Control by wifi with a simple HTML page
  -Obstacle detection through sonar
  -Configurable speed
  
  Copyright: (c)Florian ROBERT
  
  This file is part of robot-esp.
  
  robot-esp is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  robot-esp is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <NewPing.h>

#define WEBSERVER_PORT 80
#ifdef ESP32
  #include <WiFi.h>
  #include <WebServer.h>
  WebServer server(WEBSERVER_PORT);
#elif ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  ESP8266WebServer server(WEBSERVER_PORT);
#endif

// Define below the name and the password of the access point that the ESP will generate so as to control the robot
const char* ssid = "robot-esp";  // Enter SSID here
const char* password = "";  //Enter Password here

// Define below the different pins connected to L298N and SONAR
#ifdef ESP32
  #define IN1 26
  #define IN2 27
  #define ENA 14
  #define IN3 32
  #define IN4 33
  #define ENB 25
  #define TRG 19
  #define ECH 18
  //PWM properties for ESP32
  const int freq = 30000;
  const int pwmChannel1 = 1;
  const int pwmChannel2 = 2;
  const int resolution = 8;
#else //ESP8266
  #define IN1 D1
  #define IN2 D2
  #define ENA D2
  #define IN3 D3
  #define IN4 D4
  #define ENB D6
  #define TRG D7
  #define ECH D8
#endif

long duration;
float distance;

int powerRight = 0; // Power of the right motor
int powerLeft = 0; // Power of the left motor

int robotSpeed = 0;

NewPing sonar(TRG, ECH);

void setup()
{

  Serial.begin(115200);
  Serial.println("Init pins");
  pinMode (ENA, OUTPUT);
  pinMode (IN1, OUTPUT);
  pinMode (IN2, OUTPUT);
  pinMode (ENB, OUTPUT);
  pinMode (IN3, OUTPUT);
  pinMode (IN4, OUTPUT);

  #ifdef ESP32
    ledcSetup(pwmChannel1, freq, resolution);  
    ledcSetup(pwmChannel2, freq, resolution); 
    ledcAttachPin(ENA, pwmChannel1);
    ledcAttachPin(ENB, pwmChannel2);
  #endif
  
  Serial.println("Start AP");
  WiFi.softAP(ssid, password);
  delay(100);

  server.on("/", handle_OnConnect);
  server.on("/robotfwd", handle_robotfwd);
  server.on("/robotoff", handle_robotoff);
  server.on("/robotrear", handle_robotrear);
  server.on("/robotr", handle_robotr);
  server.on("/robotl", handle_robotl);
  server.on("/robothigh", handle_robothigh);
  server.on("/robotmid", handle_robotmid);
  server.on("/robotlow", handle_robotlow);
  server.onNotFound(handle_NotFound);
  
  server.begin();
  
  Serial.println("HTTP server started");
}
/*__________________________________________________________*/

void loop()
{

  server.handleClient();

  // Power the motors following according left and right power
  power(powerLeft,powerRight);

  // Measure distance with Sonar
  int distance = sonar.ping_cm();

  // If distance is < X , stop, go backward and turn
  if(distance > 0 && distance < 30){
    avoidance();
  }
}

void avoidance(){
    Serial.print("Avoidance sequence");
    power(0,0);
    delay(50);
    power(-180,-180);
    delay(500);
    power(-210,210);
    delay(500);
}

void power(int Left, int Right){
  if(Left > 0){
    digitalWrite (IN3, LOW);
    digitalWrite (IN4, HIGH);
  }else if( Left == 0){
    digitalWrite (IN3, LOW);
    digitalWrite (IN4, LOW);
  }else if ( Left < 0) {
    digitalWrite (IN3, HIGH);
    digitalWrite (IN4, LOW);
  }

  if(Right > 0){
    digitalWrite (IN1, LOW);
    digitalWrite (IN2, HIGH);
  }else if( Right == 0){;
    digitalWrite (IN1, LOW);
    digitalWrite (IN2, LOW);
  }else if ( Right < 0) {
    digitalWrite (IN1, HIGH);
    digitalWrite (IN2, LOW);
  }
  
  #ifdef ESP32
    ledcWrite(pwmChannel1, abs(Right));   
    ledcWrite(pwmChannel2, abs(Left));
  #else //ESP8266
    analogWrite(ENA, abs(Right));
    analogWrite(ENB, abs(Left));
  #endif

}

void handle_OnConnect() {
  server.send(200, "text/html", SendHTML()); 
}

void handle_robotfwd() {
  Serial.println("Going forward");
  powerRight = robotSpeed;
  powerLeft = robotSpeed;
  server.send(200, "text/html", SendHTML()); 
}

void handle_robotrear() {
  Serial.println("Going backward");
  powerRight = -robotSpeed;
  powerLeft = -robotSpeed;
  server.send(200, "text/html", SendHTML()); 
}

void handle_robotoff() {
  Serial.println("Stop");
  powerRight = 0;
  powerLeft = 0;
  server.send(200, "text/html", SendHTML()); 
}

void handle_robotr() {
  Serial.println("Right");
  powerRight = -robotSpeed;
  powerLeft = robotSpeed;
  server.send(200, "text/html", SendHTML()); 
}

void handle_robotl() {
  Serial.println("Left");
  powerRight = robotSpeed;
  powerLeft = -robotSpeed;
  server.send(200, "text/html", SendHTML()); 
}

void handle_robotlow() {
  Serial.println("robotSpeed Low");
  robotSpeed = 170;
  server.send(200, "text/html", SendHTML()); 
}

void handle_robotmid() {
  Serial.println("robotSpeed Mid");
  robotSpeed = 200;
  server.send(200, "text/html", SendHTML()); 
}

void handle_robothigh() {
  Serial.println("robotSpeed High");
  robotSpeed = 230;
  server.send(200, "text/html", SendHTML()); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

// Inspired from https://lastminuteengineers.com/creating-esp32-web-server-arduino-ide/
String SendHTML(){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>ESPRobot</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: inline;width: 50px;background-color: #3498db;border: none;color: white;padding: 15px 15px;text-decoration: none;font-size: 20px;line-height:40px;margin: 0px auto 25px;cursor: pointer;border-radius: 10px;}\n";
  ptr +=".button-on {background-color: #33cc33;}\n";
  ptr +=".button-on:active {background-color: #33cc33;}\n";
  ptr +=".button-off {background-color: #ff0000;}\n";
  ptr +=".button-off:active {background-color: #ff0000;}\n";
  ptr +=".button-speed {background-color: #66a3ff;}\n";
  ptr +=".button-speed:active {background-color: ##6a3ff;}\n";
  ptr +="p {font-size: 20px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<p>Control</p>\n";
  ptr +="<p><a class=\"button button-on\" href=\"/robotfwd\">Frwd</a></p>\n";
  ptr +="<p><a class=\"button button-on\" href=\"/robotl\">Left</a>&nbsp;<a class=\"button button-off\" href=\"/robotoff\">Stop</a>&nbsp;<a class=\"button button-on\" href=\"/robotr\">Rght</a></p>\n";
  ptr +="<p><a class=\"button button-on\" href=\"/robotrear\">Rear</a></p>\n";
  ptr +="<p>&nbsp;</p>\n";
  ptr +="<p>Speed</p>\n";
  ptr +="<p><a class=\"button button-speed\" href=\"/robotlow\">Low</a>&nbsp;<a class=\"button button-speed\" href=\"/robotmid\">Mid</a>&nbsp;<a class=\"button button-speed\" href=\"/robothigh\">High</a></p>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
