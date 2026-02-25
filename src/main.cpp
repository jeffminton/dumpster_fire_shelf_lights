// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// Released under the GPLv3 license to match the rest of the
// Adafruit NeoPixel library
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "EspHtmlTemplateProcessor.h"
// include library to read and write from flash memory
#include <EEPROM.h>
#include "index.h" //Our HTML webpage contents
#include <Adafruit_NeoPixel.h>
#include <uri/UriBraces.h>

#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN         D1 // On Trinket or Gemma, suggest changing this to 1
#define STATUS_LED_PN D4
#define RESET_PIN   0

#define BUTTON_PRESS_MIN_LENGTH_MS 5000

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 9 // Popular NeoPixel ring size

// define the number of bytes you want to access
#define EEPROM_SIZE (NUMPIXELS * 3) + 4 

//SSID and Password of your WiFi router
const char* ssid = "enginerdy";
const char* password = "chemicalcomputer";
String mac_address;
char* board_name;
char* host_name;

const int start_eeprom_address = 0;

const long utcOffsetInSeconds = -25200;

// const long ntp_update_delay = 300000;
const long ntp_update_delay = 10000;
long last_ntp_update = -1;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

ESP8266WebServer server(80); //Server on port 80
EspHtmlTemplateProcessor templateProcessor(&server);

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, ntp_update_delay);

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 50 // Time (in milliseconds) to pause between pixels



int default_color_array[][3] = {
    {  0, 150,   0}, {150,   0,   0}, {  0,   0, 150},
    {150, 150,   0}, {150,   0, 105}, {  0, 150, 150},
    {250,  50, 250}, { 50, 250, 250}, {250, 250,  50}
};
int default_on_hour = 6, default_on_minutes = 0;
int default_off_hour = 20, default_off_minutes = 0;
int default_use_schedule = 1;

int color_array[][3] = {
    {  0, 150,   0}, {150,   0,   0}, {  0,   0, 150},
    {150, 150,   0}, {150,   0, 105}, {  0, 150, 150},
    {250,  50, 250}, { 50, 250, 250}, {250, 250,  50}
};

int on_hour = -1, on_minutes = -1;
int off_hour = -1, off_minutes = -1;
int curr_hour = -1, curr_minutes = -1;
int on_seconds_since_midnight = -1, off_seconds_since_midnight = -1;
int curr_seconds_since_midnight = -1, prev_seconds_since_midnight = -1;
int use_schedule = -1;


String color_form_names[] = {
    "led_zero",
    "led_one",
    "led_two",
    "led_three",
    "led_four",
    "led_five",
    "led_six",
    "led_seven",
    "led_eight"
};


char page_buffer[sizeof(MAIN_page)];

bool update_colors = true;
bool leds_on = false;
bool button_is_pressed = false;
bool identify = false;

int seconds_since_midnight(int hour, int minute) {
    return (hour * 60 * 60) + (minute * 60);
}


void save_led_values() {
    int address = start_eeprom_address;

    for ( int i = 0; i < (int) (sizeof(color_form_names) / sizeof(String)); i++ ){
        for( int j = 0; j < 3; j++) {
            EEPROM.write(address, color_array[i][j]);
            address++;
        }
    }

    EEPROM.write(address, on_hour);
    address++;
    EEPROM.write(address, on_minutes);
    address++;
    EEPROM.write(address, off_hour);
    address++;
    EEPROM.write(address, off_minutes);
    address++;
    EEPROM.write(address, use_schedule);
    address++;

    EEPROM.commit();
}



void load_led_values() {
    int address = start_eeprom_address;

    for ( int i = 0; i < (int) (sizeof(color_form_names) / sizeof(String)); i++ ){
        for( int j = 0; j < 3; j++) {
            color_array[i][j] = EEPROM.read(address);
            address++;
        }
    }

    on_hour = EEPROM.read(address);
    address++;
    on_minutes = EEPROM.read(address);
    address++;
    off_hour = EEPROM.read(address);
    address++;
    off_minutes = EEPROM.read(address);
    address++;
    use_schedule = EEPROM.read(address);
    if(use_schedule > 1) {
        use_schedule = default_use_schedule;
    }
    address++;

    on_seconds_since_midnight = seconds_since_midnight(on_hour, on_minutes);
    off_seconds_since_midnight = seconds_since_midnight(off_hour, off_minutes);

}



void reset_saved_to_defaults() {
    for(int i = 0; i < pixels.numPixels(); i++) { // For each pixel...
        color_array[i][0] = default_color_array[i][0];
        color_array[i][1] = default_color_array[i][1];
        color_array[i][2] = default_color_array[i][2];
    }

    on_hour = default_on_hour;
    on_minutes = default_on_minutes;
    off_hour = default_off_hour;
    off_minutes = default_off_minutes;
    use_schedule = default_use_schedule;

    update_colors = true;
    save_led_values();
    load_led_values();
}



boolean button_pressed( int button_pin, int desired_value ) {
    unsigned long start_time = millis();

    int pin_value = digitalRead( button_pin );

    while( (millis() - start_time) < (unsigned long) BUTTON_PRESS_MIN_LENGTH_MS ) {
        pin_value = digitalRead( button_pin );
        yield();
        if( pin_value != desired_value ) {
            return false;
        }
    }

    return pin_value == desired_value;
} 




void check_reset_button_pressed() {
    button_is_pressed = button_pressed( RESET_PIN, LOW);
    if(button_is_pressed) {
        reset_saved_to_defaults();
    }
    while(button_is_pressed == true) {
        button_is_pressed = button_pressed( RESET_PIN, LOW);
    }
}




char* get_shelf_name(){
    String test_mac_string = String("b4:e6:2d:78:20:48");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"Dumpster Shelf 1";
    }
    
    test_mac_string = String("a4:cf:12:b9:26:9b");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"Dumpster Shelf 2";
    }
    
    test_mac_string = String("bc:dd:c2:47:67:47");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"Dumpster Shelf 3";
    }

    test_mac_string = String("a4:cf:12:b9:26:7b");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"Dumpster Shelf 4";
    }

    return (char*)"Shelf Name Not Found";
}




char* get_host_name(){
    String test_mac_string = String("b4:e6:2d:78:20:48");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"dumpster_shelf_1";
    }
    
    test_mac_string = String("a4:cf:12:b9:26:9b");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"dumpster_shelf_2";
    }
    
    test_mac_string = String("bc:dd:c2:47:67:47");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"dumpster_shelf_3";
    }

    test_mac_string = String("a4:cf:12:b9:26:7b");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"dumpster_shelf_4";
    }

    return (char*)"unknown";
}



char* color_to_hex_string(int color_index) {
    char *hex_buffer = (char*)malloc(sizeof(char) * 10);

    sprintf(hex_buffer, "#%02X%02X%02X", color_array[color_index][0], color_array[color_index][1], color_array[color_index][2]);

    return hex_buffer;
}


char* time_to_string(int hour, int minutes) {
    char *hex_buffer = (char*)malloc(sizeof(char) * 10);

    sprintf(hex_buffer, "%02d:%02d", hour, minutes);

    return hex_buffer;
}



// Returning the substitution value of a specific keyword
String indexKeyProcessor(const String& key)
{
    if (key == "NAME") {
        return board_name;
    } else if (key == "LIGHT_STATE") {
        return String(leds_on).c_str();
    } else if (key == "ON_SECONDS_SET") {
        return String(on_seconds_since_midnight).c_str();
    } else if (key == "OFF_SECONDS_SET") {
        return String(off_seconds_since_midnight).c_str();
    } else if (key == "CURR_SECONDS") {
        return String(curr_seconds_since_midnight).c_str();
    } else if (key == "PREV_SECONDS_SET") {
        return String(prev_seconds_since_midnight).c_str();
    } else if (key == "LED_0") {
        return color_to_hex_string(0);
    } else if (key == "LED_1") {
        return color_to_hex_string(1);
    } else if (key == "LED_2") {
        return color_to_hex_string(2);
    } else if (key == "LED_3") {
        return color_to_hex_string(3);
    } else if (key == "LED_4") {
        return color_to_hex_string(4);
    } else if (key == "LED_5") {
        return color_to_hex_string(5);
    } else if (key == "LED_6") {
        return color_to_hex_string(6);
    } else if (key == "LED_7") {
        return color_to_hex_string(7);
    } else if (key == "LED_8") {
        return color_to_hex_string(8);
    } else if (key == "ON_TIME") {
        return time_to_string(on_hour, on_minutes);
    } else if (key == "OFF_TIME") {
        return time_to_string(off_hour, off_minutes);
    } else if (key == "USE_SCHEDULE") {
        // return use_schedule == 1 ? "value=\"checked\"" : "";
        // return use_schedule == 1 ? "value=\"\" checked=\"\"" : "";
        return use_schedule == 1 ? "checked" : "";
    } else if (key == "USE_SCHEDULE_DEBUG") {
        // return use_schedule == 1 ? "value=\"checked\"" : "";
        return String(use_schedule).c_str();
    } else if (key == "HOST_NAME") {
        return host_name;
    } else if (key == "MAC_ADDRESS") {
        return mac_address;
    }
  return "Key not found";
}




void handleRoot() {
    yield();
    templateProcessor.processAndSend("/index.html", indexKeyProcessor);
}


// Hex string to long conversion
int hstol(String recv){
    char c[recv.length() + 1];
    recv.toCharArray(c, recv.length() + 1);
    return (int) strtol(c, NULL, 16); 
}

// Decimal string to long conversion
int dstol(String recv){
    char c[recv.length() + 1];
    recv.toCharArray(c, recv.length() + 1);
    return (int) strtol(c, NULL, 10); 
}



void identify_flash() {
    //Flash all the pixel red for 1 second then off for 1 second 10 times
    identify = false;
    for(int i=0; i<5; i++) {
        yield();
        for(int j=0; j<pixels.numPixels(); j++) {
            pixels.setPixelColor(j, pixels.Color(150, 0, 0)); // Red color
        }
        pixels.show();
        delay(500);

        pixels.clear();
        pixels.show();
        delay(500);
    }

    leds_on = true;
    update_colors = true;
}




void handle_update_timer() {
    yield();
    String on_time, off_time;
    
    on_time = server.arg("on_time");
    off_time = server.arg("off_time");
    use_schedule = server.hasArg("use_schedule") ? 1 : 0;

    on_hour = dstol(on_time.substring(0, 2));
    on_minutes = dstol(on_time.substring(3));
    off_hour = dstol(off_time.substring(0, 2));
    off_minutes = dstol(off_time.substring(3));
    
    on_seconds_since_midnight = seconds_since_midnight(on_hour, on_minutes);
    off_seconds_since_midnight = seconds_since_midnight(off_hour, off_minutes);

    save_led_values();

    templateProcessor.processAndSend("/index.html", indexKeyProcessor);
}



void handle_update_colors() {
    yield();
    String hex_color_string, arg_name, red_str, green_str, blue_str;
    long red, green, blue;


    for ( int i = 0; i < (int) (sizeof(color_form_names) / sizeof(String)); i++ ){
        arg_name = color_form_names[i];
        if( server.hasArg(arg_name) ){
            hex_color_string = server.arg(arg_name);

            red = hstol(hex_color_string.substring(1, 3));
            
            green = hstol(hex_color_string.substring(3, 5));

            blue = hstol(hex_color_string.substring(5, 7));

            color_array[i][0] = red;
            color_array[i][1] = green;
            color_array[i][2] = blue;
        }
    }

    update_colors = true;
    save_led_values();

    templateProcessor.processAndSend("/index.html", indexKeyProcessor);
}


void handle_update_color(int led_index, String hex_color_string) {
    yield();
    String red_str, green_str, blue_str;
    long red, green, blue;

    if( server.hasArg("color") ){

        red = hstol(hex_color_string.substring(1, 3));
        
        green = hstol(hex_color_string.substring(3, 5));

        blue = hstol(hex_color_string.substring(5, 7));

        color_array[led_index][0] = red;
        color_array[led_index][1] = green;
        color_array[led_index][2] = blue;
    }

    update_colors = true;
    save_led_values();

    server.send(200, "text/plain", hex_color_string);
}


void handle_leds_on() {
    yield();
    leds_on = true;
    update_colors = true;

    server.send(200, "text/plain", "1");
}

void handle_leds_off() {
    yield();
    leds_on = false;
    update_colors = true;

    server.send(200, "text/plain", "0");
}

void handle_identify() {
    identify = true;
    server.send(200, "text/plain", "1");
}

void handleNotFound(){
    server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}


void setup() {

    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(STATUS_LED_PN, OUTPUT);

    digitalWrite(STATUS_LED_PN, LOW);

    EEPROM.begin(EEPROM_SIZE);
    
    Serial.begin(9600);

    LittleFS.begin();
  
    WiFi.begin(ssid, password);     //Connect to your WiFi router
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }    
    mac_address = WiFi.macAddress();
    mac_address.toLowerCase();
    Serial.println(mac_address);
    
    board_name = get_shelf_name();
    host_name = get_host_name();

    Serial.print("Baord Name: ");
    Serial.println(board_name);


    ArduinoOTA.setHostname(host_name);
    ArduinoOTA.setPassword("esp8266");

    ArduinoOTA.onStart([]() {
        Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();


    

    server.on("/", handleRoot);
    server.on("/", HTTP_GET, handleRoot); 
    server.on("/update_colors", HTTP_POST, handle_update_colors);
    server.on("/update_timer", HTTP_POST, handle_update_timer);
    server.on("/leds_on", HTTP_GET, handle_leds_on);
    server.on("/leds_off", HTTP_GET, handle_leds_off);
    server.on("/identify", HTTP_GET, handle_identify);
    server.on(UriBraces("/update_color/{}/{}"), []() {
        String led = server.pathArg(0);
        String color = server.pathArg(1);
        handle_update_color(dstol(led), color);

        // server.send(200, "text/plain", "User: '" + user + "'");
    });
    server.onNotFound(handleNotFound);


    server.begin();                  //Start server
    Serial.println("HTTP server started");

    // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
    // Any other board, you can remove this part (but no harm leaving it):
    #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
        clock_prescale_set(clock_div_1);
    #endif
    // END of Trinket-specific code.


    load_led_values();

    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

    timeClient.begin();

    digitalWrite(STATUS_LED_PN, HIGH);
}

void loop() {
    server.handleClient();          //Handle client requests
    yield();
    ArduinoOTA.handle();
    yield();

    check_reset_button_pressed();

    timeClient.update();

    curr_hour = timeClient.getHours();
    curr_minutes = timeClient.getMinutes();
    curr_seconds_since_midnight = seconds_since_midnight(curr_hour, curr_minutes);

    if( use_schedule == 1 && curr_seconds_since_midnight != prev_seconds_since_midnight ) {
        prev_seconds_since_midnight = curr_seconds_since_midnight;

        digitalWrite(STATUS_LED_PN, LOW);

        if(
            curr_seconds_since_midnight >= on_seconds_since_midnight && 
            curr_seconds_since_midnight < off_seconds_since_midnight
        ) {
            if( leds_on == false ) {
                leds_on = true;
                update_colors = true;
            }
        } else if( leds_on == true ) {
            leds_on = false;
            update_colors = true;
        }

        digitalWrite(STATUS_LED_PN, HIGH);

    }

    if( identify == true ) {
        identify_flash();
    }

    if( update_colors == true ) {
        if( leds_on == true ) {
            Serial.println("Turn ON LEDs");
            load_led_values();
            for(int i = 0; i < pixels.numPixels(); i++) { // For each pixel...
                pixels.setPixelColor(i, pixels.Color(color_array[i][0], color_array[i][1], color_array[i][2]));
            }
        } else if ( leds_on == false ) {
            Serial.println("Turn OFF LEDs");
            pixels.clear();
        }

        pixels.show();   // Send the updated pixel colors to the hardware.

        update_colors = false;
    }

}
