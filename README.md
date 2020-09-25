# e-ink-displays-backend
The Arduino Development

Code for Arduino UNO WIFI rev.2 Board to show images from the [e-ink-displays-api](https://github.com/noi-techpark/e-ink-displays-api) that can be controlled with the [e-ink-displays-webapp](https://github.com/noi-techpark/e-ink-displays-webapp).

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**

- [Setup](#setup)
    - [WPA](#wpa)
    - [OPEN WIFI - NO PASSWORD](#open-wifi---no-password)
    - [Predefined display name](#predefined-display-name)
    - [Upload](#upload)
    - [Troubleshoot](#troubleshoot)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# Setup
Put SSID of Network you want to conncect to arduino_secrets.h
```
#define SECRET_SSID "YOUR_SSID"
``` 

Depending on the Security of the Wifi you want to connect change following lines of code

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
If you want to connect automatically to an existing display, put the exact display Name you can see in the Webapp to config.h file. example:
```
#define DISPLAY_NAME "Seminar 2 Display"
```

### Upload
Now you can upload the code to your Arduino Board. Set Serial Monitor BAUD Rate to 115200 and take note of IP-Address the board got. So you can create a connection for the Board with the Spring-Backend in the Webapp by putting the IP-Address in the Network Address field when you create the connection.

### Troubleshoot
If the proxy doesn't show any connection after some minutes, check if firewall of machine where proxy is running has ingoing traffic allowed and that proxy and the displays are in the same WIFI network. The incoming traffic needs to be allowed because of UDP broadcasting.