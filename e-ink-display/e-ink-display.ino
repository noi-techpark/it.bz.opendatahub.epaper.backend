/*
  e-ink-display

  A simple web server that can show an image to the E-Ink Display or clear the screen.


  created 18 Januray 2020
  by Simon Dalvai
*/
#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include <EPD.h>
#include <GUI_Paint.h>
#include <EPD_SDCard.h>

#include "config.h"

#undef USE_DEBUG

#define EINK_DEBUG 1
#if EINK_DEBUG
  #define EINK_INFO 1
	#define DBG(a) Serial.print(a)
  #define DBG2(a, b) Serial.print(a); Serial.print(b)
  #define DBG3(a, b, c) Serial.print(a); Serial.print(b); Serial.print(c)
#else
	#define DBG(a)
  #define DBG2(a, b)
  #define DBG3(a, b, c)
#endif

#define EINK_INFO 1
#if EINK_INFO
	#define INFO(a) Serial.print(a)
  #define INFO2(a, b) Serial.print(a); Serial.print(b)
  #define INFO3(a, b, c) Serial.print(a); Serial.print(b); Serial.print(c)
#else
	#define INFO(a)
  #define INFO2(a, b)
  #define INFO3(a, b, c)
#endif


#define WIFI_CHECK_REPEATS 3
#define NL "\r\n"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;          // your network SSID (name)
char pass[] = SECRET_PASS;          // your network password (use for WPA, or use as key for WEP)
char displayName[] = DISPLAY_NAME;  // Display name, which must be present already

IPAddress broadCast;
IPAddress ip;


char ipBuff[18];
int imageIdx = 0;
char imageName[13] = "12345678.TXT"; // must be a 8.3 notation (uppercase)
char bufimg[EPD_7IN5BC_WIDTH];

boolean connectedToProxy = false;

WiFiUDP udp;
WiFiServer server(80);
WiFiClient wifiClient;

File imageFile;

// state flags
boolean sleeping = false;
boolean hasImage = false;

// current action flags
boolean content = false;
boolean imageNameRequest = false;
boolean imageDataRequest = false;
boolean imageDataFlush = false;
boolean imageExists = false;
boolean imagePrint = false;

long chunkCounter = 0;
int loopCounter = 0;

void setup() {
  // Start dev mode; enable serial monitor...
  DEV_Module_Init();
  #undef USE_DEBUG

  // check for the WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    INFO("SETUP: Communication with WiFi module failed! We stop here!" NL);
    while (true);
  }

  INFO(NL "SETUP: Init display..." NL);
  EPD_7IN5BC_Init();
  EPD_7IN5BC_Clear();
  DEV_Delay_ms(500);
  INFO3(NL "DONE! Name = ", displayName, NL);

  INFO(NL "SETUP: Init SD card...");
  SDCard_Init();

  connectToWifi();
  printWifiStatus();
  sleepDisplay();
}


void loop() {

  loopCounter++;

  // check wifi connection and try to reconnect if lost
  // reset proxy connection flag, to be able to redo the udp broadcast
  if (! wifiCheckStatus(WIFI_CHECK_REPEATS)) {
    connectToWifi();
    connectedToProxy = false;
    myDelay(1000);
  } else {

    if (!connectedToProxy && !wifiClient) {
      // send a reply, to the IP address and port that sent us the packet we received
      udpBroadcast();
      myDelay(5000);
    }

    // check if client exits from last loop iteration
    if (!wifiClient) {
      if (loopCounter >= 10) {
        DBG(".");
        loopCounter = 0;
      }
      wifiClient = server.available(); // listen for clients
      myDelay(1000);
    }

    // an http request ends with a blank line
    boolean currentLineIsBlank = true;

    // proxy replies with state request as UDP response
    // so if a wifiLCinet connects, it means that the display is connected to the proxy
    // ao we can stop udp server and set connected flag
    if (!connectedToProxy) {
      connectedToProxy = true;
      udp.stop();
    }

    // to read content (every single bit)
    char c;

    while (wifiClient.available()) {
      c = wifiClient.read();

      if (!content) {
        DBG("!");
        if (c == '\n' && currentLineIsBlank) {
          content = true;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      } else {
        // image data
        if (imageDataRequest) {
          // INFO3("IDR...imageDataRequest: ", c, NL);
          if (c == '0' || c == '1') {
            bufimg[imageIdx] = c;
            imageIdx++;
            if (imageIdx >= EPD_7IN5BC_WIDTH) {
              imageDataFlush = true;
            }
          } else {
            imageDataRequest = false;
            imagePrint = true;
            imageDataFlush = true;
          }

          if (imageDataFlush) {
              // INFO(bufimg);
              INFO2("Writing line #", chunkCounter);
              imageFile.close();
              imageFile = SD.open(imageName, FILE_WRITE);
              int amount = imageFile.write(bufimg, EPD_7IN5BC_WIDTH);
              myDelay(5);
              imageFile.close();
              myDelay(5);
              INFO3(" --> written bytes: ", amount, NL);
              imageIdx = 0;
              imageDataFlush = false;
              chunkCounter++;
          }

          if (chunkCounter >= EPD_7IN5BC_HEIGHT) {
              imageDataRequest = false;
              imagePrint = true;
              imageDataFlush = true;
              break;
          }

        } else if (imageNameRequest) {
          if  (c == '-') {
            INFO2(NL "IMAGE: imageName = ", imageName);
            imageExists = SD.exists(imageName);
            if (imageExists) {
              imagePrint = true;
              INFO("...image already exists" NL);
              break;
            } else {
              imageDataRequest = true;
              imageIdx = 0;
              chunkCounter = 0;
              INFO("...image does not exist. Uploading!" NL);
            }
            imageNameRequest = false;
          } else {
            imageName[imageIdx] = c;
            imageIdx++;
          }
        } else if  (c == '1') { // load image request
          imageNameRequest = true;
          imageIdx = 0;
        } else if (c == '2') {  // clear screen request
          clearDisplay();
          sendStateAndReset();
        } else if (c == '3') {  // state request
          sendStateAndReset();
        }

      }
    }

    // draw image to screen
    if (imagePrint) {
      DBG(c);
      sendStateAndReset();  // FIXME should be afterwards, but otherwise the proxy blocks too long
      drawImage(imageName);
      hasImage = true;
    }
  }

  myDelay(100);
}

void myDelay(long microSeconds)
{
  microSeconds *= 100; // transform to milliseconds
  const unsigned long start = micros();
  while (true)
  {
    unsigned long now = micros();
    if (now - start >= microSeconds)
    {
      return;
    }
  }
}

void sendStateAndReset() {
  INFO("Send STATE back! Wait for the wifi client to be free...");
  while (wifiClient.available()) {
    wifiClient.read();
  }
  INFO("OK! Sending...");
  char macBuff[18];
  byte mac[6];
  WiFi.macAddress(mac);
  sprintf (macBuff, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  char state[200];

  // use placeholders d = displayName, s = sleeping, i = hasImage, b = battery, w = width, h = height, m = mac
  sprintf(
    state,
    "{\"d\": \"%s\" ,\"s\": %s ,\"i\": %s ,\"b\": %i, \"w\": %i,\"h\": %i,\"m\": \"%s\"}",
    displayName,
    sleeping ? "true" : "false",
    hasImage ? "true" : "false",
    0,
    (int)EPD_7IN5BC_WIDTH,
    (int)EPD_7IN5BC_HEIGHT,
    macBuff
  );

  wifiClient.println("HTTP/1.1 200 OK");
  wifiClient.println("Content-Type: application/json");
  wifiClient.println("Connection: close");
  wifiClient.print("Content-Length: ");
  wifiClient.println(strlen(state));
  wifiClient.println();
  wifiClient.println(state);
  wifiClient.println();
  // give the proxy time to receive the data
  delay(2000);
  wifiClient.stop();

  // Reset flags and start over...
  content = false;
  imageNameRequest = false;
  imageDataRequest = false;
  imagePrint = false;

  INFO("Resetting and state transmission: DONE!" NL);
}

void udpBroadcast() {
  udp.beginPacket(broadCast, 5006);
  udp.write(ipBuff);
  udp.endPacket();
}

void drawImage(const char *img_id) {
  INFO(NL "drawImage: START..." NL);
  imageFile.close();
  imageFile = SD.open(img_id);

  wakeUpDisplay();
  Paint_NewImage(IMAGE_BW, EPD_7IN5_WIDTH, EPD_7IN5_HEIGHT, IMAGE_ROTATE_0, IMAGE_COLOR_POSITIVE);
  Paint_Clear(BLACK);

  INFO3("drawImage: Clearing done... printing progress of y axis now! Full height:", EPD_7IN5BC_HEIGHT, NL);
  for (int y = 0; y < EPD_7IN5BC_HEIGHT; y++) {
    // INFO(y);
    if (imageFile.available()) {
      int amount = imageFile.read(bufimg, EPD_7IN5BC_WIDTH);
      DBG3("We've got data with length = ", amount, NL);
    } else {
      int amount = imageFile.read(bufimg, EPD_7IN5BC_WIDTH);
      DBG3("Not enough data: ", amount, NL);
      break;
    }
    for (int x = 0; x < EPD_7IN5BC_WIDTH; x++) {
      if (bufimg[x] == '1') {
        Paint_DrawPoint(x, y, WHITE, DOT_PIXEL_DFT, DOT_STYLE_DFT);
      }
    }
  }
  EPD_7IN5BC_Display();
  DEV_Delay_ms(2000);
  imageFile.close();
  INFO("drawImage: DONE!" NL);
}

boolean wifiCheckStatus(int repeats) {
  for (int i = 0; i < repeats; i++) {
    if (WiFi.status() == WL_CONNECTED)
      return true;
  }
  return false;
}

void connectToWifi() {
  if (wifiCheckStatus(WIFI_CHECK_REPEATS)) {
    INFO("WIFI: Already connected..." NL);
    return;
  }
  while (! wifiCheckStatus(WIFI_CHECK_REPEATS)) {
    INFO2("WIFI: Connecting... Status = ", WiFi.status());
    if (sizeof(pass) > 0)
      WiFi.begin(ssid, pass);
    else
      WiFi.begin(ssid);
    delay(10000);
  }
  INFO(" ...Connected!" NL);
  INFO("Starting UDP and WIFI server");
  server.begin();
  udp.begin(5005);
  INFO(" ...DONE!" NL);
}

void clearDisplay() {
  INFO("Clear display: START" NL);
  wakeUpDisplay();
  EPD_7IN5BC_Clear();
  sleepDisplay();
  hasImage = false;
  INFO("Clear display: DONE!" NL);
}

void sleepDisplay() {
  if (!sleeping) {
    EPD_7IN5BC_Sleep();
    DEV_Delay_ms(2000);
    sleeping = true;
    INFO("Display sleeping");
  }
}

void wakeUpDisplay() {
  if (sleeping) {
    DEV_Module_Init();
    EPD_7IN5BC_Init();
    DEV_Delay_ms(500);
    sleeping = false;
    INFO("Display woke up");
  }
}

void printWifiStatus() {
  IPAddress ip = WiFi.localIP();
  IPAddress subnet = WiFi.subnetMask();
  broadCast = ip | ~subnet;

  INFO2("SSID: ", WiFi.SSID());
  INFO3("; IP Address: ", ip, NL);

  String ipString = "";
  for (int i = 0; i < 4; i++) {
    ipString += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  ipString.toCharArray(ipBuff, ipString.length() + 1);

  Paint_NewImage(IMAGE_BW, EPD_7IN5_WIDTH, EPD_7IN5_HEIGHT, IMAGE_ROTATE_0, IMAGE_COLOR_INVERTED);
  Paint_Clear(BLACK);
  Paint_DrawString_EN(200, 160, "Connected!", &Font24, BLACK, WHITE);
  Paint_DrawString_EN(200, 260, ipBuff, &Font24, BLACK, WHITE);
  EPD_7IN5BC_Display();
}
