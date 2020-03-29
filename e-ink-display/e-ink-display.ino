/*
  e-ink-display

  A simple web server that can show an image to the E-Ink Display or clear the screen.


  created 18 Januray 2020
  by Simon Dalvai
*/
#include <SPI.h>
#include <WiFiNINA.h>
#include "EPD_7in5.h"
#include "GUI_Paint.h"
#include "config.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;


WiFiServer server(80);

char state[] = "0;0;100"; //0 sleeping, 1 has image, 2 battery state


void setup() {

  DEV_Module_Init();
  EPD_7IN5_Init();
  EPD_7IN5_Clear();
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
  server.begin();
  // you're connected now, so print out the status:
  printWifiStatus();
  sleep();
}


void loop() {
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
            Paint_DrawPoint(x, y, BLACK, DOT_PIXEL_DFT, DOT_STYLE_DFT);
          }
          else if (c == '2') { // 2 means clear display
            wakeUp();
            clearDisplay();
            sleep();
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println();
            client.println(state);
            break;
          }
          else if (c == '3') { //3 means API is asking for current state
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println();
            client.println(state);
            break;
          }

          //incrementing coordinates
          x++;
          if (x == EPD_7IN5_WIDTH) {
            x = 0;
            y++;
          }

          //          delay to kepp sure that image is displayed correctly
          if (x % 10 == 0)
            DEV_Delay_ms(1);
        }

      } else {
        //gets only triggered when writing new image

        state[2] = '1';


        //Closing conncetion with client
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println();
        client.println(state);

        //All bits recieved, so writing image to display and
        writeImageToDisplay();

        sleep();
        break;
      }
    }
    // give the web browser time to receive the data
    delay(100);

    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
  delay(1000);
}


void writeImageToDisplay() {
  Serial.println("Write image to screen");
  EPD_7IN5_Display();
  DEV_Delay_ms(2000);
  Serial.println("Write image to screen done");
}

void clearDisplay() {
  Serial.println("Clear display");
  Paint_NewImage(IMAGE_BW, EPD_7IN5_WIDTH, EPD_7IN5_HEIGHT, IMAGE_ROTATE_0, IMAGE_COLOR_INVERTED);
  Paint_Clear(WHITE);
  EPD_7IN5_Display();
  state[2] = '0';
  Serial.println("Clear display done");
}

void sleep() {
  if (state[0] == '0') {
    EPD_7IN5_Sleep();
    DEV_Delay_ms(2000);
    state[0] = '1';
    Serial.println("Display sleeping");
  }
}



void wakeUp() {
  if ( state[0] == '1') {
    DEV_Module_Init();
    EPD_7IN5_Init();
    DEV_Delay_ms(500);
    state[0] = '0';
    Serial.println("Display woke up");
  }
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  char ipBuff[18];
  sprintf(ipBuff, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

  Paint_NewImage(IMAGE_BW, EPD_7IN5_WIDTH, EPD_7IN5_HEIGHT, IMAGE_ROTATE_0, IMAGE_COLOR_INVERTED);
  Paint_Clear(WHITE);

  Paint_DrawString_EN(200, 130, ssid, &Font24, WHITE, BLACK);
  Paint_DrawString_EN(200, 160, ipBuff, &Font24, WHITE, BLACK);

  EPD_7IN5_Display();
}
