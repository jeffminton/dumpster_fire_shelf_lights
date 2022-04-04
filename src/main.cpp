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
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN         D1 // On Trinket or Gemma, suggest changing this to 1
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
    {20, 0, 0},
    {  0, 150,   0}, {150,   0,   0}, {  0,   0, 150},
    {150, 150,   0}, {150,   0, 105}, {  0, 150, 150},
    {250,  50, 250}, { 50, 250, 250}, {250, 250,  50}
};
int default_on_hour = 6, default_on_minutes = 0;
int default_off_hour = 20, default_off_minutes = 0;

int color_array[][3] = {
    {20, 0, 0},
    {  0, 150,   0}, {150,   0,   0}, {  0,   0, 150},
    {150, 150,   0}, {150,   0, 105}, {  0, 150, 150},
    {250,  50, 250}, { 50, 250, 250}, {250, 250,  50}
};

int on_hour = -1, on_minutes = -1;
int off_hour = -1, off_minutes = -1;
int curr_hour = -1, curr_minutes = -1;
int on_seconds_since_midnight = -1, off_seconds_since_midnight = -1;
int curr_seconds_since_midnight = -1, prev_seconds_since_midnight = -1;


String color_form_names[] = {
    "converter",
    "led_one",
    "led_two",
    "led_three",
    "led_four",
    "led_five",
    "led_six",
    "led_seven",
    "led_eight",
    "led_nine"
};


char page_buffer[sizeof(MAIN_page)];

bool update_colors = true;
bool leds_on = false;
bool button_is_pressed = false;


int seconds_since_midnight(int hour, int minute) {
    return (hour * 60 * 60) + (minute * 60);
}


void save_led_values() {
    int address = start_eeprom_address;

    Serial.println("Save To Flash");
    for ( int i = 0; i < (int) (sizeof(color_form_names) / sizeof(String)); i++ ){
        for( int j = 0; j < 3; j++) {
            EEPROM.write(address, color_array[i][j]);
            Serial.print("b: ");
            Serial.print(color_array[i][j]);
            Serial.print(", ");
            address++;
        }
        Serial.println("");
    }

    EEPROM.write(address, on_hour);
    address++;
    EEPROM.write(address, on_minutes);
    address++;
    EEPROM.write(address, off_hour);
    address++;
    EEPROM.write(address, off_minutes);
    address++;

    Serial.print("on_hour: ");
    Serial.println(on_hour);
    Serial.print("on_minutes: ");
    Serial.println(on_minutes);
    Serial.print("off_hour: ");
    Serial.println(off_hour);
    Serial.print("off_minutes: ");
    Serial.println(off_minutes);

    EEPROM.commit();
}



void load_led_values() {
    int address = start_eeprom_address;

    Serial.println("Load From Flash");
    for ( int i = 0; i < (int) (sizeof(color_form_names) / sizeof(String)); i++ ){
        for( int j = 0; j < 3; j++) {
            color_array[i][j] = EEPROM.read(address);
            // Serial.print("b: ");
            // Serial.print(color_array[i][j]);
            // Serial.print(", ");
            address++;
        }
        // Serial.println("");
    }

    on_hour = EEPROM.read(address);
    address++;
    on_minutes = EEPROM.read(address);
    address++;
    off_hour = EEPROM.read(address);
    address++;
    off_minutes = EEPROM.read(address);
    address++;

    on_seconds_since_midnight = seconds_since_midnight(on_hour, on_minutes);
    off_seconds_since_midnight = seconds_since_midnight(off_hour, off_minutes);

    // Serial.print("on_hour: ");
    // Serial.println(on_hour);
    // Serial.print("on_minutes: ");
    // Serial.println(on_minutes);
    // Serial.print("off_hour: ");
    // Serial.println(off_hour);
    // Serial.print("off_minutes: ");
    // Serial.println(off_minutes);
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

    update_colors = true;
    save_led_values();
    load_led_values();
}



boolean button_pressed( int button_pin, int desired_value ) {
    unsigned long start_time = millis();

    int pin_value = digitalRead( button_pin );

    // Serial.println("Pressed Button");

    while( (millis() - start_time) < (unsigned long) BUTTON_PRESS_MIN_LENGTH_MS ) {
        pin_value = digitalRead( button_pin );
        yield();
        if( pin_value != desired_value ) {
            // Serial.println( "button_pressed: button not pressed" );
            return false;
        }
    }

    // Serial.print("Time since func start (millis() - start_time): ");
    // Serial.print((millis() - start_time));
    // Serial.print( "  digital value: " );
    // Serial.print(digitalRead( button_pin ));
    // Serial.print( "  pin value: " );
    // Serial.print(pin_value);
    // Serial.print( "  desired value: " );
    // Serial.print(desired_value);
    // Serial.print( "  button_pressed: " );
    // Serial.println(pin_value == desired_value);

    return pin_value == desired_value;
} 




void check_reset_button_pressed() {
    button_is_pressed = button_pressed( RESET_PIN, LOW);
    if(button_is_pressed) {
        // Serial.print("button_is_pressed == false. ");
        // Serial.print("button press state. ");
        // Serial.println(button_is_pressed);
        reset_saved_to_defaults();
    }
    while(button_is_pressed == true) {
        button_is_pressed = button_pressed( RESET_PIN, LOW);
    }
}




char* get_shelf_name(){
    String test_mac_string = String("60:01:94:03:80:fd");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"Dumpster Shelf 1";
    }
    
    test_mac_string = String("60:01:94:01:21:cf");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"Dumpster Shelf 2";
    }
    
    test_mac_string = String("5c:cf:7f:23:f6:5b");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"Dumpster Shelf 3";
    }

    test_mac_string = String("5c:cf:7f:23:fe:5c");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"Dumpster Shelf 4";
    }

    return (char*)"unknown";
}




char* get_host_name(){
    String test_mac_string = String("60:01:94:03:80:fd");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"dumpster_shelf_1";
    }
    
    test_mac_string = String("60:01:94:01:21:cf");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"dumpster_shelf_2";
    }

    test_mac_string = String("5c:cf:7f:23:f6:5b");
    test_mac_string.toLowerCase();
    if( mac_address == test_mac_string) {
        return (char*)"dumpster_shelf_3";
    }

    test_mac_string = String("5c:cf:7f:23:fe:5c");
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
    } else if (key == "LED_9") {
        return color_to_hex_string(9);
    } else if (key == "ON_TIME") {
        return time_to_string(on_hour, on_minutes);
    } else if (key == "OFF_TIME") {
        return time_to_string(off_hour, off_minutes);
    }
  return "Key not found";
}




//===============================================================
// This routine is executed when you open its IP in browser
//===============================================================
// void refresh_page_buffer() {
//     for ( int i = 0; i < (int) (sizeof(color_form_names) / sizeof(String)); i++ ){
//         Serial.print(color_form_names[i]);

//         Serial.print(": red: ");
//         Serial.print(color_array[i][0]);

//         Serial.print(", green: ");
//         Serial.print(color_array[i][1]);

//         Serial.print(", blue: ");
//         Serial.println(color_array[i][2]);
//     }
//     sprintf(page_buffer, MAIN_page, 
//         board_name, board_name, leds_on,
//         on_seconds_since_midnight, off_seconds_since_midnight,
//         curr_seconds_since_midnight, prev_seconds_since_midnight,
//         color_array[0][0], color_array[0][1], color_array[0][2],
//         color_array[7][0], color_array[7][1], color_array[7][2],
//         color_array[6][0], color_array[6][1], color_array[6][2],
//         color_array[1][0], color_array[1][1], color_array[1][2],
//         color_array[8][0], color_array[8][1], color_array[8][2],
//         color_array[5][0], color_array[5][1], color_array[5][2],
//         color_array[2][0], color_array[2][1], color_array[2][2],
//         color_array[9][0], color_array[9][1], color_array[9][2],
//         color_array[4][0], color_array[4][1], color_array[4][2],
//         color_array[3][0], color_array[3][1], color_array[3][2],
//         on_hour, on_minutes, off_hour, off_minutes
//     );


// }




void handleRoot() {
    yield();
    // String s = MAIN_page; //Read HTML contents
    // char s[] = MAIN_page;

    
    // refresh_page_buffer();
    // server.send(200, "text/html", page_buffer); //Send web page
    templateProcessor.processAndSend("/index.html", indexKeyProcessor);
}


int hstol(String recv){
    char c[recv.length() + 1];
    recv.toCharArray(c, recv.length() + 1);
    return (int) strtol(c, NULL, 16); 
}

int dstol(String recv){
    char c[recv.length() + 1];
    recv.toCharArray(c, recv.length() + 1);
    return (int) strtol(c, NULL, 10); 
}





void handle_update_timer() {
    yield();
    String on_time, off_time;
    
    on_time = server.arg("on_time");
    off_time = server.arg("off_time");

    Serial.print("on_time: ");
    Serial.println(on_time);
    Serial.print("off_time: ");
    Serial.println(off_time);

    Serial.print("on_hour s: ");
    Serial.println(on_time.substring(0, 2));
    Serial.print("on_minutes s: ");
    Serial.println(on_time.substring(3));
    Serial.print("off_hour s: ");
    Serial.println(off_time.substring(0, 2));
    Serial.print("off_minutes s: ");
    Serial.println(off_time.substring(3));

    on_hour = dstol(on_time.substring(0, 2));
    on_minutes = dstol(on_time.substring(3));
    off_hour = dstol(off_time.substring(0, 2));
    off_minutes = dstol(off_time.substring(3));

    Serial.print("on_hour: ");
    Serial.println(on_hour);
    Serial.print("on_minutes: ");
    Serial.println(on_minutes);
    Serial.print("off_hour: ");
    Serial.println(off_hour);
    Serial.print("off_minutes: ");
    Serial.println(off_minutes);
    
    save_led_values();

    // refresh_page_buffer();
    // server.send(200, "text/html", page_buffer); //Send web page
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
            Serial.print("Hex Color: ");
            Serial.print(hex_color_string);

            red = hstol(hex_color_string.substring(1, 3));
            Serial.print(", red: ");
            Serial.print(red);

            green = hstol(hex_color_string.substring(3, 5));
            Serial.print(", green: ");
            Serial.print(green);

            blue = hstol(hex_color_string.substring(5, 7));
            Serial.print(", blue: ");
            Serial.println(blue);

            color_array[i][0] = red;
            color_array[i][1] = green;
            color_array[i][2] = blue;

        }
    }

    update_colors = true;
    save_led_values();

    // refresh_page_buffer();
    // server.send(200, "text/html", page_buffer); //Send web page
    templateProcessor.processAndSend("/index.html", indexKeyProcessor);
}

void handleNotFound(){
    server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}


void setup() {

    pinMode(RESET_PIN, INPUT_PULLUP);

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

    //If connection successful show IP address in serial monitor
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());  //IP address assigned to your ESP
    Serial.print("MAC Address: ");
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


    

    server.on("/", handleRoot);      //Which routine to handle at root location
    server.on("/", HTTP_GET, handleRoot);        // Call the 'handleRoot' function when a client requests URI "/"
    server.on("/update_colors", HTTP_POST, handle_update_colors); // Call the 'handleLogin' function when a POST request is made to URI "/login"
    server.on("/update_timer", HTTP_POST, handle_update_timer); // Call the 'handleLogin' function when a POST request is made to URI "/login"
    server.onNotFound(handleNotFound);           // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"


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

    if( curr_seconds_since_midnight != prev_seconds_since_midnight ) {
        prev_seconds_since_midnight = curr_seconds_since_midnight;

        Serial.print("leds_on: ");
        Serial.println(leds_on);
        Serial.print("curr_seconds_since_midnight: ");
        Serial.println(curr_seconds_since_midnight);
        Serial.print("on_seconds_since_midnight: ");
        Serial.println(on_seconds_since_midnight);
        Serial.print("off_seconds_since_midnight: ");
        Serial.println(off_seconds_since_midnight);

        Serial.print("curr_seconds_since_midnight >= on_seconds_since_midnight && curr_seconds_since_midnight < off_seconds_since_midnight: ");
        Serial.println(curr_seconds_since_midnight >= on_seconds_since_midnight && 
            curr_seconds_since_midnight < off_seconds_since_midnight);
        Serial.print("leds_on == true: ");
        Serial.println(leds_on == true);

        if(
            curr_seconds_since_midnight >= on_seconds_since_midnight && 
            curr_seconds_since_midnight < off_seconds_since_midnight
        ) {
            if( leds_on == false ) {
                Serial.println("Set leds_on = true");
                leds_on = true;
                update_colors = true;
            }
        } else if( leds_on == true ) {
            Serial.println("Set leds_on = false");
            leds_on = false;
            update_colors = true;
        }

        Serial.print(daysOfTheWeek[timeClient.getDay()]);
        Serial.print(", ");
        Serial.print(timeClient.getHours());
        Serial.print(":");
        Serial.print(timeClient.getMinutes());
        Serial.print(":");
        Serial.println(timeClient.getSeconds());

        Serial.print("leds_on: ");
        Serial.println(leds_on);
        Serial.print("update_colors: ");
        Serial.println(update_colors);

    }

    // pixels.clear(); // Set all pixel colors to 'off'
    // pixels.show();   // Send the updated pixel colors to the hardware.

    // delay(DELAYVAL); // Pause before next pass through loop
    // The first NeoPixel in a strand is #0, second is 1, all the way up
    // to the count of pixels minus one.
//     int firstPixelHue = 0;     // First pixel starts at red (hue 0)
//     for(int a=0; a<30; a++) {  // Repeat 30 times...
//     for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
//       pixels.clear();         //   Set all pixels in RAM to 0 (off)
//       // 'c' counts up from 'b' to end of pixels in increments of 3...
//       for(int c=b; c<pixels.numPixels(); c += 3) {
//         // hue of pixel 'c' is offset by an amount to make one full
//         // revolution of the color wheel (range 65536) along the length
//         // of the pixels (pixels.numPixels() steps):
//         int      hue   = firstPixelHue + c * 65536L / pixels.numPixels();
//         uint32_t color = pixels.gamma32(pixels.ColorHSV(hue)); // hue -> RGB
//         pixels.setPixelColor(c, color); // Set pixel 'c' to value 'color'
//       }
//       pixels.show();                // Update pixels with new contents
//       delay(50);                 // Pause for a moment
//       firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
//     }
//   }
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
