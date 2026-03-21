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
    // --- WIFI INITIALIZATION ---
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("WiFi connecting...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());    
    
    if(MDNS.begin("esp8266")) {
        Serial.println("MDNS responder started");
    }

    // --- CORE SERVER HANDLERS ---
    server.on("/", WIFI_handleRoot);
    server.onNotFound(WIFI_handleNotFound);

    // --- TEMPERATURE CONTROLS ---
    server.on("/tup", [](){ 
        if(ac.next.degrees < 30) {
            ac.next.degrees++;
            ac.next.command = stdAc::ac_command_t::kTempCommand;
            Serial.printf("Temp Increased: %d°C\n", (int)ac.next.degrees);
            ac.sendAc();
        }
        server.send(200); 
    });

    server.on("/tdown", [](){ 
        if(ac.next.degrees > 16) {
            ac.next.degrees--;
            ac.next.command = stdAc::ac_command_t::kTempCommand;
            Serial.printf("Temp Decreased: %d°C\n", (int)ac.next.degrees);
            ac.sendAc();
        }
        server.send(200); 
    });

    // --- TIMER SYSTEM (0.5h Resolution) ---
    server.on("/set_timer", [](){
        bool active = server.arg("active") == "true";
        float hours = server.arg("val").toFloat();
        
        if (active && hours > 0) {
            // Convert hours to minutes for the library
            ac.next.sleep = (int16_t)(hours * 60); 
            // In many protocols, Timer uses the Sleep command type
            ac.next.command = stdAc::ac_command_t::kSleepCommand; 
            Serial.printf("Timer ARMED: %.1f hours (%d mins)\n", hours, ac.next.sleep);
        } else {
            ac.next.sleep = -1; // -1 usually disables the timer in the library
            Serial.println("Timer CANCELLED/OFF");
        }
        
        ac.sendAc();
        server.send(200);
    });

    // --- POWER & LIGHTS ---
    server.on("/pwr_on", [](){ 
        Serial.println("Power Start");
        ac.next.power = true; 
        Serial.printf("DEBUG /pwr_on BEFORE sendAc: power=%d, mode=%d, degrees=%.1f\n", 
                      (int)ac.next.power, (int)ac.next.mode, ac.next.degrees);
        ac.next.command = stdAc::ac_command_t::kPowerCommand; 
        Serial.printf("DEBUG /pwr_on SETTING command: %d\n", (int)ac.next.command);
        ac.sendAc(); 
        server.send(200); 
    });
    server.on("/pwr_off", [](){ 
        Serial.println("Power Stop");
        ac.next.power = false; 
        Serial.printf("DEBUG /pwr_off BEFORE sendAc: power=%d, mode=%d, degrees=%.1f\n", 
                      (int)ac.next.power, (int)ac.next.mode, ac.next.degrees);
        ac.next.command = stdAc::ac_command_t::kPowerCommand; 
        Serial.printf("DEBUG /pwr_off SETTING command: %d\n", (int)ac.next.command);
        ac.sendAc(); 
        server.send(200); 
    });
    server.on("/light_on", [](){ 
        Serial.println("Light Start");
        ac.next.light = true; 
        ac.next.command = stdAc::ac_command_t::kLightCommand; 
        ac.sendAc(); server.send(200); 
    });
    server.on("/light_off", [](){ 
        Serial.println("Light Stop");
        ac.next.light = false; 
        ac.next.command = stdAc::ac_command_t::kLightCommand; 
        ac.sendAc(); server.send(200); 
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

    // --- AIRFLOW (Swings & Fresh Air) ---
    server.on("/swingv", [](){ 
        ac.next.swingv = (ac.next.swingv == stdAc::swingv_t::kOff) ? stdAc::swingv_t::kAuto : stdAc::swingv_t::kOff;
        ac.next.command = stdAc::ac_command_t::kSwingVCommand;
#ifdef DEBUG_KELON168
        Serial.println(ac.next.swingv != stdAc::swingv_t::kOff ? "V. Swing Start" : "V. Swing Stop");
#endif
        ac.sendAc(); server.send(200); 
    });
    server.on("/swingh", [](){ 
        ac.next.swingh = (ac.next.swingh == stdAc::swingh_t::kOff) ? stdAc::swingh_t::kAuto : stdAc::swingh_t::kOff;
        ac.next.command = stdAc::ac_command_t::kSwingHCommand;
#ifdef DEBUG_KELON168
        Serial.println(ac.next.swingh != stdAc::swingh_t::kOff ? "H. Swing Start" : "H. Swing Stop");
#endif
        ac.sendAc(); server.send(200); 
    });

    // --- FRESH AIR ---
    server.on("/fresh_off", [](){ Serial.println("Fresh Air Stop"); ac.next.freshAir = stdAc::freshAir_t::kOff; ac.next.command = stdAc::ac_command_t::kControlCommand; ac.sendAc(); server.send(200); });
    server.on("/fresh_low", [](){ Serial.println("Fresh Air: Low"); ac.next.freshAir = stdAc::freshAir_t::kLow; ac.next.command = stdAc::ac_command_t::kControlCommand; ac.sendAc(); server.send(200); });
    server.on("/fresh_mid", [](){ Serial.println("Fresh Air: Mid"); ac.next.freshAir = stdAc::freshAir_t::kMedium; ac.next.command = stdAc::ac_command_t::kControlCommand; ac.sendAc(); server.send(200); });
    server.on("/fresh_high", [](){ Serial.println("Fresh Air: High"); ac.next.freshAir = stdAc::freshAir_t::kMax; ac.next.command = stdAc::ac_command_t::kControlCommand; ac.sendAc(); server.send(200); });

    // --- SPECIAL FUNCTIONS (Toggles) ---
    server.on("/turbo", [](){ 
        ac.next.turbo = !ac.next.turbo; 
        ac.next.command = stdAc::ac_command_t::kSuperCommand;
#ifdef DEBUG_KELON168
        Serial.println(ac.next.turbo ? "Turbo Start" : "Turbo Stop");
#endif
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
        // Note: Manual Sleep toggle usually different from Timer
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

    server.begin();
    Serial.println("HTTP server started and listening...");
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
    html.reserve(20480); // Increased buffer for logic

    html = F("<!DOCTYPE html><html><head>");
    html += F("<meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    html += F("<style>");
    html += F("body { font-family: sans-serif; background-color: #121212; color: #e0e0e0; margin: 0; padding: 15px; display: flex; flex-direction: column; align-items: center; }");
    html += F(".remote-container { background-color: #1e1e1e; padding: 20px; border-radius: 25px; width: 100%; max-width: 360px; box-shadow: 0 10px 20px rgba(0,0,0,0.5); }");
    
    // Display Panels
    html += F(".display-row { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 15px; }");
    html += F(".panel { background: #262626; border-radius: 15px; padding: 15px; text-align: center; border: 1px solid #333; position: relative; }");
    html += F(".val-big { font-size: 38px; font-weight: bold; color: #00e676; line-height: 1; }");
    html += F(".timer-led { width: 8px; height: 8px; border-radius: 50%; background: #444; position: absolute; top: 10px; right: 10px; transition: 0.3s; }");
    html += F(".timer-on { background: #ffb300; box-shadow: 0 0 8px #ffb300; }");
    
    // Buttons & Highlighting
    html += F("h3 { font-size: 11px; color: #666; text-transform: uppercase; margin: 15px 0 8px 5px; border-bottom: 1px solid #333; }");
    html += F(".grid { display: grid; gap: 8px; margin-bottom: 10px; }");
    html += F(".g2 { grid-template-columns: 1fr 1fr; } .g3 { grid-template-columns: 1fr 1fr 1fr; } .g5 { grid-template-columns: repeat(5, 1fr); }");
    html += F(".btn { background-color: #333; color: #eee; border: 2px solid transparent; padding: 12px 2px; border-radius: 12px; font-size: 11px; cursor: pointer; transition: 0.2s; }");
    
    // THE RED CIRCLE/BORDER STYLE
    html += F(".active-sel { border-color: #ff3d00 !important; box-shadow: inset 0 0 5px rgba(255,61,0,0.5); color: #fff !important; }");
    
    html += F(".btn-control { background-color: #424242; font-size: 16px; font-weight: bold; }");
    html += F(".b-on { background-color: #2e7d32; } .b-off { background-color: #c62828; }");
    html += F("</style></head><body>");

    html += F("<div class='remote-container'>");

    // 1. DISPLAYS
    html += F("<div class='display-row'>");
    html += F("<div class='panel'><div class='val-big'><span id='deg'>");
    html += String((int)ac.next.degrees);
    html += F("</span>°C</div><div style='font-size:10px;color:#666'>TEMP</div></div>");
    html += F("<div class='panel'><div id='t-led' class='timer-led'></div><div class='val-big'><span id='tim'>0.0</span>h</div><div style='font-size:10px;color:#666'>TIMER</div></div>");
    html += F("</div>");

    // 2. ADJUSTMENT CONTROLS
    html += F("<h3>Adjustments</h3><div class='grid g2'>");
    html += F("<button class='btn btn-control' onclick='changeTemp(1)'>TEMP +</button><button class='btn btn-control' onclick='changeTemp(-1)'>TEMP -</button>");
    html += F("<button class='btn btn-control' style='color:#ffb300' onclick='changeTimer(0.5)'>TIME +</button><button class='btn btn-control' style='color:#ffb300' onclick='changeTimer(-0.5)'>TIME -</button>");
    html += F("</div>");

    // 3. POWER
    html += F("<h3>Power</h3><div class='grid g2'>");
    html += "<button class='btn b-on " + String(ac.next.power ? "active-sel" : "") + "' onclick='sendCmd(this,\"pwr_on\")'>ON</button>";
    html += "<button class='btn b-off " + String(!ac.next.power ? "active-sel" : "") + "' onclick='sendCmd(this,\"pwr_off\")'>OFF</button>";
    html += F("</div>");

    // 4. MODES
    html += F("<h3>Mode</h3><div class='grid g5'>");
    const char* mNames[] = {"AUTO","COOL","HEAT","DRY","FAN"};
    int mValues[] = {0, 1, 2, 3, 4}; // Mapping to stdAc::opmode_t values
    for(int i=0; i<5; i++) {
        String active = ((int)ac.next.mode == mValues[i]) ? "active-sel" : "";
        html += "<button class='btn " + active + "' onclick='sendMode(this," + String(i) + ")'>" + String(mNames[i]) + "</button>";
    }
    html += F("</div>");

    // 5. FAN SPEED
    html += F("<h3>Fan Speed</h3><div class='grid g3'>");
    // Auto is usually 0, 1-5 follow
    String fAutoActive = ((int)ac.next.fanspeed == 0) ? "active-sel" : "";
    for(int i=1; i<=5; i++) {
        String active = ((int)ac.next.fanspeed == i) ? "active-sel" : "";
        html += "<button class='btn " + active + "' onclick='sendFan(this," + String(i) + ")'>" + String(i) + "</button>";
    }
    html += "<button class='btn " + fAutoActive + "' onclick='sendFan(this,0)'>AUTO</button></div>";

    // 6. AIRFLOW (Swings only)
    html += F("<h3>Airflow</h3><div class='grid g2'>");
    html += "<button class='btn " + String(ac.next.swingv != stdAc::swingv_t::kOff ? "active-sel" : "") + "' onclick='toggleBtn(this,\"swingv\")'>V. SWING</button>";
    html += "<button class='btn " + String(ac.next.swingh != stdAc::swingh_t::kOff ? "active-sel" : "") + "' onclick='toggleBtn(this,\"swingh\")'>H. SWING</button>";
    html += F("</div>");

    // 7. FRESH AIR (4-State Selection)
    html += F("<h3>Fresh Air</h3><div class='grid g2'>");
    const char* fNames[] = {"OFF", "LOW", "MID", "HIGH"};
    const char* fPaths[] = {"fresh_off", "fresh_low", "fresh_mid", "fresh_high"};
    int fValues[] = {0, 1, 2, 3}; // Mapping to your freshAir_t enum

    for(int i=0; i<4; i++) {
        // Check if this specific level is the one currently active in the struct
        String active = ((int)ac.next.freshAir == fValues[i]) ? "active-sel" : "";
        html += "<button class='btn " + active + "' onclick='sendCmd(this,\"" + String(fPaths[i]) + "\")'>" + String(fNames[i]) + "</button>";
    }
    html += F("</div>");

    // 8. SPECIAL FUNCTIONS
    html += F("<h3>Special</h3><div class='grid g3'>");
    struct Spec { String n; String p; bool val; };
    Spec specs[] = { {"TURBO","turbo",ac.next.turbo}, {"QUIET","quiet",ac.next.quiet}, {"ECO","eco",ac.next.econo}, 
                     {"SLEEP","sleep",ac.next.sleep!=-1}, {"SMART","smart",ac.next.smart}, {"VOICE","voice",ac.next.voice} };
    for(auto s : specs) {
        html += "<button class='btn " + String(s.val ? "active-sel" : "") + "' onclick='toggleBtn(this,\"" + s.p + "\")'>" + s.n + "</button>";
    }
    html += F("</div>");

    html += F("</div><script>");
    // JavaScript handles the Red Circle moving when you click
    html += F("function clearGrid(btn){ let siblings = btn.parentNode.querySelectorAll('.btn'); siblings.forEach(s=>s.classList.remove('active-sel')); }");
    html += F("function sendCmd(btn,p){ clearGrid(btn); btn.classList.add('active-sel'); fetch('/'+p); }");
    html += F("function sendMode(btn,m){ sendCmd(btn, ['mode_auto','mode_cool','mode_heat','mode_dry','mode_fan'][m]); }");
    html += F("function sendFan(btn,f){ let paths=['fan_auto','fan_1','fan_2','fan_3','fan_4','fan_5']; sendCmd(btn, paths[f]); }");
    html += F("function toggleBtn(btn,p){ btn.classList.toggle('active-sel'); fetch('/'+p); }");
    
    html += F("function changeTemp(d){let e=document.getElementById('deg'); let v=parseInt(e.innerText)+d; if(v>=16&&v<=30){e.innerText=v; fetch(d>0?'/tup':'/tdown');}}");
    html += F("function changeTimer(d){let e=document.getElementById('tim'); let v=parseFloat(e.innerText)+d; if(v>=0&&v<=24){e.innerText=v.toFixed(1);}}");
    html += F("function toggleTimer(s){document.getElementById('t-led').className=s?'timer-led timer-on':'timer-led'; fetch('/set_timer?active='+s+'&val='+document.getElementById('tim').innerText);}");
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