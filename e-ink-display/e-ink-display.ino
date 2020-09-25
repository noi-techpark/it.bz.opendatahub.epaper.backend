/*
  e-ink-display

  A simple web server that can show an image to the E-Ink Display or clear the screen.


  created 18 Januray 2020
  by Simon Dalvai
*/
#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include "EPD.h"
#include "GUI_Paint.h"
#include "config.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
char display_name[] = DISPLAY_NAME;


int status = WL_IDLE_STATUS;

IPAddress broadCast;
IPAddress ip;
char macBuff[18];

boolean connectedToProxy = false;

WiFiUDP udp;
WiFiServer server(80);

boolean sleeping;
boolean hasImage;
int battery;


void setup() {
  DEV_Module_Init();
  EPD_7IN5BC_Init();
  EPD_7IN5BC_Clear();
  DEV_Delay_ms(500);
  Serial.println("Display Init done");


  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    if (sizeof(pass) > 0)
      status = WiFi.begin(ssid, pass);
    else
      status = WiFi.begin(ssid);

    // wait 10 seconds for connection:
    delay(10000);
  }
  // you're connected now, so print out the status:
  printWifiStatus();

  
  byte mac[6];
  WiFi.macAddress(mac);
  sprintf (macBuff, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  sleep();

  server.begin();
  udp.begin(5005);
}


void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    while (WiFi.status() != WL_CONNECTED) {
      Serial.println(WiFi.status());
      Serial.println("Wifi connection lost");
      if (sizeof(pass) > 0)
        status = WiFi.begin(ssid, pass);
      else
        status = WiFi.begin(ssid);
      Serial.println("Tried restart");
      delay(10000);
    }
    server.begin();
    udp.begin(5005);
  } else {
    // listen for incoming clients
    WiFiClient client = server.available();

    if (client) {
      Serial.println("new client");
      // an http request ends with a blank line
      boolean currentLineIsBlank = true;

      //to read content
      boolean content = false;

      //to save if display has been cleared before new image gets written
      boolean isDisplayClearedAndWakedUp = false;


      //coordinates to write every single pixel to screen
      int x = 0;
      int y = 0;

      while (client.connected()) {
        if (client.available()) {
          connectedToProxy = true;
          char c = client.read();

          if (!content) {
            Serial.write(c);
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
          }
          else {

            
            if (c == '1') {
              if (!isDisplayClearedAndWakedUp) {
                wakeUp();
                clearDisplay();
                isDisplayClearedAndWakedUp = true;
              }
              Paint_DrawPoint(x, y, WHITE, DOT_PIXEL_DFT, DOT_STYLE_DFT);
            }
            else if (c == '2') { // 2 means clear display
              wakeUp();
              clearDisplay();
              sleep();


              char state[200];
              sprintf(state, "{\"display_name\": \"%s\" ,\"sleeping\": %s ,\"hasImage\": %s ,\"battery\": %i, \"width\": %i,\"height\": %i,\"mac\": \"%s\"}",display_name ,sleeping ? "true" : "false", hasImage ? "true" : "false", battery, (int)EPD_7IN5BC_WIDTH, (int)EPD_7IN5BC_HEIGHT, macBuff);

              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: application/json");
              client.println();
              client.println(state);
              break;
            }
            else if (c == '3') { //3 means API is asking for current state
              char state[200];
              sprintf(state, "{\"display_name\": \"%s\" ,\"sleeping\": %s ,\"hasImage\": %s ,\"battery\": %i, \"width\": %i,\"height\": %i,\"mac\": \"%s\"}",display_name, sleeping ? "true" : "false", hasImage ? "true" : "false", battery, (int)EPD_7IN5BC_WIDTH, (int)EPD_7IN5BC_HEIGHT, macBuff);

              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: application/json");
              client.println();
              client.println(state);
              break;
            }

            //incrementing coordinates
            x++;
            if (x == EPD_7IN5BC_WIDTH) {
              x = 0;
              y++;
            }

            //          delay to kepp sure that image is displayed correctly
            if (x % 12 == 0)
              DEV_Delay_ms(2);

            // when third image is written, write buffer to display
            if (y > 0 && y % (EPD_7IN5BC_HEIGHT/3) == 0 && x == 0)
              writeImageToDisplay();
          }

        } else {
          //gets only triggered when writing new image

          hasImage = true;
//          writeImageToDisplay();
          sleep();

          char state[200];
          sprintf(state, "{\"display_name\": \"%s\" ,\"sleeping\": %s ,\"hasImage\": %s ,\"battery\": %i, \"width\": %i,\"height\": %i,\"mac\": \"%s\"}", display_name, sleeping ? "true" : "false", hasImage ? "true" : "false", battery, (int)EPD_7IN5BC_WIDTH, (int)EPD_7IN5BC_HEIGHT, macBuff);
          
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println();
          client.println(state);
          break;
        }
      }
      // give the web browser time to receive the data
      delay(500);

      // close the connection:
      client.stop();
      Serial.println("client disconnected");
    }

    if (!connectedToProxy) {
      // send a reply, to the IP address and port that sent us the packet we received
      if (udp.beginPacket(broadCast, 5006) == 0)
      {
        Serial.println(F("BeginPacket fail"));
      }
      udp.write(macBuff);
      if (udp.endPacket() == 0)
      {
        Serial.println(F("endPacket fail"));
      }
      delay(10000);
    }

  }
  delay(100);
}


void writeImageToDisplay() {
  Serial.println("Write image to screen");
  EPD_7IN5BC_Display();
  DEV_Delay_ms(2000);
  Serial.println("Write image to screen done");
}

void clearDisplay() {
  Serial.println("Clear display");
  Paint_NewImage(IMAGE_BW, EPD_7IN5_WIDTH, EPD_7IN5_HEIGHT, IMAGE_ROTATE_0, IMAGE_COLOR_INVERTED);
  Paint_Clear(BLACK);
  EPD_7IN5_Display();
  hasImage = false;
  Serial.println("Clear display done");
}

void sleep() {
  if (!sleeping) {
    EPD_7IN5BC_Sleep();
    DEV_Delay_ms(2000);
    sleeping = true;
    Serial.println("Display sleeping");
  }
}



void wakeUp() {
  if (sleeping) {
    DEV_Module_Init();
    EPD_7IN5BC_Init();
    DEV_Delay_ms(500);
    sleeping = false;;
    Serial.println("Display woke up");
  }
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  IPAddress subnet = WiFi.subnetMask();
  broadCast = ip | ~subnet;
  Serial.print("IP Address: ");
  Serial.println(ip);


  Paint_NewImage(IMAGE_BW, EPD_7IN5_WIDTH, EPD_7IN5_HEIGHT, IMAGE_ROTATE_0, IMAGE_COLOR_INVERTED);
  Paint_Clear(BLACK);

  Paint_DrawString_EN(200, 160, "Connecting...", &Font24, BLACK, WHITE);

  EPD_7IN5BC_Display();
}
