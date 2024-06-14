#define R(c) (byte((c) >> 16))
#define G(c) (byte((c) >> 8))
#define B(c) (byte(c))
#define W(c) (byte((c) >> 24))
#include <wled.h>
#include "defines.h"
// This is the IRA2020 specific heartbeat signal to announce a device, more then the NATS Ping/Pong it defines the device specific topic roots
void nats_announce()
{
  String announce_message = String( "{\"mac_string\": \"") + mac_string + String("\",");   // Add MAC
  announce_message += String("\"IP\":\"") + String(WiFi.localIP()) + String("\",");       // Add IP
  announce_message += String("\"HWTYPE\":\"") + String("Wemos 32 D mini") + String("\",");        // Add HW TYPE
  announce_message += String("\"HWREV\":\"") + String("Rev.01") + String("\",");          // Add HW board Rev
  announce_message += String("\"EXTMODE\":\"") + String(ext_mode) + String("\",");
  announce_message += String("\"MODE\":\"") + String(nats_mode) + String("\",");
  announce_message += String("\"VERSION\":\"") + String(IRA_VERSION) + String("\",");
 /* String name;
  // send the name back
  for(uint i = 0; i < dev_name_length-1; i++)       // -1 because is always NULL terminated
  {
    char letter = EEPROM.read(DEV_NAME + i);
    if(letter != 0xff)
      name +=  letter;
      
  }*/
  // TODO check what this is abaout
  String name = cmDNS;
    
  announce_message += String("\"NAME\":\"") + name + String("\",");
  
  // TODO: Add everything in EEPROM,  ...
  announce_message += String("\"pixel_length\": ") + strip.getLengthTotal() + String(",");
  /* announce_message += String("\"fx\": ") + fx_select + String(",");
  announce_message += String("\"fx_speed\": ") + fx_speed + String(",");
  announce_message += String("\"fx_xfade\": ") + fx_xfade + String(",");
  announce_message += String("\"fx_fgnd_r\": ") + fx_fgnd_r + String(",");
  announce_message += String("\"fx_fgnd_g\": ") + fx_fgnd_g + String(",");
  announce_message += String("\"fx_fgnd_b\": ") + fx_fgnd_b + String(",");
  announce_message += String("\"fx_bgnd_r\": ") + fx_bgnd_r + String(",");
  announce_message += String("\"fx_bgnd_g\": ") + fx_bgnd_g + String(",");
  announce_message += String("\"fx_bgnd_b\": ") + fx_bgnd_b + String(" ");
*/
  announce_message += String("}");

  // TODO: Add everything in EEPROM,  ...
  String natspath = natssetup.natsTopic + String(".") + natssetup.natsGroup + String(".devices.") + mac_string;
  String announce_topic = natspath + String(".announce");
  DEBUG_PRINTLN(announce_topic);
  nats->publish(announce_topic.c_str(), announce_message.c_str());
}

void nats_publish_status ()
{
  String natspath = natssetup.natsTopic + String(".") + natssetup.natsGroup + String(".devices.") + mac_string;
  String status_topic = natspath + String(".status");
  //long rssi = WiFi.RSSI();
  //String status_message = String("{\"rssi\": \"") + String(rssi) + String("\"}");

  //nats->publish(status_topic.c_str(), status_message.c_str());

  //==========================================
    AsyncWebSocketMessageBuffer * buffer;

  if (!requestJSONBufferLock(12)) return;

  JsonObject state = doc.createNestedObject("state");
  serializeState(state);
  JsonObject info  = doc.createNestedObject("info");
  serializeInfo(info);

  size_t len = measureJson(doc);
  DEBUG_PRINTF("JSON buffer size: %u for WS request (%u).\n", doc.memoryUsage(), len);

  size_t heap1 = ESP.getFreeHeap();
  DEBUG_PRINT(F("heap ")); DEBUG_PRINTLN(ESP.getFreeHeap());
  #ifdef ESP8266
  if (len>heap1) {
    DEBUG_PRINTLN(F("Out of memory (WS)!"));
    nats->publish(msg.reply, F("{\"Success\":false,\"Error\":\"Out of memory\"}"));
    return;
  }
  #endif
  buffer = ws.makeBuffer(len); // will not allocate correct memory sometimes on ESP8266
  #ifdef ESP8266
  size_t heap2 = ESP.getFreeHeap();
  DEBUG_PRINT(F("heap ")); DEBUG_PRINTLN(ESP.getFreeHeap());
  #else
  size_t heap2 = 0; // ESP32 variants do not have the same issue and will work without checking heap allocation
  #endif
  if (!buffer || heap1-heap2<len) {
    releaseJSONBufferLock();
    DEBUG_PRINTLN(F("Nats WLED buffer allocation failed."));
    nats->publish(status_topic.c_str(), F("{\"Success\":false,\"Error\":\"Out of memory\"}"));
    return; //out of memory

  }

  buffer->lock();
  serializeJson(doc, (char *)buffer->get(), len);

  DEBUG_PRINT(F("Sending NATS status "));
  DEBUG_PRINTLN(status_topic);
  DEBUG_PRINTLN((char *)buffer->get());

  nats->publish(status_topic.c_str(), (char *)buffer->get());

  buffer->unlock();

  releaseJSONBufferLock();

  //==========================================
}

void nats_publish_ext_mode(uint mode)
{
  String natspath = natssetup.natsTopic + String(".") + natssetup.natsGroup + String(".devices.") + mac_string;
  String ext_mode_topic = natspath + String(".ext_mode");
  nats->publish(ext_mode_topic.c_str(), String(mode, DEC).c_str());
}

void nats_publish_ir(uint16_t packet, uint8_t teamnr)
{
  String natspath = natssetup.natsTopic + String(".") + natssetup.natsGroup + String(".devices.") + mac_string;
  
  String ir_topic = natspath + String(".ir");

  String ir_message = String("{\"packet\": \"") + String(packet) + String("\",");

  String team;

  switch (teamnr) {
    case 1:
      team = "REX";
      break;
    case 2:
      team = "GIGGLE";
      break;
    case 4:
      team = "BUZZ";
      break;
  }
  ir_message += String("\"team\":\"") + team + String("\"}");

  nats->publish(ir_topic.c_str(), ir_message.c_str());
}
/// checked
void nats_ping_handler(NATS::msg msg) {
    DEBUG_PRINTLN("[NATS] ping message received");

    delay(random(0,1000));      // random delay up to a sec to avoid broadcast storm

    nats_announce();
}
/// checked
// This blinks the on-board debug LED a defined number of times (in the message) for board identification
void nats_debug_blink_handler(NATS::msg msg) {
  DEBUG_PRINTLN("[NATS] debug led blink message received");
  nats->publish(msg.reply, "received!");
// config for LED is set in define
	int count = atoi(msg.data);
	while (count-- > 0) {
    DEBUG_PRINT("_");
		digitalWrite(DEBUG_LED, LOW);
		delay(100);
    DEBUG_PRINT("|");
		digitalWrite(DEBUG_LED, HIGH);
		delay(100);
	}
  DEBUG_PRINTLN("");
}

// This sets the operation mode of the board
void nats_mode_handler(NATS::msg msg) { 
  if(nats_mode != atoi(msg.data))
  {
    // TODO check if mode is valid
    DEBUG_PRINT("[NATS] mode changed to: ");
    DEBUG_PRINTLN(msg.data);

    nats_mode = atoi(msg.data);
//    EEPROM.write(NATS_MODE, nats_mode);
//    EEPROM.commit();

    printMode(nats_mode);

    // make sure we switch IR reception back off
    if((nats_mode != MODE_RGB_TO_PIXELS_W_IR ) && (nats_mode != MODE_FX_TO_PIXELS_W_IR))
    {
      ir_delay = 0;
    }
  }
  nats->publish(msg.reply, "+OK");
}

void nats_reset_handler(NATS::msg msg) { 
  DEBUG_PRINTLN("[NATS] Reset CB Handler");
  if(atoi(msg.data))
  {
    DEBUG_PRINTLN("[NATS] Restarting in 2 seconds");
 
    delay(2000);
 
    ESP.restart();
  }
  else
  {
    DEBUG_PRINTLN("[NATS] Reset Handler, value was not >0");
  }
}

/* Accepts 3 bytes
2 bytes: parameter index
1 byte: parameter value
*/
void nats_config_handler(NATS::msg msg) {
  DEBUG_PRINTLN("[NATS] Config Handler");

  DEBUG_PRINT("[NATS] Message Data: ");
  DEBUG_PRINTLN(msg.data);

  uint8_t dec_data[msg.size];
  uint8_t dec_length = decode_base64((unsigned char *) msg.data, dec_data);

  DEBUG_PRINT("[NATS] Decoded Message Length: ");
  DEBUG_PRINTLN(dec_length);

  DEBUG_PRINT("[NATS] Decoded Message Data: ");
  for(int i = 0; i<dec_length; i++)
  {
    DEBUG_PRINTF("%2x",dec_data[i]);
    DEBUG_PRINT(" ");
  }
  DEBUG_PRINTLN(" ");

  uint16_t param_index = (dec_data[0] << 8) + dec_data[1];
  DEBUG_PRINT("[NATS] Parameter Index: ");
  DEBUG_PRINTLN(param_index);

  DEBUG_PRINT("[NATS] Parameter Value: ");
  DEBUG_PRINTLN(dec_data[2]);
  
//  EEPROM.write(param_index, dec_data[2]);
//  EEPROM.commit();

  nats->publish(msg.reply, "+OK"); 
}

/* This receives a complete DMX Frame of 512+1 bytes (allow SC to be set), 513 bytes are Base64 encoded
Currently the start code isn't used
*/
void nats_dmx_frame_handler(NATS::msg msg) {
  DEBUG_PRINTLN("[NATS] DMX Frame Handler");

  if( (nats_mode == MODE_DMX_OUT) || (nats_mode == MODE_DMX_TO_PIXELS_W_IR) || (nats_mode == MODE_DMX_TO_PIXELS_WO_IR))   // need to add ext mode to this
  {
    DEBUG_PRINT("[NATS] Message Data: ");
    DEBUG_PRINTLN(msg.data);

    uint8_t dec_data[msg.size];
    uint16_t dec_length = decode_base64((unsigned char *) msg.data, dec_data);  // hopefully length = 513, if not we just overwrite the first section

    DEBUG_PRINT("[NATS] Decoded Message Length: ");
    DEBUG_PRINTLN(dec_length);

    if(dec_length > 513)
    {
      DEBUG_PRINT("[NATS] Got too many bytes!");
      nats->publish(msg.reply, "NOK");   // Not in the right mode
    }
    else
    {
      DEBUG_PRINT("[NATS] Decoded Message Data: ");
      for(int i = 0; i<dec_length; i++)
      {
        DEBUG_PRINT(i);
        DEBUG_PRINT(": ");
        DEBUG_PRINTF("%2x",dec_data[i]);
        DEBUG_PRINT(" ");
/// TODO add DMX write over the wled bus
//        DMX::Write(i, dec_data[i]);     // Let's write it away
      }
      DEBUG_PRINTLN(" ");
    }
  }
  else
  {
    DEBUG_PRINTLN("[NATS] not in a DMX mode, leaving");
    nats->publish(msg.reply, "NOK");   // Not in the right mode
  }
}

// This receives a delta DMX Frame (normally smaller) of channel-value pairs
/// TODO not implented yet
void nats_dmx_delta_frame_handler(NATS::msg msg) {
  DEBUG_PRINTLN("[NATS] DMX Delta Frame Handler");

  if( (nats_mode == MODE_DMX_OUT) || (nats_mode == MODE_DMX_TO_PIXELS_W_IR) || (nats_mode == MODE_DMX_TO_PIXELS_WO_IR))   // need to add ext mode to this
  {
    DEBUG_PRINT("[NATS] Message Data: ");
    DEBUG_PRINTLN(msg.data);
  }
  else
  {
    DEBUG_PRINTLN("[NATS] not in a DMX mode, leaving");
    nats->publish(msg.reply, "NOK, Not in DMX mode");
  }
}

/* This routine is the NATS RGB data parser
2 bytes offset
2 byte: length
datablock: <length> * <#data> * <pixels>
#data comes from EEPROM memory
bv: 0x00 0x02 0x00 0x02 0xRR 0xGG 0xBB 0xRR 0xGG 0xBB
for 2 pixels with RGB (3data per pixel) with offset 2
this way we can send RGBA or RGBW pixels in a later phase
*/
void nats_rgb_frame_handler(NATS::msg msg) {
  DEBUG_PRINTLN("[NATS] RGB Frame Handler");

  if( (nats_mode == MODE_RGB_TO_PIXELS_W_IR ) || (nats_mode == MODE_RGB_TO_PIXELS_WO_IR) )   // need to add ext mode to this
  {
    if( (ir_delay != 0) && (nats_mode == MODE_RGB_TO_PIXELS_W_IR))
    { 
      nats->publish(msg.reply, "NOK, IR overruled");   // IR overruled
      return;
    }

    uint8_t dec_data[msg.size];
    uint8_t dec_length = decode_base64((unsigned char *) msg.data, dec_data);
    uint16_t pix_len = (dec_data[2] << 8) + dec_data[3];

    // Exit ASAP
    if(pix_len > strip.getLengthTotal())
    {
      DEBUG_PRINTLN("[NATS] Too many pixels");
      nats->publish(msg.reply, "NOK, Too many pixels");   // Not in the right mode
      return;
    }

    if(pix_len == 0)
    {
      DEBUG_PRINTLN("[NATS] Length is 0");
      nats->publish(msg.reply, "NOK, Length is 0");   // Not ok
      return;
    }

    DEBUG_PRINT("[NATS] Message Data: ");
    DEBUG_PRINTLN(msg.data);

    DEBUG_PRINT("[NATS] Decoded Message Length: ");
    DEBUG_PRINTLN(dec_length);

    DEBUG_PRINT("[NATS] Decoded Message Data: ");
    for(int i = 0; i<dec_length; i++)
    {
      DEBUG_PRINTF("%2x",dec_data[i]);
      DEBUG_PRINT(" ");
    }
    DEBUG_PRINTLN(" ");

    uint16_t pix_offset = (dec_data[0] << 8) + dec_data[1];
    DEBUG_PRINT("[NATS] RGB Pixel Data Offset: ");
    DEBUG_PRINTLN(pix_offset);
    /// TODO number of bytes per pixel shouldn't be hardcoded can come from wled config
    DEBUG_PRINT("[NATS] RGB Per Pixel Datapoints: ");
    DEBUG_PRINTLN(3);                            // @TODO: This needs to be checked & used!, currently A and W are ditched

    for(int led_index = 0; led_index < pix_len; led_index++) // from 0 increment with #data per pixel
    {
      uint pixel = led_index + pix_offset;

      /// TODO check limits of pixel max number of leds over all segments
      if (pixel < MAX_LEDS){
        strip.setPixelColor(pixel,
                          dec_data[4 + (led_index*3)],
                          dec_data[4 + (led_index*3) + 1],
                          dec_data[4 + (led_index*3) + 2]);
        //leds[pixel].r = dec_data[4 + (led_index*3)];                    // actual data starts at 4th byte
        //leds[pixel].g = dec_data[4 + (led_index*3) + 1];
        //leds[pixel].b = dec_data[4 + (led_index*3) + 2];   
        
        uint32_t clr = strip.getPixelColor(pixel);

        DEBUG_PRINT("[NATS] RGB Pixel: ");
        DEBUG_PRINT(pixel);
        DEBUG_PRINT(", R:");
        DEBUG_PRINT(R(clr));
        DEBUG_PRINT(" G:");
        DEBUG_PRINT(G(clr));
        DEBUG_PRINT(" B:");
        DEBUG_PRINTLN(B(clr));
      }
    }
    DEBUG_PRINT("[NATS] RGB Pixel Data Copied Over!");
    nats->publish(msg.reply, "+OK");  
  }
  else
  {
    DEBUG_PRINTLN("[NATS] not in a RGB mode, leaving");
    nats->publish(msg.reply, "NOK, not in a RGB mode");   // Not in the right mode
  }
}

/* This routine is the NATS RGB data parser
2 bytes length
1 byte: #data per pixel
datablock: <length> * <#data> * <pixels>
bv: 0x00 0x02 0x03 0xRR 0xGG 0xBB 0xRR 0xGG 0xBB
for 2 pixels with RGB (3data per pixel)
this way we can send RGBA or RGBW pixels in a later phase
*/
void nats_old_rgb_frame_handler(NATS::msg msg) {
  DEBUG_PRINTLN("[NATS] Old RGB Frame Handler");

  if( (nats_mode == MODE_RGB_TO_PIXELS_W_IR ) || (nats_mode == MODE_RGB_TO_PIXELS_WO_IR) )   // need to add ext mode to this
  {
    DEBUG_PRINT("[NATS] Message Data: ");
    DEBUG_PRINTLN(msg.data);

    uint8_t dec_data[msg.size];
    uint8_t dec_length = decode_base64((unsigned char *) msg.data, dec_data);

    DEBUG_PRINT("[NATS] Decoded Message Length: ");
    DEBUG_PRINTLN(dec_length);

    DEBUG_PRINT("[NATS] Decoded Message Data: ");
    for(int i = 0; i<dec_length; i++)
    {
      DEBUG_PRINTF("%2x",dec_data[i]);
      DEBUG_PRINT(" ");
    }
    DEBUG_PRINTLN(" ");

    uint16_t pix_len = (dec_data[0] << 8) + dec_data[1];
    DEBUG_PRINT("[NATS] RGB Pixel Data Length: ");
    DEBUG_PRINTLN(pix_len);
    
    if(pix_len > MAX_PIXELS)
    {
      DEBUG_PRINTLN("[NATS] Too many pixels");
      nats->publish(msg.reply, "NOK, Too many pixels");   // Not in the right mode
      return;
    }

    DEBUG_PRINT("[NATS] RGB Per Pixel Datapoints: ");
    DEBUG_PRINTLN(dec_data[2]);                            // @TODO: This needs to be checked & used!, currently A and W are ditched

    for(int led_index = 0; led_index < pix_len; led_index++) // from 0 increment with #data per pixel
    {
      strip.setPixelColor(led_index,
                        dec_data[4 + (led_index*3)],
                        dec_data[4 + (led_index*3)]+1,
                        dec_data[4 + (led_index*3)]+2);
      //leds[led_index].r = dec_data[3 + led_index*dec_data[2]];                    // actual data starts at 3th byte
      //leds[led_index].g = dec_data[3 + led_index*dec_data[2] + 1];
      //leds[led_index].b = dec_data[3 + led_index*dec_data[2] + 2];   

      uint32_t clr = strip.getPixelColor(led_index);

      DEBUG_PRINT("[NATS] RGB Pixel: ");
      DEBUG_PRINT(led_index);
      DEBUG_PRINT(", R:");
      DEBUG_PRINT(R(clr));
      DEBUG_PRINT(" G:");
      DEBUG_PRINT(G(clr));
      DEBUG_PRINT(" B:");
      DEBUG_PRINTLN(B(clr));
    }
    DEBUG_PRINT("[NATS] RGB Pixel Data Copied Over!");
    nats->publish(msg.reply, "+OK");  
  }
  else
  {
    DEBUG_PRINTLN("[NATS] not in a RGB mode, leaving");

    nats->publish(msg.reply, "NOK, not in RGB mode");   // Not in the right mode
  }
}

/* This is the FX callback routine
Base64 encoded message = 
First Byte = FX selection
Second Byte = FX Speed
Third Byte = FX Crossfade
4Th Byte = FGND R
5Th = FGND G
6TH = FGND B
7TH = BGND R
8TH = BGND G
9TH = BGND B
*/
void nats_fx_handler(NATS::msg msg) {
  DEBUG_PRINTLN("[NATS] FX Handler");

  if( (nats_mode == MODE_FX_TO_PIXELS_W_IR ) || (nats_mode == MODE_FX_TO_PIXELS_WO_IR) )   // need to add ext mode to this
  {
    uint8_t dec_data[msg.size];
    uint8_t dec_length = decode_base64((unsigned char *) msg.data, dec_data); 
    
    if(dec_length == 0)
    {
      DEBUG_PRINTLN("[NATS] Decode_base64 error");
      nats->publish(msg.reply, "NOK, base64 error");
      return;
    } 

    fx_select = dec_data[0];
    fx_speed = dec_data[1];
    fx_xfade = dec_data[2];

    fx_fgnd_r = dec_data[3];
    fx_fgnd_g = dec_data[4];
    fx_fgnd_b = dec_data[5];

    fx_bgnd_r = dec_data[6];
    fx_bgnd_g = dec_data[7];
    fx_bgnd_b = dec_data[8];
    
    DEBUG_PRINT("[NATS] Selected FX:");
    DEBUG_PRINTLN(fx_select);

    DEBUG_PRINT("[NATS] Set Speed:");
    DEBUG_PRINTLN(fx_speed);

    DEBUG_PRINT("[NATS] Set XFade:");
    DEBUG_PRINTLN(fx_xfade);

    DEBUG_PRINT("[NATS] FGND R:");
    DEBUG_PRINT(fx_fgnd_r);
    DEBUG_PRINT(" G: ");
    DEBUG_PRINT(fx_fgnd_b);
    DEBUG_PRINT(" B: ");
    DEBUG_PRINTLN(fx_fgnd_b);

    DEBUG_PRINT("[NATS] BGND R:");
    DEBUG_PRINT(fx_bgnd_r);
    DEBUG_PRINT(" G: ");
    DEBUG_PRINT(fx_bgnd_b);
    DEBUG_PRINT(" B: ");
    DEBUG_PRINTLN(fx_bgnd_b);

    /*EEPROM.write(FX_SELECT, fx_select);
    EEPROM.write(FX_SPEED, fx_speed);
    EEPROM.write(FX_XFADE, fx_xfade);

    EEPROM.write(FX_FGND_R, fx_fgnd_r);
    EEPROM.write(FX_FGND_G, fx_fgnd_g);
    EEPROM.write(FX_FGND_B, fx_fgnd_b);

    EEPROM.write(FX_BGND_R, fx_bgnd_r);
    EEPROM.write(FX_BGND_G, fx_bgnd_g);
    EEPROM.write(FX_BGND_B, fx_bgnd_b);

    EEPROM.commit();*/
    /// TODO send FX data to wled

    nats->publish(msg.reply, "+OK");   // Not in the right mode
  }
  else
  {
    DEBUG_PRINTLN("[NATS] not in a FX mode, leaving");
    nats->publish(msg.reply, "NOK, not in FX mode");   // Not in the right mode
  }
}

/* This is the name callback routine
max 32 byte name message, ascii encoded
*/
void nats_name_handler(NATS::msg msg) { 
  DEBUG_PRINTLN("[NATS] Name Handler");

  if(msg.size > 32)
  {
    DEBUG_PRINTLN("[NATS] Name too long, ignoring");
    nats->publish(msg.reply, "NOK, Name too long");
    return;
  }

  if(msg.data[0] == '*')
  {
    DEBUG_PRINT("[NATS] Name request: ");
    String name;
    // send the name back
    /*for(uint i = 0; i < dev_name_length-1; i++)   // -1 because is always 0 terminated
    {
      char c = EEPROM.read(DEV_NAME + i);
      name += String(c);
    }*/
    name = cmDNS;
    DEBUG_PRINTLN(name);
    String nats_name_topic = String(natssetup.natsTopic) + String(".") + mac_string + String(".name");
    nats->publish(nats_name_topic.c_str(), name.c_str());
    return;
  }
  if(msg.size > 1)
  {
    DEBUG_PRINT("[NATS] Name set,");
    dev_name_length = msg.size;
    // EEPROM.write(DEV_NAME_LENGTH, dev_name_length);
    DEBUG_PRINT(" length:");
    DEBUG_PRINTLN(dev_name_length);

    // set the name
    DEBUG_PRINT("[NATS] Name: ");
    DEBUG_PRINTLN(msg.data);
    
    doSerializeConfig = true;
    strlcpy( cmDNS, &msg.data[0], dev_name_length);
  
/*    for(uint c = 0; c < dev_name_length-1; c++)     // -1 because is always 0 terminated
    {
      DEBUG_PRINT(msg.data[c]);
      cmDNS[c] = msg.data[c];
      //EEPROM.write(DEV_NAME+c, msg.data[c]);
      /// TODO set cmDNS name and reinitialize MDNS as if we did the settings page
    }
    cmDNS[dev_name_length-1] = 0;*/
    DEBUG_PRINTLN(" ");
    // EEPROM.commit();
    delay(1000);    // needed for EEPROM to process
    nats->publish(msg.reply, "+OK");

    nats_announce();
  }
}

boolean natsLive = false;
unsigned long natsLastLiveTime = 0;
#define NATS_LIVE_INTERVAL 40

void sendDataNats( NATS::msg msg)
{
  AsyncWebSocketMessageBuffer * buffer;

  if (!requestJSONBufferLock(12)) return;

  JsonObject state = doc.createNestedObject("state");
  serializeState(state);
  JsonObject info  = doc.createNestedObject("info");
  serializeInfo(info);

  size_t len = measureJson(doc);
  DEBUG_PRINTF("JSON buffer size: %u for WS request (%u).\n", doc.memoryUsage(), len);

  size_t heap1 = ESP.getFreeHeap();
  DEBUG_PRINT(F("heap ")); DEBUG_PRINTLN(ESP.getFreeHeap());
  #ifdef ESP8266
  if (len>heap1) {
    DEBUG_PRINTLN(F("Out of memory (WS)!"));
    nats->publish(msg.reply, F("{\"Success\":false,\"Error\":\"Out of memory\"}"));
    return;
  }
  #endif
  buffer = ws.makeBuffer(len); // will not allocate correct memory sometimes on ESP8266
  #ifdef ESP8266
  size_t heap2 = ESP.getFreeHeap();
  DEBUG_PRINT(F("heap ")); DEBUG_PRINTLN(ESP.getFreeHeap());
  #else
  size_t heap2 = 0; // ESP32 variants do not have the same issue and will work without checking heap allocation
  #endif
  if (!buffer || heap1-heap2<len) {
    releaseJSONBufferLock();
    DEBUG_PRINTLN(F("Nats WLED buffer allocation failed."));
    nats->publish(msg.reply, F("{\"Success\":false,\"Error\":\"Out of memory\"}"));
    return; //out of memory

  }

  buffer->lock();
  serializeJson(doc, (char *)buffer->get(), len);
  String natspath = natssetup.natsTopic + String(".") + natssetup.natsGroup + String(".devices.") + mac_string;
  String status_topic = natspath + String(".status");
  
  DEBUG_PRINT(F("Sending NATS data "));
  DEBUG_PRINTLN(status_topic);
  DEBUG_PRINTLN(msg.reply);
  DEBUG_PRINTLN((char *)buffer->get());

  nats->publish(status_topic.c_str(), F("{\"Success\":false,\"Error\":\"Out of memory\"}"));
  if (msg.reply){
    nats->publish(msg.reply,(char *)buffer->get());
  }
  else
    nats->publish(status_topic.c_str(),(char *)buffer->get());
  
  buffer->unlock();
  releaseJSONBufferLock();
}

void nats_WLED_handler(NATS::msg msg) { 
  DEBUG_PRINTLN("[NATS] WLED Handler");
  DEBUG_PRINT("GOT MESSAGE FROM ") ;
  DEBUG_PRINTLN(msg.reply) ;

  if (msg.size < 10 && msg.data[0] == 'p') {
    // application layer ping/pong heartbeat.
    // client-side socket layer ping packets are unanswered (investigate)
    nats->publish(msg.reply, "pong");
    return;
  }

  bool verboseResponse = false;
  if (!requestJSONBufferLock(11)) return;

  DeserializationError error = deserializeJson(doc, msg.data, msg.size);
  JsonObject root = doc.as<JsonObject>();
  if (error || root.isNull()) {
    releaseJSONBufferLock();
    nats->publish(msg.reply, F("{\"Success\":false,\"Error\":\"Empty document or deserialization error\"}"));
    return;
  }
  if (root["v"] && root.size() == 1) {
    //if the received value is just "{"v":true}", send only to this client
    verboseResponse = true;
  } else if (root.containsKey("lv")) {
    natsLive = root["lv"] ;
  } else {
    verboseResponse = deserializeState(root);
  }
  releaseJSONBufferLock(); // will clean fileDoc

  if (!interfaceUpdateCallMode) { // individual client response only needed if no WS broadcast soon
    if (verboseResponse) {
      sendDataNats(msg);
    } else {
      // we have to send something back otherwise WS connection closes
      nats->publish(msg.reply, F("{\"success\":true}"));
    }
  }
}




bool sendLiveLedsNats()
{
  size_t used = strip.getLengthTotal();
#ifdef ESP8266
  const size_t MAX_LIVE_LEDS_WS = 256U;
#else
  const size_t MAX_LIVE_LEDS_WS = 1024U;
#endif
  size_t n = ((used -1)/MAX_LIVE_LEDS_WS) +1; //only serve every n'th LED if count over MAX_LIVE_LEDS_WS
  size_t pos = (strip.isMatrix ? 4 : 2);  // start of data
  size_t bufSize = pos + (used/n)*3;

  AsyncWebSocketMessageBuffer * wsBuf = ws.makeBuffer(bufSize);
  if (!wsBuf) return false; //out of memory
  uint8_t* buffer = wsBuf->get();
  buffer[0] = 'L';
  buffer[1] = 1; //version

#ifndef WLED_DISABLE_2D
  size_t skipLines = 0;
  if (strip.isMatrix) {
    buffer[1] = 2; //version
    buffer[2] = Segment::maxWidth;
    buffer[3] = Segment::maxHeight;
    if (used > MAX_LIVE_LEDS_WS*4) {
      buffer[2] = Segment::maxWidth/4;
      buffer[3] = Segment::maxHeight/4;
      skipLines = 3;
    } else if (used > MAX_LIVE_LEDS_WS) {
      buffer[2] = Segment::maxWidth/2;
      buffer[3] = Segment::maxHeight/2;
      skipLines = 1;
    }
  }
#endif

  for (size_t i = 0; pos < bufSize -2; i += n)
  {
#ifndef WLED_DISABLE_2D
    if (strip.isMatrix && skipLines) {
      if ((i/Segment::maxWidth)%(skipLines+1)) i += Segment::maxWidth * skipLines;
    }
#endif
    uint32_t c = strip.getPixelColor(i);
    uint8_t r = R(c);
    uint8_t g = G(c);
    uint8_t b = B(c);
    uint8_t w = W(c);
    buffer[pos++] = scale8(qadd8(w, r), strip.getBrightness()); //R, add white channel to RGB channels as a simple RGBW -> RGB map
    buffer[pos++] = scale8(qadd8(w, g), strip.getBrightness()); //G
    buffer[pos++] = scale8(qadd8(w, b), strip.getBrightness()); //B
  }

  /// TODO  send buffer to Nats server
  String natspath = natssetup.natsTopic + String(".") + natssetup.natsGroup + String(".devices.") + mac_string;
  
  String live_topic = natspath + String(".WLED.Live");
  DEBUG_PRINTLN(live_topic);
  nats->publish(live_topic.c_str(), buffer);
  return true;
}

void handleLiveViewNats()
{
  if (millis() - natsLastLiveTime > NATS_LIVE_INTERVAL)
  {
    bool success = true;
    if (natsLive) success = sendLiveLedsNats();
    natsLastLiveTime = millis();
    if (!success) natsLastLiveTime -= 20; //try again in 20ms if failed due to non-empty WS queue
  }
}
