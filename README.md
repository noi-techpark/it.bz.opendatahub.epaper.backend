# e-ink-displays-backend
The Arduino Development

The Ardunio code for the E-ink project.

The arduino used UDP and HTTP to communicate with proxy/API and a E-Paper shield
to send image to the display.

NOTE: The Arduino is not so powerful, so sending and displaying images might take some minutes.

Code for Arduino UNO WIFI rev.2 Board to show images from the
[e-ink-displays-api](https://github.com/noi-techpark/e-ink-displays-api) that
can be controlled with the
[e-ink-displays-webapp](https://github.com/noi-techpark/e-ink-displays-webapp).

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**

- [e-ink-displays-backend](#e-ink-displays-backend)
  - [Requirements](#requirements)
  - [Setup](#setup)
    - [WPA](#wpa)
    - [OPEN WIFI - NO PASSWORD](#open-wifi---no-password)
    - [Predefined display name](#predefined-display-name)
    - [Upload](#upload)
    - [SD Card](#sd-card)
    - [Auto connect to API with proxy](#auto-connect-to-api-with-proxy)
    - [Connect with no proxy](#connect-with-no-proxy)
    - [Troubleshoot](#troubleshoot)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

## Requirements

- An Ardunio Uno Wifi Rev 2 Board was used during development, but any Arduino
  board with Wifi connection should work, you might need to use other WIFI
  Libraries but E-Paper logic remains the same

- E-Ink display (Waveshare 7.5  displays with 680x384 in all versions, used and
  tested with this project) But you can use any display with any resolution,
  because resooltion can be set in API

- E-Ink Shield to connect Arduino with display

- SD-Card to save the images before writing to display will be used more in
  future, to even save mutliple images and be able to simply trigger the display
  of a new image whitout the need of sending it. So images coulkd be loaded
  during night and during day only simple triggers change the image 


## Setup
Put SSID of Network you want to conncect to arduino_secrets.h
```c
#define SECRET_SSID "YOUR_SSID"
``` 

Depending on the security of the Wifi you want to connect change following lines
of code

### WPA
Add the password to config.h
```
#define SECRET_PASS "YOUR_PASSWORD"
```
### OPEN WIFI - NO PASSWORD
Leave SECRET_PASS empty in config.h
```
#define SECRET_PASS ""
```

### Predefined display name
If you want to connect automatically to an existing display, put the exact
display Name you can see in the Webapp to config.h file. example:
```
#define DISPLAY_NAME "Seminar 2 Display"
```

### Upload
Now you can upload the code to your Arduino Board. Set Serial Monitor BAUD Rate
to 115200 and take note of IP-Address the board got. So you can create a
connection for the Board with the Spring-Backend in the Webapp by putting the
IP-Address in the Network Address field when you create the connection.

### SD Card
Plugin an SDCard of any size (images are small so alos <1 GB works>) before you
plugin the arduino to the power source.

### Auto connect to API with proxy
If you are using a proxy in the API setup, the display connects automatically
with the API. That works by the display broadcasting his MAC Address over UDP
and the proxy listening to it. So when the proxy recieves a new UDP message with
a MAC address, it sends it to the API. Then the API reconnects to the display or
creates a new connection if its the first time that a dosplay connects. Note:
Plase make sure firewall doesn't block UDP incoming packages in yor proxies
machine

### Connect with no proxy
If you don't use a proxy, the auto connection doesn't work. In that case you
need to plug in the display and wait until you can read its IP address on the
display. Then you can create or edit with the webapp a connection and set the
corresponding IP address. The you can send the image with the web app by
clicking on send, and the display should recive the image within minutes.

### Troubleshoot
If the proxy doesn't show any connection after some minutes, check if firewall
of machine where proxy is running has ingoing traffic allowed and that proxy and
the displays are in the same WIFI network. The incoming traffic needs to be
allowed because of UDP broadcasting.