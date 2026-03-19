/* Copyright 2019 David Conran
 *
 * This example code demonstrates how to use the "Common" IRac class to control
 * various air conditions. The IRac class does not support all the features
 * for every protocol. Some have more detailed support that what the "Common"
 * interface offers, and some only have a limited subset of the "Common" options.
 *
 * This example code will:
 * o Try to turn on, then off every fully supported A/C protocol we know of.
 * o It will try to put the A/C unit into Cooling mode at 25C, with a medium
 *   fan speed, and no fan swinging.
 * Note: Some protocols support multiple models, only the first model is tried.
 *
 */
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRutils.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <main_private.h>

/*WIFI settings*/

IRac ac(IR_REMOTE_KELON168_GPIO_PIN); // Create a A/C object using GPIO to sending messages with.

ESP8266WebServer server(80);

void IR_setup();
void WIFI_setup();
void WIFI_loop();
void WIFI_handleRoot();
void WIFI_handleNotFound();

void IR_setup()
{
  delay(200);
  /*Setup the A/C specifications*/
  ac.next.protocol = decode_type_t::KELON168;    // Set a protocol to use.
  ac.next.mode = stdAc::opmode_t::kCool;         // Run in cool mode initially.
  ac.next.celsius = true;                        // Use Celsius for temp units. False = Fahrenheit
  ac.next.degrees = 25;                          // 25 degrees.
  ac.next.fanspeed = stdAc::fanspeed_t::kMedium; // Start the fan at medium.
  ac.next.swingv = stdAc::swingv_t::kOff;        // Don't swing the fan up or down.
  ac.next.swingh = stdAc::swingh_t::kOff;        // Don't swing the fan left or right.
  ac.next.light = false;                         // Turn off any LED/Lights/Display that we can.
  ac.next.beep = false;                          // Turn off any beep from the A/C if we can.
  ac.next.econo = false;                         // Turn off any economy modes if we can.
  ac.next.filter = false;                        // Turn off any Ion/Mold/Health filters if we can.
  ac.next.turbo = false;                         // Don't use any turbo/powerful/etc modes.
  ac.next.quiet = false;                         // Don't use any quiet/silent/etc modes.
  ac.next.sleep = -1;                            // Don't set any sleep time or modes.
  ac.next.clean = false;                         // Turn off any Cleaning options if we can.
  ac.next.clock = -1;                            // Don't set any current time if we can avoid it.
  ac.next.power = false;                         // Initially start with the unit off.
  Serial.println("Try to turn on & off every supported A/C type ...");
}

void WIFI_setup()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("WiFi connecting...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266"))
  {
    Serial.println("MDNS responder started");
  }

  server.on("/", WIFI_handleRoot);
  server.onNotFound(WIFI_handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  server.on("/tempup", []()
            {
  Serial.println("tempup start");
  ac.next.degrees = min(ac.next.degrees + 1, 30.0f); // Max 30C
  if(ac.sendAc()) {
    Serial.println("tempup sent successfully");
  } else {
    Serial.println("tempup failed to send");
  }
  server.send(200, "text/plain", "OK"); 
  Serial.println("tempup end"); });

  server.on("/tempdown", []()
            {
    Serial.println("tempdown start");
    ac.next.degrees = max(ac.next.degrees - 1, 16.0f); // Min 16C
    if(ac.sendAc()) {
      Serial.println("tempdown sent successfully");
    } else {
      Serial.println("tempdown failed to send");
    }
    server.send(200, "text/plain", "OK"); 
    Serial.println("tempdown end"); });

  server.on("/togglepower", []()
            {
    Serial.println("togglepower start");
    ac.next.power = !ac.next.power;
    if(ac.sendAc()) {
      Serial.println("togglepower sent successfully");
    } else {
      Serial.println("togglepower failed to send");
    }
    server.send(200, "text/plain", "OK"); 
    Serial.println("togglepower end"); });

  server.on("/modecool", []()
            {
    Serial.println("modecool start");
    ac.next.mode = stdAc::opmode_t::kCool;
    if(ac.sendAc()) {
      Serial.println("modecool sent successfully");
    } else {
      Serial.println("modecool failed to send");
    }
    server.send(200, "text/plain", "OK");
    Serial.println("modecool end"); });

  server.on("/modefan", []()
            {
    Serial.println("modefan start");
    ac.next.mode = stdAc::opmode_t::kFan;
    if(ac.sendAc()) {
      Serial.println("modefan sent successfully");
    } else {
      Serial.println("modefan failed to send");
    }
    server.send(200, "text/plain", "OK"); 
    
    Serial.println("modefan end"); });
}

void setup()
{
  Serial.begin(115200);
  IR_setup();
  WIFI_setup();
}

void loop()
{
  WIFI_loop();
  MDNS.update();
}
void WIFI_handleRoot()
{
  String html = F("<!DOCTYPE html><html><head>");
  html += F("<meta name='viewport' content='width=device-width, initial-scale=1'>");
  html += F("<style>");
  html += F("body { font-family: sans-serif; background: #f0f2f5; text-align: center; color: #333; }");
  html += F(".card { background: white; margin: 20px auto; padding: 20px; border-radius: 15px; max-width: 400px; box-shadow: 0 4px 10px rgba(0,0,0,0.1); }");
  html += F(".temp-display { font-size: 48px; font-weight: bold; margin: 10px 0; color: #007bff; }");
  html += F(".btn { background: #007bff; border: none; color: white; padding: 15px 25px; border-radius: 10px; font-size: 18px; margin: 5px; cursor: pointer; width: 40%; transition: 0.3s; }");
  html += F(".btn:active { transform: scale(0.95); background: #0056b3; }");
  html += F(".btn-pwr { background: #dc3545; width: 90%; }");
  html += F(".grid { display: flex; flex-wrap: wrap; justify-content: center; }");
  html += F("</style></head><body>");

  html += F("<div class='card'>");
  html += F("<h2>AC Control</h2>");

  // Temperature Section
  html += F("<div class='temp-display'><span id='temp'>");
  html += String((int)ac.next.degrees);
  html += F("</span>&deg;C</div>");

  html += F("<div class='grid'>");
  html += F("<button class='btn' onclick='sendCmd(\"tempdown\")'>-</button>");
  html += F("<button class='btn' onclick='sendCmd(\"tempup\")'>+</button>");
  html += F("</div>");

  html += F("<hr style='border:0; border-top:1px solid #eee; margin:20px 0;'>");

  // Power and Mode
  html += F("<button class='btn btn-pwr' onclick='sendCmd(\"togglepower\")'>ON / OFF</button>");

  html += F("<div class='grid' style='margin-top:10px;'>");
  html += F("<button class='btn' style='background:#17a2b8' onclick='sendCmd(\"modecool\")'>Cool</button>");
  html += F("<button class='btn' style='background:#ffc107; color:black' onclick='sendCmd(\"modefan\")'>Fan</button>");
  html += F("</div>");

  html += F("</div>"); // End Card

  // AJAX Script
  html += F("<script>");
  html += F("function sendCmd(path) {");
  html += F("  fetch('/' + path).then(response => {");
  html += F("    if(path.includes('temp')) { location.reload(); }"); // Reload to see new temp
  html += F("  });");
  html += F("}");
  html += F("</script></body></html>");

  server.send(200, "text/html", html);
}
void WIFI_handleNotFound()
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
}

void WIFI_loop()
{
  server.handleClient();
}