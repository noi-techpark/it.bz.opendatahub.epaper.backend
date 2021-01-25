/*
  e-ink-display

  A simple web server that can show an image to the E-Ink Display or clear the screen.


  created 18 Januray 2020
  by Simon Dalvai
*/
#include <SPI.h>
#include <WiFiUdp.h> // UDP for auto-connection with proxy
#include <WiFiNINA.h>
#include "EPD.h"
#include "GUI_Paint.h"

#include "EPD_SDCard.h"

#include "config.h" // Wifi password and SSID

// please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;       // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
char display_name[] = DISPLAY_NAME;

int status = WL_IDLE_STATUS;

IPAddress broadCast;

WiFiUDP udp;

char ipBuff[18];
char state[300];

// WebSocket config
char serverAddress[] = "192.168.1.12";  // TODO make dynamic with UDP
int port = 80;

boolean connectedToProxy = false; // to make UDP broadcast in case not connected

File imageFile;

// State, TODO try struct => simpler json formatting
boolean sleeping = false;
boolean hasImage = false;
int battery;

// flags to see what is going on, might not be needed with WS
boolean content = false;
boolean stateRequest = false;
boolean clearRequest = false;
boolean imageRequest = false;

  //write macc address to mac buff, TODO make locally or with malloc to save ram
//  char macBuff[18];
//  byte mac[6];
//  WiFi.macAddress(mac);
//  sprintf (macBuff, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);



void setup() {

  // init display and display-module
  DEV_Module_Init();
  EPD_7IN5BC_Init();
  EPD_7IN5BC_Clear();
  DEV_Delay_ms(500);
  Serial.println("Display Init done");

  // init SD card
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


    // Connect to WPA/WPA2 network. Change this line if using open or WEP network (check WiFiNina docs)
    if (sizeof(pass) > 0)
      status = WiFi.begin(ssid, pass);
    else
      status = WiFi.begin(ssid); // No passphrase

    // wait 10 seconds for connection:
    delay(10000);
  }
  // you're connected now, so print out the status:
  printWifiStatus();


  //    sleep();

  udp.begin(5005);
}


void loop() {

  // check wifi connection and try to reconnect if lost
  if (WiFi.status() != WL_CONNECTED) {
    reconnectToWifi();
  } else {


    // UDP broadcast
    if (!connectedToProxy) { // TODO check also if WS client exists

      udpBrodacast();
      //      udpListen();

      delay(5000);
    }

    // TODO do ws stuff


  }

  delay(100);
}

/*
   To broadcast the IP adress to the proxy with UDP
*/
void udpBrodacast() {
  // begin UDP packet and failcheck
  if (udp.beginPacket(broadCast, 5006) == 0)
  {
    Serial.println(F("BeginPacket fail"));
  }

  udp.write(ipBuff); // actual UDP broadcast

  // end UDP packet and failcheck
  if (udp.endPacket() == 0)
  {
    Serial.println(F("endPacket fail"));
  }
}

/*
   To listen to the reposne of the proxy,
   set the connectedToProxy flag on response
   and extracting the remoteIp and port
*/
void udpListen() {

}


void reconnectToWifi() {
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
  //start udp server
  udp.begin(5005);
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

  // TODO try to convert IP t ochar array directly, Strings are bad
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


// TODO add Wifi.loPowerMode() and WIFI.noLowPowermode() on sleep and on wake up!!!! to save energy
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


void MySDCard_CreateImage(void) {
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

void MySDCard_WriteImagePart(const char part) {
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

void MySDCard_CloseImage(void) {
  // close the file:
  imageFile.flush();
  imageFile.close();
  Serial.println("done.");
}

String MySDCard_ReadImageLine(int y) {
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

char MySDCard_ReadPixel() {
  if (imageFile.available())
  {
    return imageFile.read();
  }
  return "";
}
