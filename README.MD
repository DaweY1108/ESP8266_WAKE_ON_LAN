# ESP8266 Wake-on-LAN Controller

<p align="center">
  <img src="[Add your project logo/banner here]" alt="ESP8266 WoL Controller" width="600">
</p>

A modern, responsive web interface for Wake-on-LAN functionality using ESP8266. Control your PC remotely with WoL packets and physical power control through a relay.

```
    ____                  __  ____________  ____ 
   / __ \____ __      ____\ \/ <  <  / __ \( __ )
  / / / / __ `/ | /| / / _ \  // // / / / / __  |
 / /_/ / /_/ /| |/ |/ /  __/ // // / /_/ / /_/ / 
/_____/\__,_/ |__/|__/\___/_//_//_/\____/\____/ 
```

## Features

- 🔐 Secure login system
- 💫 Modern, responsive web interface using Tailwind CSS
- 🌐 Wake-on-LAN packet sending
- 🔌 Physical power control through relay
- 📊 Real-time device status monitoring
- 🔄 Automatic reconnection on WiFi loss
- 📱 Mobile-friendly design

## Screenshots

[Add screenshots of your login screen and main interface here]

## Hardware Requirements

- ESP8266 board (NodeMCU, Wemos D1 Mini, etc.)
- Relay module (connected to pin 12 by default)
- Power supply for ESP8266
- Access to your local network

## Software Dependencies

- ESP8266WiFi
- WiFiClient
- WiFiServer
- ESP8266WebServer
- ESP8266Ping
- WiFiUdp

## Installation

1. Clone this repository
2. Open the project in Arduino IDE
3. Install required libraries through the Arduino Library Manager
4. Configure your settings in the CONFIG SECTION:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   const char* adminPassword = "YOUR_ADMIN_PASSWORD";
   const int serverPort = 80;
   const int RELAY_PIN = 12;
   const unsigned long STATUS_CHECK_INTERVAL = 10000;
   ```
5. Upload the code to your ESP8266

## Usage

1. Power up your ESP8266
2. Connect to your WiFi network
3. Access the web interface through your browser at `http://[ESP8266_IP]`
4. Log in using your configured admin password
5. Enter your PC's MAC address and IP address
6. Use the interface to:
   - Send Wake-on-LAN packets
   - Control power through relay
   - Monitor device status

## Configuration

### Relay Pin

The default relay pin is set to 12. You can change this in the CONFIG SECTION:
```cpp
const int RELAY_PIN = 12;
```

### Status Check Interval

Device status is checked every 10 seconds by default. Modify this in the CONFIG SECTION:
```cpp
const unsigned long STATUS_CHECK_INTERVAL = 10000;
```

### Web Server Port

The default web server port is 80. Change it in the CONFIG SECTION:
```cpp
const int serverPort = 80;
```

## Security

- Web interface is protected with a password
- All control functions require authentication
- Passwords are transmitted securely
- Configure a strong admin password before deployment

## Contributing

Feel free to submit issues and enhancement requests!

## License

[Add your chosen license here]

## Author

Made by DaweY

## Acknowledgments

- TailwindCSS for the beautiful UI
- ESP8266 Community for excellent libraries
- [Add any other acknowledgments]

## Future Plans

- [ ] Add HTTPS support
- [ ] Implement OTA updates
- [ ] Add device scheduling
- [ ] [Add your planned features]