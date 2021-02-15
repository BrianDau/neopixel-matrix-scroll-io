// Adafruit_NeoMatrix scrolling message from IO feed
// Adapted from Adafruit Neomatrix matrixscroll example code
// Uses 8 columns of 8 Neopixel strands wired in 
// zigzag formation to an ESP8266 Wemos D1 mini

/************************ Adafruit IO Configuration *******************************/

// visit io.adafruit.com if you need to create an account,
// or if you need your Adafruit IO key.
#define IO_USERNAME    "YOUR-IO-NAME"
#define IO_KEY         "YOUR-IO-KEY"

/******************************* WIFI Configuration **************************************/

#define WIFI_SSID       "YOUR-SSID"
#define WIFI_PASS       "YOUR-PASSWORD"

#include "AdafruitIO_WiFi.h"
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);

/************************ Main Program Starts Here *******************************/
#include <ESP8266WiFi.h>
#include <AdafruitIO.h>
#include <Adafruit_MQTT.h>
#include <ArduinoHttpClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#ifndef PSTR
#define PSTR // Make Arduino Due happy
#endif

#define PIN 6

// ESP8266 has an I2S neopixel library which can only use pin RX
// so it's recommended to use the same pin with Neopixel to avoid
// rewiring when changing libs
#ifdef ESP8266
#define PIN RX
#endif

// MATRIX DECLARATION:
// Parameter 1 = width of NeoPixel matrix
// Parameter 2 = height of matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_GRBW    Pixels are wired for GRBW bitstream (RGB+W NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)


// The way I've wired the matrix has the ESP8266 at the bottom right
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, PIN,
                            NEO_MATRIX_BOTTOM     + NEO_MATRIX_RIGHT +
                            NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
                            NEO_GRB            + NEO_KHZ800);

const uint16_t colors[] = {
  matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255)
};

// You can follow the instructions on this page: https://learn.adafruit.com/iot-led-sign/overview
// to setup your feeds and a dashboard in a manner similar to what I have below
// I also enabled Webhooks on the signtext feed to allow me to run 
// an iOS shortcut to send new text to the feed.
AdafruitIO_Feed *signtext = io.feed("sign-quotes.signtext"); // set up the 'signtext' feed
AdafruitIO_Feed *signcolor = io.feed("sign-quotes.signcolor"); // set up the 'signcolor' feed

void setup() {
  // start the serial connection
  Serial.begin(115200);

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(40);
  matrix.setTextColor(colors[0]);

  // connect to io.adafruit.com
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  // set up a message handler for the 'signtext' feed.
  // the handleMessage function (defined below)
  // will be called whenever a message is
  // received from adafruit io.
  signtext->onMessage(handleMessage);

  // set up a message handler for the 'signcolor' feed.
  // the handleColor function (defined below)
  // will be called whenever a color update is
  // received from adafruit io.
  signcolor->onMessage(handleColor);

  // wait for a connection
  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
  displayData();
}


int x    = matrix.width();
int pass = 0;
static String theText = "HELLO WORLD!";
static int bufferOffset = 7;

void loop() {
  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();
}

// this function is called whenever a message
// is received from Adafruit IO. it was attached to
// the signtext feed in the setup() function above.
void handleMessage(AdafruitIO_Data *data) {

  lightPixels(colors[0]); // reset all pixels to red
  delay(500);
  lightPixels(0); // reset all pixels to off
  delay(500);
  lightPixels(colors[0]); // reset all pixels to red
  delay(500);
  lightPixels(0); // reset all pixels to off
  delay(500);
  lightPixels(colors[0]); // reset all pixels to red
  delay(500);

  String newText = data->toString(); // store the incoming data in a string

  theText = newText;

  // After parsing, call the function to display the new text
  displayData();

}

// this function is called whenever a color update
// is received from Adafruit IO. it was attached to
// the signcolor feed in the setup() function above.
void handleColor(AdafruitIO_Data *data) {
  char* newColor = data->toChar(); // store the incoming data as char
  Serial.print("Changing color to ");
  Serial.println(newColor);
  int r, g, b;
  // parse the hex into RGB
  sscanf(newColor, "#%02x%02x%02x", &r, &g, &b);

  matrix.setTextColor(matrix.Color(r, g, b));
  // Briefly flash the color on the matrix as confirmation
  lightPixels(matrix.Color(r, g, b));
  delay(1000);
  matrix.fillScreen(0);
  matrix.show();
}
// Function that shows the most recent data
void displayData() {
  Serial.println("Displaying data");
  Serial.println(theText);

  int bufferSize = 0 - (theText.length() * bufferOffset);

  while (--x > bufferSize) {
    matrix.fillScreen(0);
    matrix.setCursor(x, 0);
    matrix.print(theText.c_str());
    matrix.show();
    delay(100);
  }
  x = matrix.width();

}


// Function to set all the NeoPixels to the specified color.
void lightPixels(uint32_t color) {
  matrix.fillScreen(color);
  matrix.show();
}
