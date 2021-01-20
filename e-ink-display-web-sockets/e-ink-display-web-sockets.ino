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

#include "EPD_SDCard.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;       // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
char display_name[] = DISPLAY_NAME;

int status = WL_IDLE_STATUS;

IPAddress broadCast;
IPAddress ip;
char macBuff[18];
char ipBuff[18];
char state[300];

boolean connectedToProxy = false;

WiFiUDP udp;
WiFiServer server(80);
WiFiClient wifiClient;

File imageFile;


boolean sleeping = false;
boolean hasImage = false;
int battery;

boolean content = false;
boolean stateRequest = false;
boolean clearRequest = false;
boolean imageRequest = false;


void setup() {
  DEV_Module_Init();
  EPD_7IN5BC_Init();
  EPD_7IN5BC_Clear();
  DEV_Delay_ms(500);
  Serial.println("Display Init done");

  SDCard_Init();


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
  // check wifi connection and try to reconnect if lost
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
    // check if client exits from last loop iteration
    if (wifiClient) {
      //      Serial.println("new client");
      // an http request ends with a blank line
      boolean currentLineIsBlank = true;
      connectedToProxy = true;

      //to read content
      char c;

      int counter = 0;
      while (wifiClient.available()) {
        c = wifiClient.read();

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
          if (c == '2') // clear screen
          {
            Serial.println("Clear Request");
            clearRequest = true;
            clearDisplay();
          }
          else if (c == '3') // state
          {
            stateRequest = true;
            Serial.println("State Request");
          }
          else // image data
          {
            if (!imageRequest)
            {
              imageRequest = true;
              Serial.println("Image Request");
              MySDCard_CreateImage();
            }
            MySDCard_WriteImagePart(c);
            counter++;
            if (counter == 32) {
              //              Serial.println("Data chunck written");
              break;
            }
          }
        }
      }

      if (!wifiClient.available()) { // check if client has still bytes to send
        sprintf(state, "{\"displayName\": \"%s\" ,\"sleeping\": %s ,\"hasImage\": %s ,\"battery\": %i, \"width\": %i,\"height\": %i,\"mac\": \"%s\"}", display_name, sleeping ? "true" : "false", hasImage ? "true" : "false", battery, (int)EPD_7IN5BC_WIDTH, (int)EPD_7IN5BC_HEIGHT, macBuff);

        Serial.println(state);
        Serial.println(strlen(state));

        wifiClient.println("HTTP/1.1 200 OK");
        wifiClient.println("Content-Type: application/json");
        wifiClient.println("Connection: close");
        wifiClient.print("Content-Length: ");
        wifiClient.println(strlen(state));
        wifiClient.println();
        wifiClient.println(state);
        wifiClient.println();
        // give the web browser time to receive the data
        delay(2000);

        // close the connection:
        wifiClient.stop();
        Serial.println("client disconnected");


        if (imageRequest) {
          MySDCard_CloseImage();
          Serial.println("Writing image");
          wakeUp();
          clearDisplay();
          MySDCard_OpenImage();

          for (int y = 0; y < EPD_7IN5BC_HEIGHT; y++) {
            for (int x = 0; x < EPD_7IN5BC_WIDTH; x++) {
              if (MySDCard_ReadPixel() == '1') {
                Paint_DrawPoint(x, y, WHITE, DOT_PIXEL_DFT, DOT_STYLE_DFT);
              }
            }
          }

          EPD_7IN5BC_Display();
          DEV_Delay_ms(2000);
          MySDCard_CloseImage();
          Serial.println("Writing image done");
        }
        hasImage = true;
        content = false;
        stateRequest = false;
        clearRequest = false;
        imageRequest = false;
        wifiClient = server.available();
        delay(2000);
      }
    } else {
      //      Serial.println("Listen for clients");
      wifiClient = server.available(); // listen for clients
      delay(2000);
    }

    if (!connectedToProxy && !wifiClient) {
      // send a reply, to the IP address and port that sent us the packet we received
      if (udp.beginPacket(broadCast, 5006) == 0)
      {
        Serial.println(F("BeginPacket fail"));
      }
      udp.write(ipBuff);
      if (udp.endPacket() == 0)
      {
        Serial.println(F("endPacket fail"));
      }
      delay(5000);
    }

  }
  delay(10);
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


  String ipString = ipToString(ip);
  ipString.toCharArray(ipBuff, ipString.length() + 1);


  Paint_NewImage(IMAGE_BW, EPD_7IN5_WIDTH, EPD_7IN5_HEIGHT, IMAGE_ROTATE_0, IMAGE_COLOR_INVERTED);
  Paint_Clear(BLACK);

  Paint_DrawString_EN(200, 160, "Connecting...", &Font24, BLACK, WHITE);
  Paint_DrawString_EN(200, 260, ipBuff, &Font24, BLACK, WHITE);

  EPD_7IN5BC_Display();
}

String ipToString(IPAddress ip) {
  String s = "";
  for (int i = 0; i < 4; i++) {
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return s;
}


void MySDCard_CreateImage(void)
{
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    if (SD.exists("IMAGE.TXT"))
    {
        DEBUG("DELETE FILE!\n");
        SD.remove("IMAGE.TXT");
    }
    while (SD.exists("IMAGE.TXT"))
        ;
    imageFile = SD.open("IMAGE.TXT", FILE_WRITE);

    DEBUG("FILE OPEN!\n");
}

void MySDCard_OpenImage(void)
{
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    imageFile = SD.open("IMAGE.TXT");

    DEBUG("FILE OPEN!\n");
}

void MySDCard_WriteImagePart(const char part)
{
    // if the file opened okay, write to it:
    if (imageFile)
    {
        // Serial.print("Writing to test.txt...");
        imageFile.print(part);
    }
    else
    {
        // if the file didn't open, print an error:
        Serial.println("error opening test.txt");
    }

    //   DEBUG("WRITING PART!\n");
}

void MySDCard_CloseImage(void)
{

    // close the file:
    imageFile.flush();
    imageFile.close();
    Serial.println("done.");
}

String MySDCard_ReadImageLine(int y)
{
    while (y > -1 && imageFile.available())
    {
        if (y == 0)
        {
            String line = imageFile.readStringUntil('\n');
            // line.replace(",", "");
            // DEBUG(line);
            return line;
        }
        y--;
    }
    return "";
}

char MySDCard_ReadPixel()
{
    if(imageFile.available())
    {
        return imageFile.read();
    }
    return "";
}
