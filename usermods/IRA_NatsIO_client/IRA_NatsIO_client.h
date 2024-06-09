// https://github.com/isobit/arduino-nats
// https://github.com/davidsai/Arduino-nats-streaming-client

#pragma once

//#include "wled.h"
#include "base64.hpp"
#include "ir_data_packet.h"


uint    ext_mode = 15;  // This is to read in the 4 mode configuration pins (external), defaults on 15 = no pins connected
String  mac_string = "";     // MAC Address in string format
uint    ir_delay = 0;   // counter to time the IR delay effect
uint    post = 0;       // counter to post status messages


#include "ArduinoNats.h"
#include "defines.h"
#include "eeprom_map.h"           // this keeps the map of the non-volatile storage & the variables belonging to it
#include "operation_modes.h"      // these define all possible operation modes of IRA2020


/*
 * Usermods allow you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * 
 * This is an example for a v2 usermod.
 * v2 usermods are class inheritance based and can (but don't have to) implement more functions, each of them is shown in this example.
 * Multiple v2 usermods can be added to one compilation easily.
 * 
 * Creating a usermod:
 * This file serves as an example. If you want to create a usermod, it is recommended to use usermod_v2_empty.h from the usermods folder as a template.
 * Please remember to rename the class and file to a descriptive name.
 * You may also use multiple .h and .cpp files.
 * 
 * Using a usermod:
 * 1. Copy the usermod into the sketch folder (same folder as wled00.ino)
 * 2. Register the usermod by adding #include "usermod_filename.h" in the top and registerUsermod(new MyUsermodClass()) in the bottom of usermods_list.cpp
 */


WiFiClient client;

NATS nats(
	&client,
	NATS_SERVER, NATS_DEFAULT_PORT
);
NATSUtil::NATSServer natsserver;

#include "nats_cb_handlers.h"
  
// Gets the MAC address and prints it to serial Monitor
void wifi_printMAC() {
  byte mac[6];
  WiFi.macAddress(mac);
  DEBUG_PRINTF("%2x",mac[5]);
  DEBUG_PRINT(":");
  DEBUG_PRINTF("%2x",mac[4]);
  DEBUG_PRINT(":");
  DEBUG_PRINTF("%2x",mac[3]);
  DEBUG_PRINT(":");
  DEBUG_PRINTF("%2x",mac[2]);
  DEBUG_PRINT(":");
  DEBUG_PRINTF("%2x",mac[1]);
  DEBUG_PRINT(":");
  DEBUG_PRINTF("%2x\n",mac[0]);
}

// Build the string that is used for NATS subscriptions
void wifi_setMACstring() {
  byte mac[6];
  WiFi.macAddress(mac);
  mac_string =  String(mac[5], HEX) + 
                String("_") +
                String(mac[4], HEX) + 
                String("_") +
                String(mac[3], HEX) + 
                String("_") +
                String(mac[2], HEX) + 
                String("_") +
                String(mac[1], HEX) + 
                String("_") +
                String(mac[0], HEX);
}

bool parse_server(const char* msg) {
			//StaticJsonDocument<200> doc;
      DynamicJsonDocument doc(1000);
      //SpiRamJsonDocument doc(1000);

			DeserializationError error = deserializeJson(doc, msg);

			if (error) {
          DEBUG_PRINT("[NATS] deserializeJson() failed: ");
          DEBUG_PRINTLN(error.c_str());
          DEBUG_PRINT("Capacity: ");
          DEBUG_PRINTLN(doc.capacity());
  				return false;
			}
			else {
				natsserver.server_id = doc["server_id"]; 	// "NBFUSPNOHFLIABATORIMNGJFPFUK4M43PHP2EYJ2O2LRXK6F27PS7EXI"
				natsserver.server_name = doc["server_name"];
				natsserver.server_version = doc["version"]; 	// "2.6.5"
				natsserver.proto = doc["proto"];				// 1
				natsserver.go_version = doc["go"];			 	// "go1.17.2"
				natsserver.server_host = doc["host"]; 			// "0.0.0.0"
				natsserver.port = doc["port"]; 					// 4222
				natsserver.headers = doc["headers"]; 			// true
				natsserver.max_payload = doc["max_payload"]; 	// 1048576
				natsserver.client_id = doc["client_id"]; 		// 6
				natsserver.client_ip = doc["client_ip"]; 		// "192.168.20.238"	
        natsserver.nonce = doc["nonce"];            // "f-CpX3z5g_HA4-c"
        natsserver.xkey = doc["xkey"];              // "XCY2U7KDG7P7KZ7UBUSEVSJSF3XT4SVZKLXP5LEKE4B5YG6NGD7VBFFG"
			}
			return true;
}

void nats_print_server_info() {
  DEBUG_PRINT("[SYS] nats_print_server_info() running on core ");
  DEBUG_PRINTLN(xPortGetCoreID());
 
  DEBUG_PRINTLN("[NATS] Server INFO: ");
  DEBUG_PRINT("[NATS] Server ID: ");
  DEBUG_PRINTLN(natsserver.server_id);

  DEBUG_PRINT("[NATS] Server Name: ");
  DEBUG_PRINTLN(natsserver.server_name);

  DEBUG_PRINT("[NATS] Server Version: ");
  DEBUG_PRINTLN(natsserver.server_version);

  DEBUG_PRINT("[NATS] Proto: ");
  DEBUG_PRINTLN(natsserver.proto);

  DEBUG_PRINT("[NATS] Go Version: ");
  DEBUG_PRINTLN(natsserver.go_version);

  DEBUG_PRINT("[NATS] Server Host: ");
  DEBUG_PRINTLN(natsserver.server_host);

  DEBUG_PRINT("[NATS] Server Port: ");
  DEBUG_PRINTLN(natsserver.port);

  DEBUG_PRINT("[NATS] Headers: ");
  DEBUG_PRINTLN(natsserver.headers);

  DEBUG_PRINT("[NATS] Max Payload: ");
  DEBUG_PRINTLN(natsserver.max_payload);

  DEBUG_PRINT("[NATS] Client ID: ");
  DEBUG_PRINTLN(natsserver.client_id);

  DEBUG_PRINT("[NATS] Client IP: ");
  DEBUG_PRINTLN(natsserver.client_ip);

  DEBUG_PRINT("[NATS] nonce: ");
  DEBUG_PRINTLN(natsserver.nonce);

  DEBUG_PRINT("[NATS] xkey: ");
  DEBUG_PRINTLN(natsserver.xkey);
}


void nats_on_connect(const char* msg) {
  DEBUG_PRINTLN("[NATS] Connect");
  DEBUG_PRINTLN(msg);

  if(parse_server(msg))
  {
    nats_print_server_info();
  }

  String nats_debug_blink_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".blink");
  DEBUG_PRINT("[NATS] Subscribing: ");
  DEBUG_PRINTLN(nats_debug_blink_topic);
  nats.subscribe(nats_debug_blink_topic.c_str(), nats_debug_blink_handler);

  String nats_mode_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".mode");
  DEBUG_PRINT("[NATS] Subscribing: ");
  DEBUG_PRINTLN(nats_mode_topic);
  nats.subscribe(nats_mode_topic.c_str(), nats_mode_handler);

  String nats_ping_topic = String(NATS_ROOT_TOPIC) + String(".ping");
  DEBUG_PRINT("[NATS] Subscribing: ");
  DEBUG_PRINTLN(nats_ping_topic);
  nats.subscribe(nats_ping_topic.c_str(), nats_ping_handler);
  
  String nats_reset_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".reset");
  DEBUG_PRINT("[NATS] Subscribing: ");
  DEBUG_PRINTLN(nats_reset_topic);
  nats.subscribe(nats_reset_topic.c_str(), nats_reset_handler);

  String nats_config_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".config");
  DEBUG_PRINT("[NATS] Subscribing: ");
  DEBUG_PRINTLN(nats_config_topic);
  nats.subscribe(nats_config_topic.c_str(), nats_config_handler);

  //@TODO the following only need to subscribe upon mode change!
  String nats_dmx_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".dmx");
  DEBUG_PRINT("[NATS] Subscribing: ");
  DEBUG_PRINTLN(nats_dmx_topic);
  nats.subscribe(nats_dmx_topic.c_str(), nats_dmx_frame_handler);

  String nats_delta_dmx_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".deltadmx");
  DEBUG_PRINT("[NATS] Subscribing: ");
  DEBUG_PRINTLN(nats_delta_dmx_topic);
  nats.subscribe(nats_delta_dmx_topic.c_str(), nats_dmx_delta_frame_handler);

  String nats_rgb_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".rgb");
  DEBUG_PRINT("[NATS] Subscribing: ");
  DEBUG_PRINTLN(nats_rgb_topic);
  nats.subscribe(nats_rgb_topic.c_str(), nats_rgb_frame_handler);

  String nats_fx_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".fx");
  DEBUG_PRINT("[NATS] Subscribing: ");
  DEBUG_PRINTLN(nats_fx_topic);
  nats.subscribe(nats_fx_topic.c_str(), nats_fx_handler);

  String nats_name_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".name");
  DEBUG_PRINT("[NATS] Subscribing: ");
  DEBUG_PRINTLN(nats_name_topic);
  nats.subscribe(nats_name_topic.c_str(), nats_name_handler);

  // nats_announce();
}

void nats_on_error(const char* msg) {
  DEBUG_PRINT("[NATS] Error: ");
  DEBUG_PRINTLN(msg);
  DEBUG_PRINT(" outstanding pings: ");
  DEBUG_PRINTF("%d",nats.outstanding_pings);
  DEBUG_PRINT(" reconnect attempts: ");
  DEBUG_PRINTF("%d\n",nats.reconnect_attempts);
}

void nats_on_disconnect() {
  DEBUG_PRINTLN("[NATS] Disconnect");
}


//class name. Use something descriptive and leave the ": public Usermod" part :)
class IRA_NatsIO : public Usermod {

  private:

    // Private class members. You can declare variables and functions only accessible to your usermod here
    bool enabled = false;
    bool initDone = false;
    unsigned long lastTime = 0;

    // set your config variables to their boot default value (this can also be done in readFromConfig() or a constructor if you prefer)
  public:  
    String _NatsServer;
    String _NatsTopic;

  private:
    // string that are used multiple time (this will save some flash memory)
    static const char _name[];
    static const char _enabled[];

  
  public:

    // non WLED related methods, may be used for data exchange between usermods (non-inline methods should be defined out of class)

    /**
     * Enable/Disable the usermod
     */
    inline void enable(bool enable) { enabled = enable; }

    /**
     * Get usermod enabled/disabled state
     */
    inline bool isEnabled() { return enabled; }

    // in such case add the following to another usermod:
    //  in private vars:
    //   #ifdef USERMOD_EXAMPLE
    //   IRA_NatsIO* UM;
    //   #endif
    //  in setup()
    //   #ifdef USERMOD_EXAMPLE
    //   UM = (IRA_NatsIO*) usermods.lookup(USERMOD_ID_EXAMPLE);
    //   #endif
    //  somewhere in loop() or other member method
    //   #ifdef USERMOD_EXAMPLE
    //   if (UM != nullptr) isExampleEnabled = UM->isEnabled();
    //   if (!isExampleEnabled) UM->enable(true);
    //   #endif


    // methods called by WLED (can be inlined as they are called only once but if you call them explicitly define them out of class)

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * readFromConfig() is called prior to setup()
     * You can use it to initialize variables, sensors or similar.
     */
    void setup() {
      // do your set-up here
      //DEBUG_PRINTLN("Hello from my usermod!");
      DEBUG_PRINTLN("starting IRA usermod");
      dev_name_length = 32;

      initDone = true;
    }


    /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
    void connected() {
      DEBUG_PRINT("Connected to WiFi!");
      wifi_printMAC();
      wifi_setMACstring();

      /// NATS
      DEBUG_PRINTLN("[NATS] connecting to nats ...");
      nats.on_connect = nats_on_connect;
      nats.on_error = nats_on_error;
      nats.on_disconnect = nats_on_disconnect;

      if(nats.connect())
      {
        DEBUG_PRINT(" connected to: ");
        DEBUG_PRINTLN(NATS_SERVER);
        
      }
      else
      {
        DEBUG_PRINTLN("NATS not connected!");
      }
    }


    /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     * 
     * Tips:
     * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
     *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
     * 
     * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
     *    Instead, use a timer check as shown here.
     */
    void loop() {
      // if usermod is disabled or called during strip updating just exit
      // NOTE: on very long strips strip.isUpdating() may always return true so update accordingly
      if (!enabled || strip.isUpdating()) return;

      // do your magic here
      if (millis() - lastTime > 1000) {
        DEBUG_PRINTLN("I'm alive!");
        lastTime = millis();
      }
      
      if (ext_mode != getMode())
      {
        DEBUG_PRINT("[IO] Ext Mode changed: ");
        DEBUG_PRINTF("%d\n",getMode()); 
        ext_mode = getMode();

        printMode(ext_mode);
        if (WiFi.status() == WL_CONNECTED)
        {
          if(nats.connected)
          {
            nats_publish_ext_mode(ext_mode);
          }
          delay(300);
        } //against sending too many state changes at once
      }

      if (WiFi.status() == WL_CONNECTED)
      {
        // make sure new messages are handled
        nats.process();

        // send all status info
        post++;
        if (post == 100000)
        {  
          nats_publish_status();
          post = 0;
        }
      }
      
      //yield();        // Needed for FastLED
    }
    
    /*
     * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
     * Creating an "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
     * Below it is shown how this could be used for e.g. a light sensor
     */
    void addToJsonInfo(JsonObject& root)
    {
      // if "u" object does not exist yet wee need to create it
      JsonObject user = root["u"];
      if (user.isNull()) user = root.createNestedObject("u");

      //this code adds "u":{"ExampleUsermod":[20," lux"]} to the info object
      //int reading = 20;
      //JsonArray lightArr = user.createNestedArray(FPSTR(_name))); //name
      //lightArr.add(reading); //value
      //lightArr.add(F(" lux")); //unit

      // if you are implementing a sensor usermod, you may publish sensor data
      //JsonObject sensor = root[F("sensor")];
      //if (sensor.isNull()) sensor = root.createNestedObject(F("sensor"));
      //temp = sensor.createNestedArray(F("light"));
      //temp.add(reading);
      //temp.add(F("lux"));
    }


    /*
     * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void addToJsonState(JsonObject& root)
    {
      if (!initDone || !enabled) return;  // prevent crash on boot applyPreset()

      JsonObject usermod = root[FPSTR(_name)];
      if (usermod.isNull()) usermod = root.createNestedObject(FPSTR(_name));

      //usermod["user0"] = userVar0;
    }


    /*
     * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void readFromJsonState(JsonObject& root)
    {
      if (!initDone) return;  // prevent crash on boot applyPreset()

      JsonObject usermod = root[FPSTR(_name)];
      if (!usermod.isNull()) {
        // expect JSON usermod data in usermod name object: {"ExampleUsermod:{"user0":10}"}
        userVar0 = usermod["user0"] | userVar0; //if "user0" key exists in JSON, update, else keep old value
      }
      // you can as well check WLED state JSON keys
      //if (root["bri"] == 255) DEBUG_PRINTLN(F("Don't burn down your garage!"));
    }


    /*
     * addToConfig() can be used to add custom persistent settings to the cfg.json file in the "um" (usermod) object.
     * It will be called by WLED when settings are actually saved (for example, LED settings are saved)
     * If you want to force saving the current state, use serializeConfig() in your loop().
     * 
     * CAUTION: serializeConfig() will initiate a filesystem write operation.
     * It might cause the LEDs to stutter and will cause flash wear if called too often.
     * Use it sparingly and always in the loop, never in network callbacks!
     * 
     * addToConfig() will make your settings editable through the Usermod Settings page automatically.
     *
     * Usermod Settings Overview:
     * - Numeric values are treated as floats in the browser.
     *   - If the numeric value entered into the browser contains a decimal point, it will be parsed as a C float
     *     before being returned to the Usermod.  The float data type has only 6-7 decimal digits of precision, and
     *     doubles are not supported, numbers will be rounded to the nearest float value when being parsed.
     *     The range accepted by the input field is +/- 1.175494351e-38 to +/- 3.402823466e+38.
     *   - If the numeric value entered into the browser doesn't contain a decimal point, it will be parsed as a
     *     C int32_t (range: -2147483648 to 2147483647) before being returned to the usermod.
     *     Overflows or underflows are truncated to the max/min value for an int32_t, and again truncated to the type
     *     used in the Usermod when reading the value from ArduinoJson.
     * - Pin values can be treated differently from an integer value by using the key name "pin"
     *   - "pin" can contain a single or array of integer values
     *   - On the Usermod Settings page there is simple checking for pin conflicts and warnings for special pins
     *     - Red color indicates a conflict.  Yellow color indicates a pin with a warning (e.g. an input-only pin)
     *   - Tip: use int8_t to store the pin value in the Usermod, so a -1 value (pin not set) can be used
     *
     * See usermod_v2_auto_save.h for an example that saves Flash space by reusing ArduinoJson key name strings
     * 
     * If you need a dedicated settings page with custom layout for your Usermod, that takes a lot more work.  
     * You will have to add the setting to the HTML, xml.cpp and set.cpp manually.
     * See the WLED Soundreactive fork (code and wiki) for reference.  https://github.com/atuline/WLED
     * 
     * I highly recommend checking out the basics of ArduinoJson serialization and deserialization in order to use custom settings!
     */
    void addToConfig(JsonObject& root)
    {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_enabled)] = enabled;
      //save these vars persistently whenever settings are saved

      top[F("NatsServerURL")] = _NatsServer;
      top[F("NatsRootTopic")] = _NatsTopic;
    }

    /*
     * readFromConfig() can be used to read back the custom settings you added with addToConfig().
     * This is called by WLED when settings are loaded (currently this only happens immediately after boot, or after saving on the Usermod Settings page)
     * 
     * readFromConfig() is called BEFORE setup(). This means you can use your persistent values in setup() (e.g. pin assignments, buffer sizes),
     * but also that if you want to write persistent values to a dynamic buffer, you'd need to allocate it here instead of in setup.
     * If you don't know what that is, don't fret. It most likely doesn't affect your use case :)
     * 
     * Return true in case the config values returned from Usermod Settings were complete, or false if you'd like WLED to save your defaults to disk (so any missing values are editable in Usermod Settings)
     * 
     * getJsonValue() returns false if the value is missing, or copies the value into the variable provided and returns true if the value is present
     * The configComplete variable is true only if the "exampleUsermod" object and all values are present.  If any values are missing, WLED will know to call addToConfig() to save them
     * 
     * This function is guaranteed to be called on boot, but could also be called every time settings are updated
     */
    bool readFromConfig(JsonObject& root)
    {
      // default settings values could be set here (or below using the 3-argument getJsonValue()) instead of in the class definition or constructor
      // setting them inside readFromConfig() is slightly more robust, handling the rare but plausible use case of single value being missing after boot (e.g. if the cfg.json was manually edited and a value was removed)

      JsonObject top = root[FPSTR(_name)];

      bool configComplete = !top.isNull();
      configComplete &= getJsonValue(top[FPSTR(_enabled)], enabled,false);
      //DEBUG_PRINTLN(enabled);
      configComplete &= getJsonValue(top["NatsServerURL"], _NatsServer,NATS_SERVER);
      
      configComplete &= getJsonValue(top["NatsRootTopic"], _NatsTopic,NATS_ROOT_TOPIC);
      DEBUG_PRINTLN(_NatsServer);
      DEBUG_PRINTLN(_NatsTopic);
      return configComplete;
    }


    /*
     * appendConfigData() is called when user enters usermod settings page
     * it may add additional metadata for certain entry fields (adding drop down is possible)
     * be careful not to add too much as oappend() buffer is limited to 3k
     */
    void appendConfigData()
    {
     oappend(SET_F("addInfo('")); oappend(String(FPSTR(_name)).c_str()); oappend(SET_F(":NatsServer")); oappend(SET_F("',1,'enter URL to NATS serv');"));
     oappend(SET_F("addInfo('")); oappend(String(FPSTR(_name)).c_str()); oappend(SET_F(":NatsTopic") ); oappend(SET_F("',1,'enter Nats main Topic');")); 
    }


    /*
     * handleOverlayDraw() is called just before every show() (LED strip update frame) after effects have set the colors.
     * Use this to blank out some LEDs or set them to a different color regardless of the set effect mode.
     * Commonly used for custom clocks (Cronixie, 7 segment)
     */
    void handleOverlayDraw()
    {
      //strip.setPixelColor(0, RGBW32(0,0,0,0)) // set the first pixel to black
    }


    /**
     * handleButton() can be used to override default button behaviour. Returning true
     * will prevent button working in a default way.
     * Replicating button.cpp
     */
    bool handleButton(uint8_t b) {
      yield();
      // ignore certain button types as they may have other consequences
      if (!enabled
       || buttonType[b] == BTN_TYPE_NONE
       || buttonType[b] == BTN_TYPE_RESERVED
       || buttonType[b] == BTN_TYPE_PIR_SENSOR
       || buttonType[b] == BTN_TYPE_ANALOG
       || buttonType[b] == BTN_TYPE_ANALOG_INVERTED) {
        return false;
      }

      bool handled = false;
      // do your button handling here
      return handled;
    }
  

#ifndef WLED_DISABLE_MQTT
    /**
     * handling of MQTT message
     * topic only contains stripped topic (part after /wled/MAC)
     */
    bool onMqttMessage(char* topic, char* payload) {
      // check if we received a command
      //if (strlen(topic) == 8 && strncmp_P(topic, PSTR("/command"), 8) == 0) {
      //  String action = payload;
      //  if (action == "on") {
      //    enabled = true;
      //    return true;
      //  } else if (action == "off") {
      //    enabled = false;
      //    return true;
      //  } else if (action == "toggle") {
      //    enabled = !enabled;
      //    return true;
      //  }
      //}
      return false;
    }

    /**
     * onMqttConnect() is called when MQTT connection is established
     */
    void onMqttConnect(bool sessionPresent) {
      // do any MQTT related initialisation here
      //publishMqtt("I am alive!");
    }
#endif


    /**
     * onStateChanged() is used to detect WLED state change
     * @mode parameter is CALL_MODE_... parameter used for notifications
     */
    void onStateChange(uint8_t mode) {
      // do something if WLED state changed (color, brightness, effect, preset, etc)
    }


    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId()
    {
      return USERMOD_ID_EXAMPLE;
    }

   //More methods can be added in the future, this example will then be extended.
   //Your usermod will remain compatible as it does not need to implement all methods from the Usermod base class!
};


// add more strings here to reduce flash memory usage
const char IRA_NatsIO::_name[]    PROGMEM = "IRA_NatsIO";
const char IRA_NatsIO::_enabled[] PROGMEM = "enabled";


// implementation of non-inline member methods

