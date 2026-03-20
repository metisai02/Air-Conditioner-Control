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
	ac.next.protocol = decode_type_t::KELON168;	   // Set a protocol to use.
	ac.next.mode = stdAc::opmode_t::kCool;		   // Run in cool mode initially.
	ac.next.celsius = true;						   // Use Celsius for temp units. False = Fahrenheit
	ac.next.degrees = 25;						   // 25 degrees.
	ac.next.fanspeed = stdAc::fanspeed_t::kMedium; // Start the fan at medium.
	ac.next.swingv = stdAc::swingv_t::kOff;		   // Don't swing the fan up or down.
	ac.next.swingh = stdAc::swingh_t::kOff;		   // Don't swing the fan left or right.
	ac.next.light = false;						   // Turn off any LED/Lights/Display that we can.
	ac.next.beep = false;						   // Turn off any beep from the A/C if we can.
	ac.next.econo = false;						   // Turn off any economy modes if we can.
	ac.next.filter = false;						   // Turn off any Ion/Mold/Health filters if we can.
	ac.next.turbo = false;						   // Don't use any turbo/powerful/etc modes.
	ac.next.quiet = false;						   // Don't use any quiet/silent/etc modes.
	ac.next.sleep = -1;							   // Don't set any sleep time or modes.
	ac.next.clean = false;						   // Turn off any Cleaning options if we can.
	ac.next.clock = -1;							   // Don't set any current time if we can avoid it.
	ac.next.power = false;						   // Initially start with the unit off.
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

	// Add these new handlers inside WIFI_setup()
	server.on("/pwr_on", []()
			  {
  Serial.println("Power ON command received");
  ac.next.power = true;
  ac.sendAc();
  server.send(200, "text/plain", "OK"); });

	server.on("/pwr_off", []()
			  {
  Serial.println("Power OFF command received");
  ac.next.power = false;
  ac.sendAc();
  server.send(200, "text/plain", "OK"); });

	server.on("/tup", []()
			  {
  Serial.println("Temperature UP command received");
  ac.next.degrees = min(ac.next.degrees + 1.0f, 30.0f); // Cap at 30C
  ac.sendAc();
  server.send(200, "text/plain", "OK"); });

	server.on("/tdown", []()
			  {
  Serial.println("Temperature DOWN command received");
  ac.next.degrees = max(ac.next.degrees - 1.0f, 16.0f); // Floor at 16C
  ac.sendAc();
  server.send(200, "text/plain", "OK"); });

	// Examples of how you would map the other special functions from the remote:
	server.on("/eco", []()
			  { 
              Serial.println("Economy mode command received");
              ac.next.econo = true; ac.sendAc(); server.send(200, "text/plain", "OK"); });
	server.on("/quiet", []()
			  { Serial.println("Quiet mode command received"); ac.next.quiet = true; ac.next.turbo = false; ac.sendAc(); server.send(200, "text/plain", "OK"); });
	server.on("/turbo", []()
			  { Serial.println("Turbo mode command received"); ac.next.quiet = false; ac.next.turbo = true; ac.sendAc(); server.send(200, "text/plain", "OK"); });

	// You may need to look up your specific protocol's swingv_t and swingh_t settings.
	// This is an example for Louver Vertical toggle:
	server.on("/louver_v", []()
			  {
  Serial.println("Louver Vertical toggle command received");
  // Simple on/off vertical swing toggle. Your protocol might have positions (e.g. kSwingvTop).
  ac.next.swingv = (ac.next.swingv == stdAc::swingv_t::kOff) ? stdAc::swingv_t::kAuto : stdAc::swingv_t::kOff;
  ac.sendAc();
  server.send(200, "text/plain", "OK"); });
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
	String html;
	html.reserve(4096); // Optimized memory management

	html = F("<!DOCTYPE html><html><head>");
	html += F("<meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>");
	html += F("<style>");
	// Define colors and grid system
	html += F("body { font-family: sans-serif; background-color: #121212; color: #e0e0e0; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; }");
	html += F(".remote-container { background-color: #1e1e1e; padding: 25px; border-radius: 20px; box-shadow: 0 10px 20px rgba(0,0,0,0.5); width: 320px; }");
	html += F(".display-panel { background-color: #2c2c2c; border-radius: 10px; padding: 15px; margin-bottom: 20px; text-align: center; }");
	html += F(".display-label { font-size: 14px; color: #aaa; margin-bottom: 5px; }");
	html += F(".temp-val { font-size: 56px; font-weight: bold; color: #00e676; }");

	html += F(".grid-container { display: grid; gap: 15px; grid-template-columns: 1fr 1fr; margin-bottom: 15px; }");
	html += F(".tri-grid { grid-template-columns: repeat(3, 1fr); gap: 10px; }");
	html += F(".pwr-grid { grid-template-columns: 1fr 1fr; margin-bottom: 15px; }");

	// Base button style
	html += F(".btn { background-color: #333; color: #e0e0e0; border: none; padding: 15px; border-radius: 12px; font-size: 16px; cursor: pointer; transition: background 0.2s, transform 0.1s; display: flex; align-items: center; justify-content: center; flex-direction: column; gap: 8px; }");
	html += F(".btn:active { transform: scale(0.95); }");

	// Custom button styles from your remote
	html += F(".btn-on { background-color: #00c853; color: #fff; font-weight: bold; } .btn-on:active { background-color: #009624; }");
	html += F(".btn-off { background-color: #d32f2f; color: #fff; font-weight: bold; } .btn-off:active { background-color: #9a0007; }");
	html += F(".btn-eco { color: #81c784; font-size: 20px; font-weight: bold; }"); // Green leaf
	html += F(".btn-mode { color: #aaa; font-size: 20px; }");					   // Sun/flake
	html += F(".btn-temp { background-color: #424242; font-size: 24px; font-weight: bold; color: #fff; }");

	// Bottom text label buttons (from physical remote labels)
	html += F(".lbl-btn { border-radius: 8px; font-size: 14px; padding: 10px; background-color: #2c2c2c; text-align: center; color: #aaa; }");
	html += F(".lbl-btn:active { background-color: #333; }");
	html += F("</style></head><body>");

	html += F("<div class='remote-container'>");

	// 1. Digital Display (Showing Current State)
	html += F("<div class='display-panel'>");
	html += F("<div class='display-label'>TARGET TEMP</div>");
	html += F("<div class='temp-val'><span id='currTemp'>");
	html += String((int)ac.next.degrees);
	html += F("</span>&deg;C</div>");
	html += F("</div>");

	// 2. Separate ON/OFF Row (Special Request)
	html += F("<div class='grid-container pwr-grid'>");
	html += F("<button class='btn btn-on' onclick='sendCmd(\"pwr_on\")'>AC ON</button>");
	html += F("<button class='btn btn-off' onclick='sendCmd(\"pwr_off\")'>AC OFF</button>");
	html += F("</div>");

	// 3. Mode/Eco and Temperature Row (Grid matching image layout)
	html += F("<div class='grid-container' style='grid-template-columns: repeat(3, 1fr);'>");

	// Left: Eco (Green Leaf) / Mode (Sun/Flake)
	html += F("<div style='display:grid; gap:10px;'>");
	html += F("<button class='btn btn-eco' onclick='sendCmd(\"eco\")'>&#127803;</button>");			// Leaf emoji
	html += F("<button class='btn btn-mode' onclick='sendCmd(\"mode\")'>&#9728;&#10052;</button>"); // Sun/Flake symbols
	html += F("</div>");

	// Center: Temp UP/DOWN
	html += F("<div style='display:grid; gap:10px;'>");
	html += F("<button class='btn btn-temp' onclick='sendCmd(\"tup\")'>&and;</button>");
	html += F("<button class='btn btn-temp' onclick='sendCmd(\"tdown\")'>&or;</button>");
	html += F("</div>");

	// Right: Fan/Turbo (From image)
	html += F("<div style='display:grid; gap:10px;'>");
	html += F("<button class='btn btn-mode' onclick='sendCmd(\"fan\")'>&#9881;</button>");	   // Gear for settings/fan
	html += F("<button class='btn btn-mode' onclick='sendCmd(\"turbo\")'>&#127811;</button>"); // Abstract swirl for turbo/swirl
	html += F("</div>");

	html += F("</div>"); // End Mode/Temp row

	html += F("<hr style='border:0; border-top:1px solid #333; margin:15px 0;'>");

	// 4. Labeled Rectangular Buttons (Louver Control)
	html += F("<div class='grid-container tri-grid'>");
	html += F("<button class='btn lbl-btn' onclick='sendCmd(\"louver_h\")'>Louver H</button>");
	html += F("<button class='btn lbl-btn' onclick='sendCmd(\"louver_a\")'>Louver A</button>");
	html += F("<button class='btn lbl-btn' onclick='sendCmd(\"louver_v\")'>Louver V</button>");
	html += F("</div>");

	// 5. Special Functions Row (Boost/Vacation/Quiet)
	html += F("<div class='grid-container tri-grid'>");
	html += F("<button class='btn lbl-btn' style='color:#ffc107;' onclick='sendCmd(\"boost\")'>Boost</button>");	   // Yellow for boost
	html += F("<button class='btn lbl-btn' style='color:#03a9f4;' onclick='sendCmd(\"vacation\")'>Vacation</button>"); // Blue for vacation
	html += F("<button class='btn lbl-btn' style='color:#9e9e9e;' onclick='sendCmd(\"quiet\")'>Quiet</button>");	   // Grey for quiet
	html += F("</div>");

	html += F("</div>"); // End main container

	// Optimized fetch JS
	html += F("<script>");
	html += F("function sendCmd(path) { fetch('/' + path); }");
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