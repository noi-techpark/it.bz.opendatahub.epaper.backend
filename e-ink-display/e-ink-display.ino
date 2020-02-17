/*
  e-ink-display

  A simple web server that can show an image to the E-Ink Display or clear the screen.


  created 18 Jnauray 2020
  by Simon Dalvai
*/

#include <SPI.h>
#include <WiFiNINA.h>
#include "EPD_7in5.h"
#include "GUI_Paint.h"
#include "arduino_secrets.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;


//states of display
boolean isSleeping = false;
int batteryState = 100;
String lastStateTimeStamp; //timestamp to save time of last state check by API
String lastUpdateTimeStamp; //timestamp of last image update



WiFiServer server(80);

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Initialize Display");
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
    status = WiFi.begin(ssid, pass); // remove pass if WIFI has no password

    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();
  // you're connected now, so print out the status:
  printWifiStatus();

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

    //coordinates to write every single pixel to screen
    int  x = 0;
    int  y = 0;

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        if (!content)
          Serial.write(c);


        // read content
        if (content) {
          if (c == '1') {
            printPixelToScreen(x, y);
          }
          else if (c == '2') { // 2 means clear display
            clearDisplay();
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");  // the connection will be closed after completion of the response
            break;
          }
          else if (c == '3') { //3 means API is asking for current state
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Hello API");
            client.println("Connection: close");  // the connection will be closed after completion of the response
            break;
          }

          //incrementing coordinates
          x++;
          if (x == 640) {
            x = 0;
            y++;
          }
          if(x % 4 == 0)
            DEV_Delay_ms(1);
        }

        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          content = true;
          clearDisplay();
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      } else {
        //Closing conncetion with client
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");  // the connection will be closed after completion of the response

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


}

void printPixelToScreen(int x, int y) {
  Paint_DrawPoint(x, y, BLACK, DOT_PIXEL_DFT, DOT_STYLE_DFT);
//  DEV_Delay_ms(3);
}


void writeImageToDisplay() {
  Serial.println("Write image to screen");
  EPD_7IN5_Display();
  DEV_Delay_ms(2000);
  Serial.println("Write image to screen done");
}

void clearDisplay() {
  if (isSleeping)
    wakeUp();

  Serial.println("Clear display");
  Paint_NewImage(IMAGE_BW, EPD_7IN5_WIDTH, EPD_7IN5_HEIGHT, IMAGE_ROTATE_0, IMAGE_COLOR_INVERTED);
  Paint_Clear(WHITE);
  EPD_7IN5_Display();
  Serial.println("Clear display done");
}

void sleep() {
  EPD_7IN5_Sleep();
  DEV_Delay_ms(2000);
  isSleeping = true;
  Serial.println("Display sleeping");
}



void wakeUp() {
  DEV_Module_Init();
  EPD_7IN5_Init();
  DEV_Delay_ms(500);
  isSleeping = false;
  Serial.println("Display waked up");
}



void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
