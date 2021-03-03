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

**Table of Contents**

- [e-ink-displays-backend](#e-ink-displays-backend)
  - [Requirements](#requirements)
  - [Setup](#setup)
    - [Upload](#upload)
    - [SD Card](#sd-card)
    - [Auto connect to API with proxy](#auto-connect-to-api-with-proxy)
    - [Connect with no proxy](#connect-with-no-proxy)
  - [Troubleshoot](#troubleshoot)
    - [Permission denied, cannot open /dev/ttyACM0](#permission-denied-cannot-open-devttyacm0)
  - [Flight Rules](#flight-rules)
    - [arduino-cli](#arduino-cli)
    - [Uploading compiled code](#uploading-compiled-code)
    - [Debugging serial communication](#debugging-serial-communication)
    - [Missing SD.h](#missing-sdh)
    - [Warning: Device or resource busy](#warning-device-or-resource-busy)

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
Copy `config.h.dist` to `config.h`, and fill values in:

If you want to connect automatically to an existing display, put the exact
display Name you can see in the Webapp. Important: It must already exist in the
Webapp! 

```c
#define SECRET_SSID "YOUR_SSID"
#define SECRET_PASS "YOUR_PASSWORD" // empty, if no password
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

## Troubleshoot
If the proxy doesn't show any connection after some minutes, check if firewall
of machine where proxy is running has ingoing traffic allowed and that proxy and
the displays are in the same WIFI network. The incoming traffic needs to be
allowed because of UDP broadcasting.

### Permission denied, cannot open /dev/ttyACM0

1) sudo usermod -a -G dialout $USER
2) Logout/login again + re-plug the Arduino

Better way: `/opt/arduino-1.8.9/arduino-linux-setup.sh <your-user>`
`sudo /opt/arduino-1.8.9/install.sh` for IDE desktop files

## Flight Rules

### arduino-cli

Install it:
```
wget https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Linux_64bit.tar.gz 
tar xvzf arduino-cli_latest_Linux_64bit.tar.gz
sudo mv arduino-cli /usr/bin/
arduino-cli completion bash > arduino-cli-completion.sh
sudo mv arduino-cli-completion.sh /usr/share/bash-completion/completions/arduino-cli
. /usr/share/bash-completion/completions/arduino-cli
arduino-cli config init
arduino-cli core update-index
arduino-cli board listall
```

Now install the Arduino Board you use:
```
arduino-cli core search "Arduino Uno WiFi Rev2"
arduino-cli core install "arduino:megaavr"
```

Copy libraries/EPD to ~/Arduino/libraries, since it is not the one, installable from arduino.cc

List the connected board
```
arduino-cli board list
```


Compiling the sketch using the command line
```
arduino-cli compile --fqbn arduino:megaavr:uno2018 e-ink-display
```

...add `--upload` if you want to upload it immediately; `-p /dev/ttyACM1` if it is not the default port:
```
arduino-cli compile --upload -p /dev/ttyACM0 --fqbn arduino:megaavr:uno2018 e-ink-display
```


...or just verifying with:
```
arduino-cli compile --verify --fqbn arduino:megaavr:uno2018 e-ink-display
```

### Uploading compiled code

With this we can upload to more than one Arduino:
```
arduino-cli upload -p /dev/ttyACM1 --fqbn arduino:megaavr:uno2018 e-ink-display
```

### Debugging serial communication

`sudo apt install gtkterm`
or 
`sudo apt install minicom`

### Missing SD.h

arduino-cli lib install SD

### Warning: Device or resource busy 

`Warning: avrdude: usbdev_open(): WARNING: failed to set configuration 1: Device or resource busy`

apt purge modemmanager
