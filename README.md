# ClockArduino
A seven-segment clock with 6 indicators on an address LED strip with a web page for changing color and brightness, as well as time polls via NTP.

#define DHT_PIN 2              //pin connection of the temperature sensor
#define NUM_LEDS 88          // number of LEDs
#define COLOR_ORDER GRB       // determining the order of colors
#define DATA_PIN 6            // pin connection of the LED strip
#define TIMEZONE 7            // choosing a time zone

const char* ssid = "*******";      // Wi-Fi network SSID for internet access via udp
const char* password = "*******";  // password
const char* id = "ClockRGB"; // SSID Wi-Fi network arduino
const char* pass = "Space220"; //password
