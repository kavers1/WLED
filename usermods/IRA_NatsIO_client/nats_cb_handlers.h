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
  announce_message += String("\"HWTYPE\":\"") + String("IRA2020") + String("\",");        // Add HW TYPE
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
      
  }
  // TODO check what this is abaoutw
  name = cmDNS;
    
  announce_message += String("\"NAME\":\"") + name + String("\",");
  // TODO: Add everything in EEPROM,  ...
  announce_message += String("\"pixel_length\": ") + pixel_length + String(",");
  announce_message += String("\"fx\": ") + fx_select + String(",");
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
  String announce_topic = String(NATS_ROOT_TOPIC) + String(".announce");
  DEBUG_PRINTLN(announce_topic);
  nats.publish(announce_topic.c_str(), announce_message.c_str());
}

void nats_publish_status ()
{
  String status_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".status");
  long rssi = WiFi.RSSI();
  String status_message = String("{\"rssi\": \"") + String(rssi) + String("\"}");

  nats.publish(status_topic.c_str(), status_message.c_str());
}

void nats_publish_ext_mode(uint mode)
{
  String ext_mode_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".ext_mode");
  nats.publish(ext_mode_topic.c_str(), String(mode, DEC).c_str());
}

void nats_publish_ir(uint16_t packet, uint8_t teamnr)
{
  String ir_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".ir");

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

  nats.publish(ir_topic.c_str(), ir_message.c_str());
}

void nats_ping_handler(NATS::msg msg) {
    DEBUG_PRINTLN("[NATS] ping message received");

    delay(random(0,1000));      // random delay up to a sec to avoid broadcast storm

    nats_announce();
}

// This blinks the on-board debug LED a defined number of times (in the message) for board identification
void nats_debug_blink_handler(NATS::msg msg) {
  DEBUG_PRINTLN("[NATS] debug led blink message received");
  nats.publish(msg.reply, "received!");

	int count = atoi(msg.data);
	while (count-- > 0) {
		digitalWrite(15, LOW);
		delay(100);
		digitalWrite(15, HIGH);
		delay(100);
	}
}

// This sets the operation mode of the board
void nats_mode_handler(NATS::msg msg) { 
  if(nats_mode != atoi(msg.data))
  {
    // TODO check if mode is valid
    DEBUG_PRINT("[NATS] mode changed to: ");
    DEBUG_PRINTLN(msg.data);

    nats_mode = atoi(msg.data);
    EEPROM.write(NATS_MODE, nats_mode);
    EEPROM.commit();

    printMode(nats_mode);

    // make sure we switch IR reception back off
    if((nats_mode != MODE_RGB_TO_PIXELS_W_IR ) && (nats_mode != MODE_FX_TO_PIXELS_W_IR))
    {
      ir_delay = 0;
    }
  }
  nats.publish(msg.reply, "+OK");
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
  
  EEPROM.write(param_index, dec_data[2]);
  EEPROM.commit();

  nats.publish(msg.reply, "+OK"); 
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
      nats.publish(msg.reply, "NOK");   // Not in the right mode
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
    nats.publish(msg.reply, "NOK");   // Not in the right mode
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
    nats.publish(msg.reply, "NOK, Not in DMX mode");
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
      nats.publish(msg.reply, "NOK, IR overruled");   // IR overruled
      return;
    }

    uint8_t dec_data[msg.size];
    uint8_t dec_length = decode_base64((unsigned char *) msg.data, dec_data);
    uint16_t pix_len = (dec_data[2] << 8) + dec_data[3];

    // Exit ASAP
    if(pix_len > MAX_PIXELS)
    {
      DEBUG_PRINTLN("[NATS] Too many pixels");
      nats.publish(msg.reply, "NOK, Too many pixels");   // Not in the right mode
      return;
    }

    if(pix_len == 0)
    {
      DEBUG_PRINTLN("[NATS] Length is 0");
      nats.publish(msg.reply, "NOK, Length is 0");   // Not ok
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
    
    DEBUG_PRINT("[NATS] RGB Per Pixel Datapoints: ");
    DEBUG_PRINTLN(pix_len);                            // @TODO: This needs to be checked & used!, currently A and W are ditched



    for(int led_index = 0; led_index < pix_len; led_index++) // from 0 increment with #data per pixel
    {
      uint pixel = led_index + pix_offset;
      strip.setPixelColor(pixel,
                        dec_data[4 + (led_index*3)],
                        dec_data[4 + (led_index*3)]+1,
                        dec_data[4 + (led_index*3)]+2);
      //leds[pixel].r = dec_data[4 + (led_index*3)];                    // actual data starts at 4th byte
      //leds[pixel].g = dec_data[4 + (led_index*3) + 1];
      //leds[pixel].b = dec_data[4 + (led_index*3) + 2];   
      
      uint32_t clr = strip.getPixelColor(pixel_length);

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
    nats.publish(msg.reply, "+OK");  
  }
  else
  {
    DEBUG_PRINTLN("[NATS] not in a RGB mode, leaving");
    nats.publish(msg.reply, "NOK, not in a RGB mode");   // Not in the right mode
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
      nats.publish(msg.reply, "NOK, Too many pixels");   // Not in the right mode
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
    nats.publish(msg.reply, "+OK");  
  }
  else
  {
    DEBUG_PRINTLN("[NATS] not in a RGB mode, leaving");

    nats.publish(msg.reply, "NOK, not in RGB mode");   // Not in the right mode
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
      nats.publish(msg.reply, "NOK, base64 error");
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

    EEPROM.write(FX_SELECT, fx_select);
    EEPROM.write(FX_SPEED, fx_speed);
    EEPROM.write(FX_XFADE, fx_xfade);

    EEPROM.write(FX_FGND_R, fx_fgnd_r);
    EEPROM.write(FX_FGND_G, fx_fgnd_g);
    EEPROM.write(FX_FGND_B, fx_fgnd_b);

    EEPROM.write(FX_BGND_R, fx_bgnd_r);
    EEPROM.write(FX_BGND_G, fx_bgnd_g);
    EEPROM.write(FX_BGND_B, fx_bgnd_b);

    EEPROM.commit();

    nats.publish(msg.reply, "+OK");   // Not in the right mode
  }
  else
  {
    DEBUG_PRINTLN("[NATS] not in a FX mode, leaving");
    nats.publish(msg.reply, "NOK, not in FX mode");   // Not in the right mode
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
    nats.publish(msg.reply, "NOK, Name too long");
    return;
  }

  if(msg.data[0] == '*')
  {
    DEBUG_PRINT("[NATS] Name request: ");
    String name;
    // send the name back
    for(uint i = 0; i < dev_name_length-1; i++)   // -1 because is always 0 terminated
    {
      char c = EEPROM.read(DEV_NAME + i);
      name += String(c);
    }
    DEBUG_PRINTLN(name);
    String nats_name_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".name");
    nats.publish(nats_name_topic.c_str(), name.c_str());
    return;
  }
  if(msg.size > 1)
  {
    DEBUG_PRINT("[NATS] Name set,");
    dev_name_length = msg.size;
    EEPROM.write(DEV_NAME_LENGTH, dev_name_length);
    DEBUG_PRINT(" length:");
    DEBUG_PRINTLN(dev_name_length);

    // set the name
    DEBUG_PRINT("[NATS] Name: ");

    for(uint c = 0; c < dev_name_length-1; c++)     // -1 because is always 0 terminated
    {
      DEBUG_PRINT(msg.data[c]);
      //EEPROM.write(DEV_NAME+c, msg.data[c]);
    }
    DEBUG_PRINTLN(" ");
    EEPROM.commit();
    delay(1000);    // needed for EEPROM to process
    nats.publish(msg.reply, "+OK");

    nats_announce();
  }
}