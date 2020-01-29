# e-ink-displays-backend
The Arduino Development

Code for Arduino UNO WIFI rev.2 Board to show images from the [e-ink-displays-api](https://github.com/noi-techpark/e-ink-displays-api) that can be controlled with the [e-ink-displays-webapp](https://github.com/noi-techpark/e-ink-displays-webapp).

# Setup
Put SSID of Network you want to conncect to arduino_secrets.h
```
#define SECRET_SSID "YOUR_SSID"
``` 

Depending on the Security of the Wifi you want to connect change following lines of code

### WPA
Add the password to arduino_secrets.h
```
#define SECRET_PASS "YOUR_PASSWORD"
```
### OPEN WIFI - NO PASSWORD
Remove pass parameter in line 62 of e-ink-display.ino
```
status = WiFi.begin(ssid);
```

### UPLOAD
Now you can upload the code to your Arduino Board. Set Serial Monitor BAUD Rate to 115200 and take note of IP-Address the board got. So you can create a connection for the Board with the Spring-Backend in the Webapp by putting the IP-Address in the Network Address field when you create the connection.