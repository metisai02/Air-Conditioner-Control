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
	ac.next.freshAir = stdAc::freshAir_t::kOff;		   // Don't use any fresh/air/ventilation modes if we can.
	ac.next.voice = false;						   // Don't use any voice control modes if we can.
	ac.next.smart = false;						   // Don't use any "smart" modes if we can.
	ac.next.iFeel = false;						   // Don't use any iFeel modes if we can.
	ac.next.quiet = false;						   // Don't use any quiet/silent/etc modes.
    ac.next.command = stdAc::ac_command_t::kControlCommand; // Use the standard control command for this message.
	Serial.println("Try to turn on & off every supported A/C type ...");
}

void WIFI_setup() {
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
    if(MDNS.begin("esp8266"))
	{
		Serial.println("MDNS responder started");
	}

	server.on("/", WIFI_handleRoot);
	server.onNotFound(WIFI_handleNotFound);
    server.begin();


    // --- POWER & LIGHTS ---
    server.on("/pwr_on", [](){ 
        Serial.println("Power Start");
        ac.next.power = true; ac.next.command = stdAc::ac_command_t::kPowerCommand; ac.sendAc(); server.send(200); 
    });
    server.on("/pwr_off", [](){ 
        Serial.println("Power Stop");
        ac.next.power = false; ac.next.command = stdAc::ac_command_t::kPowerCommand; ac.sendAc(); server.send(200); 
    });
    server.on("/light_on", [](){ 
        Serial.println("Light Start");
        ac.next.light = true; ac.next.command = stdAc::ac_command_t::kLightCommand; ac.sendAc(); server.send(200); 
    });
    server.on("/light_off", [](){ 
        Serial.println("Light Stop");
        ac.next.light = false; ac.next.command = stdAc::ac_command_t::kLightCommand; ac.sendAc(); server.send(200); 
    });

    // --- MODES ---
    server.on("/mode_auto", [](){ Serial.println("Mode: Auto"); ac.next.mode = stdAc::opmode_t::kAuto; ac.next.command = stdAc::ac_command_t::kModeCommand; ac.sendAc(); server.send(200); });
    server.on("/mode_cool", [](){ Serial.println("Mode: Cool"); ac.next.mode = stdAc::opmode_t::kCool; ac.next.command = stdAc::ac_command_t::kModeCommand; ac.sendAc(); server.send(200); });
    server.on("/mode_heat", [](){ Serial.println("Mode: Heat"); ac.next.mode = stdAc::opmode_t::kHeat; ac.next.command = stdAc::ac_command_t::kModeCommand; ac.sendAc(); server.send(200); });
    server.on("/mode_dry", [](){ Serial.println("Mode: Dry"); ac.next.mode = stdAc::opmode_t::kDry; ac.next.command = stdAc::ac_command_t::kModeCommand; ac.sendAc(); server.send(200); });
    server.on("/mode_fan", [](){ Serial.println("Mode: Fan"); ac.next.mode = stdAc::opmode_t::kFan; ac.next.command = stdAc::ac_command_t::kModeCommand; ac.sendAc(); server.send(200); });

    // --- FAN SPEED ---
    server.on("/fan_auto", [](){ Serial.println("Fan Speed: Auto"); ac.next.fanspeed = stdAc::fanspeed_t::kAuto; ac.next.command = stdAc::ac_command_t::kFanSpeedCommand; ac.sendAc(); server.send(200); });
    server.on("/fan_1", [](){ Serial.println("Fan Speed: 1"); ac.next.fanspeed = stdAc::fanspeed_t::kMin; ac.next.command = stdAc::ac_command_t::kFanSpeedCommand; ac.sendAc(); server.send(200); });
    server.on("/fan_2", [](){ Serial.println("Fan Speed: 2"); ac.next.fanspeed = stdAc::fanspeed_t::kLow; ac.next.command = stdAc::ac_command_t::kFanSpeedCommand; ac.sendAc(); server.send(200); });
    server.on("/fan_3", [](){ Serial.println("Fan Speed: 3"); ac.next.fanspeed = stdAc::fanspeed_t::kMedium; ac.next.command = stdAc::ac_command_t::kFanSpeedCommand; ac.sendAc(); server.send(200); });
    server.on("/fan_4", [](){ Serial.println("Fan Speed: 4"); ac.next.fanspeed = stdAc::fanspeed_t::kHigh; ac.next.command = stdAc::ac_command_t::kFanSpeedCommand; ac.sendAc(); server.send(200); });
    server.on("/fan_5", [](){ Serial.println("Fan Speed: 5"); ac.next.fanspeed = stdAc::fanspeed_t::kMax; ac.next.command = stdAc::ac_command_t::kFanSpeedCommand; ac.sendAc(); server.send(200); });

    // --- SWINGS (Toggles) ---
    server.on("/swingv", [](){ 
        ac.next.swingv = (ac.next.swingv == stdAc::swingv_t::kOff) ? stdAc::swingv_t::kAuto : stdAc::swingv_t::kOff;
        Serial.println(ac.next.swingv != stdAc::swingv_t::kOff ? "Vertical Swing Start" : "Vertical Swing Stop");
        ac.next.command = stdAc::ac_command_t::kSwingVCommand;
        ac.sendAc(); server.send(200); 
    });
    server.on("/swingh", [](){ 
        ac.next.swingh = (ac.next.swingh == stdAc::swingh_t::kOff) ? stdAc::swingh_t::kAuto : stdAc::swingh_t::kOff;
        Serial.println(ac.next.swingh != stdAc::swingh_t::kOff ? "Horizontal Swing Start" : "Horizontal Swing Stop");
        ac.next.command = stdAc::ac_command_t::kSwingHCommand;
        ac.sendAc(); server.send(200); 
        ac.sendAc(); server.send(200); 
    });

    // --- FRESH AIR ---
    server.on("/fresh_off", [](){ Serial.println("Fresh Air Stop"); ac.next.freshAir = stdAc::freshAir_t::kOff; ac.next.command = stdAc::ac_command_t::kFreshAirCommand; ac.sendAc(); server.send(200); });
    server.on("/fresh_low", [](){ Serial.println("Fresh Air: Low"); ac.next.freshAir = stdAc::freshAir_t::kLow; ac.next.command = stdAc::ac_command_t::kFreshAirCommand; ac.sendAc(); server.send(200); });
    server.on("/fresh_mid", [](){ Serial.println("Fresh Air: Mid"); ac.next.freshAir = stdAc::freshAir_t::kMedium; ac.next.command = stdAc::ac_command_t::kFreshAirCommand; ac.sendAc(); server.send(200); });
    server.on("/fresh_high", [](){ Serial.println("Fresh Air: High"); ac.next.freshAir = stdAc::freshAir_t::kMax; ac.next.command = stdAc::ac_command_t::kFreshAirCommand; ac.sendAc(); server.send(200); });

    // --- SPECIAL FUNCTIONS (Toggles) ---
    server.on("/turbo", [](){ 
        ac.next.turbo = !ac.next.turbo;
        ac.next.command = stdAc::ac_command_t::kSuperCommand;
        Serial.println(ac.next.turbo ? "Turbo Start" : "Turbo Stop");
        ac.sendAc(); server.send(200); 
    });
    server.on("/quiet", [](){ 
        ac.next.quiet = !ac.next.quiet; 
        ac.next.command = stdAc::ac_command_t::kQuietCommand;
        Serial.println(ac.next.quiet ? "Quiet Start" : "Quiet Stop");
        ac.sendAc(); server.send(200); 
    });
    server.on("/voice", [](){ 
        ac.next.voice = !ac.next.voice; 
        ac.next.command = stdAc::ac_command_t::kVoiceCommand;
        Serial.println(ac.next.voice ? "Voice Start" : "Voice Stop");
        ac.sendAc(); server.send(200); 
    });
    server.on("/smart", [](){ 
        ac.next.smart = !ac.next.smart; 
        ac.next.command = stdAc::ac_command_t::kSmartCommand;
        Serial.println(ac.next.smart ? "Smart Start" : "Smart Stop");
        ac.sendAc(); server.send(200); 
    });
    server.on("/sleep", [](){ 
        ac.next.sleep = (ac.next.sleep == -1) ? 1 : -1; 
        ac.next.command = stdAc::ac_command_t::kSleepCommand;
        Serial.println(ac.next.sleep != -1 ? "Sleep Start" : "Sleep Stop");
        ac.sendAc(); server.send(200); 
    });
    server.on("/eco", [](){ 
        ac.next.econo = !ac.next.econo; 
        ac.next.command = stdAc::ac_command_t::kEconoCommand;
        Serial.println(ac.next.econo ? "Eco Start" : "Eco Stop");
        ac.sendAc(); server.send(200); 
    });
    Serial.println("HTTP server started");

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
void WIFI_handleRoot() {
    String html;
    html.reserve(8192); // Larger buffer for complex UI

    html = F("<!DOCTYPE html><html><head>");
    html += F("<meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    html += F("<style>");
    html += F("body { font-family: sans-serif; background-color: #121212; color: #e0e0e0; margin: 0; padding: 15px; display: flex; flex-direction: column; align-items: center; }");
    html += F(".remote-container { background-color: #1e1e1e; padding: 20px; border-radius: 20px; width: 100%; max-width: 360px; box-shadow: 0 10px 20px rgba(0,0,0,0.5); }");
    html += F("h3 { font-size: 11px; color: #666; text-transform: uppercase; margin: 15px 0 8px 5px; border-bottom: 1px solid #333; }");
    html += F(".grid { display: grid; gap: 8px; margin-bottom: 10px; }");
    html += F(".g2 { grid-template-columns: 1fr 1fr; }");
    html += F(".g3 { grid-template-columns: 1fr 1fr 1fr; }");
    html += F(".g5 { grid-template-columns: repeat(5, 1fr); }");
    html += F(".btn { background-color: #333; color: #eee; border: none; padding: 12px 2px; border-radius: 8px; font-size: 12px; cursor: pointer; transition: 0.1s; }");
    html += F(".btn:active { transform: scale(0.95); background-color: #444; }");
    html += F(".b-on { background-color: #2e7d32; } .b-off { background-color: #c62828; }");
    html += F("</style></head><body>");

    html += F("<div class='remote-container'>");

    // 1. POWER & LIGHT
    html += F("<h3>Main Power</h3><div class='grid g2'>");
    html += F("<button class='btn b-on' onclick='sendCmd(\"pwr_on\")'>ON</button><button class='btn b-off' onclick='sendCmd(\"pwr_off\")'>OFF</button>");
    html += F("</div><h3>Panel Light</h3><div class='grid g2'>");
    html += F("<button class='btn' onclick='sendCmd(\"light_on\")'>Light ON</button><button class='btn' onclick='sendCmd(\"light_off\")'>Light OFF</button></div>");

    // 2. MODES (5 Buttons)
    html += F("<h3>Operation Mode</h3><div class='grid g5'>");
    html += F("<button class='btn' onclick='sendCmd(\"mode_auto\")'>AUTO</button><button class='btn' onclick='sendCmd(\"mode_cool\")'>COOL</button>");
    html += F("<button class='btn' onclick='sendCmd(\"mode_heat\")'>HEAT</button><button class='btn' onclick='sendCmd(\"mode_dry\")'>DRY</button>");
    html += F("<button class='btn' onclick='sendCmd(\"mode_fan\")'>FAN</button></div>");

    // 3. FAN SPEED (1-5 + Auto)
    html += F("<h3>Fan Speed</h3><div class='grid g3'>");
    html += F("<button class='btn' onclick='sendCmd(\"fan_1\")'>1</button><button class='btn' onclick='sendCmd(\"fan_2\")'>2</button>");
    html += F("<button class='btn' onclick='sendCmd(\"fan_3\")'>3</button><button class='btn' onclick='sendCmd(\"fan_4\")'>4</button>");
    html += F("<button class='btn' onclick='sendCmd(\"fan_5\")'>5</button><button class='btn' onclick='sendCmd(\"fan_auto\")'>AUTO</button></div>");

    // 4. SWINGS
    html += F("<h3>Swing Control</h3><div class='grid g2'>");
    html += F("<button class='btn' onclick='sendCmd(\"swingv\")'>V. Swing</button><button class='btn' onclick='sendCmd(\"swingh\")'>H. Swing</button></div>");

    // 5. FRESH AIR
    html += F("<h3>Fresh Air</h3><div class='grid g2'>");
    html += F("<button class='btn' onclick='sendCmd(\"fresh_off\")'>OFF</button><button class='btn' onclick='sendCmd(\"fresh_low\")'>LOW</button>");
    html += F("<button class='btn' onclick='sendCmd(\"fresh_mid\")'>MID</button><button class='btn' onclick='sendCmd(\"fresh_high\")'>HIGH</button></div>");

    // 6. SPECIAL FUNCTIONS
    html += F("<h3>Special Functions</h3><div class='grid g3'>");
    html += F("<button class='btn' onclick='sendCmd(\"turbo\")'>Turbo</button><button class='btn' onclick='sendCmd(\"quiet\")'>Quiet</button>");
    html += F("<button class='btn' onclick='sendCmd(\"voice\")'>Voice</button><button class='btn' onclick='sendCmd(\"smart\")'>Smart</button>");
    html += F("<button class='btn' onclick='sendCmd(\"sleep\")'>Sleep</button><button class='btn' onclick='sendCmd(\"eco\")'>Eco</button></div>");

    html += F("</div><script>function sendCmd(p){fetch('/'+p);}</script></body></html>");
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