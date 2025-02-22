#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266Ping.h>
#include <WiFiUdp.h>

//=================================================
//
// Made by.:
//     ____                  __  ____________  ____ 
//    / __ \____ __      ____\ \/ <  <  / __ \( __ )
//   / / / / __ `/ | /| / / _ \  // // / / / / __  |
//  / /_/ / /_/ /| |/ |/ /  __/ // // / /_/ / /_/ / 
// /_____/\__,_/ |__/|__/\___/_//_//_/\____/\____/ 
// 
//=================================================                                               

//=================================================
// CONFIG SECTION
//=================================================

//The WIFI Router SSID
const char* ssid = "SSID";

//The WIFI Password
const char* password = "PASSWORD";

//Admin password (For the page login)
const char* adminPassword = "admin";

//The webserver port, which runned by the ESP8266
const int serverPort = 80;

//The relay pin (If you dont have a relay ignore it)
const int RELAY_PIN = 12; 

//The pc online check interval in milliseconds
const unsigned long STATUS_CHECK_INTERVAL = 10000;  

//=================================================
// CODE SECTION
//=================================================

ESP8266WebServer server(serverPort);
bool isAuthenticated = false;
bool deviceStatus = false;
unsigned long lastStatusCheck = 0;
WiFiUDP udp;

String loginPage() {
    return R"rawliteral(
<!DOCTYPE html>
<html lang="hu">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WoL Login</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <style>
        @keyframes shake {
            0%, 100% { transform: translateX(0); }
            25% { transform: translateX(-5px); }
            75% { transform: translateX(5px); }
        }
        .shake { animation: shake 0.5s cubic-bezier(.36,.07,.19,.97) both; }
        
        @keyframes glow {
            0% { box-shadow: 0 0 5px rgba(59, 130, 246, 0.5); }
            50% { box-shadow: 0 0 20px rgba(59, 130, 246, 0.8); }
            100% { box-shadow: 0 0 5px rgba(59, 130, 246, 0.5); }
        }
        
        .input-glow:focus {
            animation: glow 2s ease-in-out infinite;
        }

        .error-message {
            animation: fadeIn 0.3s ease-in-out;
        }

        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(-10px); }
            to { opacity: 1; transform: translateY(0); }
        }
    </style>
</head>
<body class="bg-gradient-to-br from-gray-900 via-gray-800 to-gray-900 text-gray-100 flex items-center justify-center min-h-screen p-4">
    <div class="backdrop-blur-sm bg-gray-800/50 p-8 rounded-2xl shadow-2xl w-full max-w-md border border-gray-700/50">
        <div class="relative mb-8">
            <h2 class="text-4xl font-bold text-center bg-clip-text text-transparent bg-gradient-to-r from-blue-400 to-cyan-300">Wake-on-LAN</h2>
            <div class="h-1 w-32 bg-gradient-to-r from-blue-500 to-cyan-400 mx-auto mt-3 rounded-full"></div>
        </div>
        
        <form id="loginForm" action="/login" method="POST" class="space-y-6">
            <div class="space-y-2">
                <label class="block text-sm font-medium text-gray-300">Password</label>
                <input type="password" name="password" 
                    class="w-full px-4 py-3 border border-gray-600 rounded-xl bg-gray-700/50 text-white focus:ring-2 focus:ring-blue-500 focus:border-transparent transition-all duration-300 input-glow"
                    required>
                <div id="errorMessage" class="hidden error-message mt-2 text-rose-500 text-sm font-medium"></div>
            </div>
            
            <button type="submit" 
                class="w-full bg-gradient-to-br from-blue-500 to-cyan-600 text-white py-4 rounded-xl hover:from-blue-600 hover:to-cyan-700 shadow-lg hover:shadow-blue-500/50 transition-all duration-300 font-medium mt-8">
                Login
            </button>
        </form>
    </div>

    <script>
        const form = document.getElementById('loginForm');
        const errorMessage = document.getElementById('errorMessage');

        form.addEventListener('submit', async (e) => {
            e.preventDefault();
            const formData = new FormData(form);
            
            try {
                const response = await fetch('/login', {
                    method: 'POST',
                    body: formData
                });
                
                if (response.ok) {
                    window.location.href = '/wol';
                } else {
                    form.classList.add('shake');
                    errorMessage.textContent = 'Incorrect password. Please try again.';
                    errorMessage.classList.remove('hidden');
                    setTimeout(() => form.classList.remove('shake'), 500);
                }
            } catch (error) {
                errorMessage.textContent = 'An error occurred. Please try again.';
                errorMessage.classList.remove('hidden');
            }
        });

        // Clear error message when user starts typing
        document.querySelector('input[name="password"]').addEventListener('input', () => {
            errorMessage.classList.add('hidden');
        });
    </script>
</body>
</html>
    )rawliteral";
}

String mainPage() {
    return R"rawliteral(
<!DOCTYPE html>
<html lang="hu">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PC Power Control</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <style>
        @keyframes pulse {
            0% { transform: scale(1); box-shadow: 0 0 0 0 rgba(59, 130, 246, 0.7); }
            50% { transform: scale(1.05); box-shadow: 0 0 20px 10px rgba(59, 130, 246, 0.3); }
            100% { transform: scale(1); box-shadow: 0 0 0 0 rgba(59, 130, 246, 0); }
        }
        .pulse { animation: pulse 2s cubic-bezier(0.4, 0, 0.6, 1) infinite; }
        
        @keyframes spin {
            to { transform: rotate(360deg); }
        }
        .spin { animation: spin 1s linear infinite; }
        
        @keyframes glow {
            0% { box-shadow: 0 0 5px rgba(59, 130, 246, 0.5); }
            50% { box-shadow: 0 0 20px rgba(59, 130, 246, 0.8); }
            100% { box-shadow: 0 0 5px rgba(59, 130, 246, 0.5); }
        }
        
        .control-button {
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            position: relative;
            overflow: hidden;
        }
        
        .control-button:before {
            content: '';
            position: absolute;
            top: 50%;
            left: 50%;
            width: 300%;
            height: 300%;
            background: rgba(255, 255, 255, 0.1);
            transform: translate(-50%, -50%) rotate(45deg);
            transition: 0.5s;
        }
        
        .control-button:hover:before {
            width: 105%;
            height: 105%;
        }
        
        .input-glow:focus {
            animation: glow 2s ease-in-out infinite;
        }
    </style>
</head>
<body class="bg-gradient-to-br from-gray-900 via-gray-800 to-gray-900 text-gray-100 flex items-center justify-center min-h-screen p-4">
    <div class="backdrop-blur-sm bg-gray-800/50 p-8 rounded-2xl shadow-2xl w-full max-w-md border border-gray-700/50">
        <div class="relative mb-8">
            <h2 class="text-4xl font-bold text-center bg-clip-text text-transparent bg-gradient-to-r from-blue-400 to-cyan-300">PC Power Control</h2>
            <div class="h-1 w-32 bg-gradient-to-r from-blue-500 to-cyan-400 mx-auto mt-3 rounded-full"></div>
        </div>
        
        <div class="space-y-6">
            <div class="space-y-2">
                <label class="block text-sm font-medium text-gray-300">IP Address</label>
                <input type="text" id="ip" placeholder="192.168.1.100" 
                    class="w-full px-4 py-3 border border-gray-600 rounded-xl bg-gray-700/50 text-white focus:ring-2 focus:ring-blue-500 focus:border-transparent transition-all duration-300 input-glow">
            </div>
            
            <div class="space-y-2">
                <label class="block text-sm font-medium text-gray-300">MAC Address</label>
                <input type="text" id="mac" placeholder="00:11:22:33:44:55" 
                    class="w-full px-4 py-3 border border-gray-600 rounded-xl bg-gray-700/50 text-white focus:ring-2 focus:ring-blue-500 focus:border-transparent transition-all duration-300 input-glow">
            </div>

            <div class="grid grid-cols-3 gap-4">
                <button onclick="sendWoL()" 
                    class="control-button bg-gradient-to-br from-emerald-500 to-green-600 text-white py-4 px-2 rounded-xl hover:from-emerald-600 hover:to-green-700 shadow-lg hover:shadow-emerald-500/50 font-medium">
                    Wake Up
                </button>
                <button onclick="powerControl('on')" 
                    class="control-button bg-gradient-to-br from-blue-500 to-cyan-600 text-white py-4 px-2 rounded-xl hover:from-blue-600 hover:to-cyan-700 shadow-lg hover:shadow-blue-500/50 font-medium">
                    Power On (Relay)
                </button>
                <button onclick="powerControl('off')" 
                    class="control-button bg-gradient-to-br from-red-500 to-rose-600 text-white py-4 px-2 rounded-xl hover:from-red-600 hover:to-rose-700 shadow-lg hover:shadow-red-500/50 font-medium">
                    Power Off (Relay)
                </button>
            </div>

            <div id="status" class="mt-8 text-center backdrop-blur-sm bg-gray-700/30 p-6 rounded-xl border border-gray-600/50">
                <div id="statusIcon" class="mx-auto w-16 h-16 mb-4"></div>
                <p id="statusText" class="text-lg font-medium bg-clip-text"></p>
            </div>
        </div>
    </div>

    <script>
        let checkingStatus = false;

        function validateMac(mac) {
            return /^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$/.test(mac);
        }

        function validateIp(ip) {
            return /^(\d{1,3}\.){3}\d{1,3}$/.test(ip);
        }

        async function updateStatus() {
            const ip = document.getElementById('ip').value;
            if (!validateIp(ip)) {
                return;
            }
            
            try {
                const response = await fetch(`/status?ip=${ip}`);
                const status = await response.text();
                
                const statusDiv = document.getElementById('status');
                const statusIcon = document.getElementById('statusIcon');
                const statusText = document.getElementById('statusText');
                
                statusDiv.classList.remove('hidden');
                
                if (status === 'Online') {
                    statusIcon.innerHTML = `<div class="w-16 h-16 rounded-full bg-gradient-to-r from-green-400 to-emerald-500 pulse"></div>`;
                    statusText.textContent = 'Device is Online';
                    statusText.className = 'text-lg font-medium text-transparent bg-gradient-to-r from-green-400 to-emerald-500 bg-clip-text';
                } else {
                    statusIcon.innerHTML = `<div class="w-16 h-16 rounded-full bg-gradient-to-r from-red-400 to-rose-500"></div>`;
                    statusText.textContent = 'Device is Offline';
                    statusText.className = 'text-lg font-medium text-transparent bg-gradient-to-r from-red-400 to-rose-500 bg-clip-text';
                }
            } catch (error) {
                console.error('Error checking status:', error);
            }
        }

        async function sendWoL() {
            const mac = document.getElementById('mac').value;
            if (!validateMac(mac)) {
                alert('Please enter a valid MAC address');
                return;
            }
            
            const statusDiv = document.getElementById('status');
            const statusIcon = document.getElementById('statusIcon');
            const statusText = document.getElementById('statusText');
            
            statusDiv.classList.remove('hidden');
            statusIcon.innerHTML = `
                <svg class="spin text-blue-500 w-16 h-16" viewBox="0 0 24 24">
                    <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4" fill="none"/>
                    <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"/>
                </svg>
            `;
            statusText.textContent = 'Sending WoL packet...';
            statusText.className = 'text-lg font-medium text-blue-400';
            
            try {
                const response = await fetch(`/wol-request?mac=${mac}`);
                if (response.ok) {
                    statusIcon.innerHTML = `<div class="w-16 h-16 rounded-full bg-gradient-to-r from-green-400 to-emerald-500 pulse"></div>`;
                    statusText.textContent = 'Wake-on-LAN packet sent successfully';
                    statusText.className = 'text-lg font-medium text-transparent bg-gradient-to-r from-green-400 to-emerald-500 bg-clip-text';
                } else {
                    throw new Error('Failed to send WoL packet');
                }
            } catch (error) {
                statusIcon.innerHTML = `<div class="w-16 h-16 rounded-full bg-gradient-to-r from-red-400 to-rose-500"></div>`;
                statusText.textContent = 'Failed to send Wake-on-LAN packet';
                statusText.className = 'text-lg font-medium text-transparent bg-gradient-to-r from-red-400 to-rose-500 bg-clip-text';
            }
        }

        async function powerControl(action) {
            const statusDiv = document.getElementById('status');
            const statusIcon = document.getElementById('statusIcon');
            const statusText = document.getElementById('statusText');
            
            statusDiv.classList.remove('hidden');
            statusIcon.innerHTML = `
                <svg class="spin text-blue-500 w-16 h-16" viewBox="0 0 24 24">
                    <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4" fill="none"/>
                    <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"/>
                </svg>
            `;
            statusText.textContent = `${action === 'on' ? 'Powering on' : 'Powering off'} via relay...`;
            statusText.className = 'text-lg font-medium text-blue-400';
            
            try {
                const response = await fetch(`/power-${action}`);
                if (response.ok) {
                    statusIcon.innerHTML = `<div class="w-16 h-16 rounded-full bg-gradient-to-r from-green-400 to-emerald-500 pulse"></div>`;
                    statusText.textContent = `Power ${action === 'on' ? 'on' : 'off'} successful`;
                    statusText.className = 'text-lg font-medium text-transparent bg-gradient-to-r from-green-400 to-emerald-500 bg-clip-text';
                } else {
                    throw new Error(`Failed to power ${action}`);
                }
            } catch (error) {
                statusIcon.innerHTML = `<div class="w-16 h-16 rounded-full bg-gradient-to-r from-red-400 to-rose-500"></div>`;
                statusText.textContent = `Failed to power ${action}`;
                statusText.className = 'text-lg font-medium text-transparent bg-gradient-to-r from-red-400 to-rose-500 bg-clip-text';
            }
        }

        // Start periodic status updates
        setInterval(updateStatus, 10000);
        // Initial status check
        updateStatus();
    </script>
</body>
</html>
    )rawliteral";
}

// Compute broadcast IP address
IPAddress getBroadcastIP() {
    IPAddress localIP = WiFi.localIP();
    IPAddress subnetMask = WiFi.subnetMask();
    return IPAddress(localIP[0] | ~subnetMask[0],
                    localIP[1] | ~subnetMask[1],
                    localIP[2] | ~subnetMask[2],
                    localIP[3] | ~subnetMask[3]);
}

byte hexToByte(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;  // Invalid hex char
}

// WoL packet sender
bool sendWOL(String macStr) {
    // Validate MAC address format first
    if (macStr.length() != 17) {
        Serial.println("Invalid MAC address length");
        return false;
    }

    // Convert MAC string to bytes
    byte mac[6];
    char *str = (char*)macStr.c_str();
    char *p = str;
    for (int i = 0; i < 6; i++) {
        // Check for valid hex chars
        if (!isxdigit(p[0]) || !isxdigit(p[1])) {
            Serial.println("Invalid MAC address characters");
            return false;
        }
        // Convert two chars to one byte
        mac[i] = (hexToByte(p[0]) << 4) | hexToByte(p[1]);
        // Skip to next pair, checking separator
        p += 2;
        if (i < 5) {
            if (*p != ':') {
                Serial.println("Invalid MAC address separator");
                return false;
            }
            p++;
        }
    }

    // Create magic packet on stack with fixed size
    byte magicPacket[102];
    
    // Initialize first 6 bytes to 0xFF
    for (int i = 0; i < 6; i++) {
        magicPacket[i] = 0xFF;
    }
    
    // Copy MAC address 16 times
    for (int i = 1; i <= 16; i++) {
        memcpy(&magicPacket[i * 6], mac, 6);
    }

    // Get broadcast address
    IPAddress broadcastIP = getBroadcastIP();
    
    bool success = false;
    const uint16_t ports[] = {7, 9};  // Standard WoL ports

    for (uint16_t port : ports) {
        if (udp.begin(2390)) {  // Use a specific local port
            delay(10);  // Small delay to ensure UDP is ready
            
            if (udp.beginPacket(broadcastIP, port)) {
                Serial.printf("Sending WoL to %s:%d\n", broadcastIP.toString().c_str(), port);
                
                // Write in smaller chunks to avoid memory issues
                const int chunkSize = 32;
                for (int i = 0; i < sizeof(magicPacket); i += chunkSize) {
                    int remaining = sizeof(magicPacket) - i;
                    int writeSize = remaining < chunkSize ? remaining : chunkSize;
                    udp.write(&magicPacket[i], writeSize);
                    yield();  // Allow system tasks to run
                }
                
                if (udp.endPacket()) {
                    success = true;
                    Serial.printf("WoL packet sent successfully to port %d\n", port);
                } else {
                    Serial.printf("Failed to send WoL packet to port %d\n", port);
                }
            } else {
                Serial.println("Failed to begin UDP packet");
            }
            
            udp.stop();
            delay(100);  // Delay between attempts
        } else {
            Serial.println("Failed to begin UDP");
        }
    }

    return success;
}

void checkDeviceStatus() {
    if (millis() - lastStatusCheck >= STATUS_CHECK_INTERVAL) {
        String ipStr = WiFi.localIP().toString(); 
        IPAddress targetIP;
        if (WiFi.hostByName(ipStr.c_str(), targetIP)) {
            deviceStatus = false;
            for (int i = 0; i < 3; i++) {
                if (Ping.ping(targetIP, 1)) {
                    deviceStatus = true;
                    break;
                }
                delay(100);
            }
        }
        lastStatusCheck = millis();
    }
}

void setup() {
    Serial.begin(9600);
    
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);  // active LOW relay
    
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, []() {
        if (!isAuthenticated) {
            server.send(200, "text/html", loginPage());
        } else {
            server.sendHeader("Location", "/wol");
            server.send(302, "text/plain", "");
        }
    });

    server.on("/login", HTTP_POST, []() {
        if (server.arg("password") == adminPassword) {
            isAuthenticated = true;
            server.sendHeader("Location", "/wol");
            server.send(302, "text/plain", "");
        } else {
            server.send(401, "text/html", loginPage());
        }
    });

    server.on("/wol", HTTP_GET, []() {
        if (!isAuthenticated) {
            server.sendHeader("Location", "/");
            server.send(302, "text/plain", "");
        } else {
            server.send(200, "text/html", mainPage());
        }
    });

    server.on("/status", HTTP_GET, []() {
        if (!isAuthenticated) {
            server.send(401, "text/plain", "Unauthorized");
            return;
        }

        String ipStr = server.arg("ip");
        IPAddress targetIP;
        if (WiFi.hostByName(ipStr.c_str(), targetIP)) {
            bool isOnline = false;
            for (int i = 0; i < 3; i++) {  // Try 3 times
                if (Ping.ping(targetIP, 1)) {
                    isOnline = true;
                    break;
                }
                delay(100);
            }
            server.send(200, "text/plain", isOnline ? "Online" : "Offline");
        } else {
            server.send(200, "text/plain", "Offline");
        }
    });

    server.on("/wol-request", HTTP_GET, []() {
        if (!isAuthenticated) {
            server.send(401, "text/plain", "Unauthorized");
            return;
        }
        
        String mac = server.arg("mac");
        bool success = sendWOL(mac);
        
        if (success) {
            server.send(200, "text/plain", "WoL packet sent successfully");
        } else {
            server.send(400, "text/plain", "Failed to send WoL packet");
        }
    });

    // Add new endpoints for relay control
    server.on("/power-on", HTTP_GET, []() {
        if (!isAuthenticated) {
            server.send(401, "text/plain", "Unauthorized");
            return;
        }
        
        digitalWrite(RELAY_PIN, LOW);  // Activate relay (assuming active LOW)
        delay(500);  // Hold for 500ms
        digitalWrite(RELAY_PIN, HIGH); // Release relay
        
        server.send(200, "text/plain", "Power on signal sent");
    });

    server.on("/power-off", HTTP_GET, []() {
        if (!isAuthenticated) {
            server.send(401, "text/plain", "Unauthorized");
            return;
        }
        
        digitalWrite(RELAY_PIN, LOW);  // Activate relay
        delay(5000);  // Hold for 5 seconds to force shutdown
        digitalWrite(RELAY_PIN, HIGH); // Release relay
        
        server.send(200, "text/plain", "Power off signal sent");
    });

    // Handle not found
    server.onNotFound([]() {
        if (!isAuthenticated) {
            server.sendHeader("Location", "/");
            server.send(302, "text/plain", "");
        } else {
            server.send(404, "text/plain", "Not found");
        }
    });

    // Start server
    server.begin();
    Serial.println("HTTP server started on port " + String(serverPort));
    
    // Print network information
    Serial.println("Network Information:");
    Serial.println("Local IP: " + WiFi.localIP().toString());
    Serial.println("Subnet Mask: " + WiFi.subnetMask().toString());
    Serial.println("Gateway IP: " + WiFi.gatewayIP().toString());
    Serial.println("Broadcast IP: " + getBroadcastIP().toString());
}

void loop() {
    // Handle client requests
    server.handleClient();
    
    // Check device status periodically
    checkDeviceStatus();
    
    // Monitor WiFi connection
    static unsigned long lastWiFiCheck = 0;
    static bool wasConnected = false;
    
    if (millis() - lastWiFiCheck > 30000) {  // Check every 30 seconds
        lastWiFiCheck = millis();
        
        if (WiFi.status() != WL_CONNECTED) {
            if (wasConnected) {
                Serial.println("WiFi connection lost! Attempting to reconnect...");
                wasConnected = false;
            }
            
            WiFi.reconnect();
        } else if (!wasConnected) {
            Serial.println("WiFi reconnected!");
            Serial.println("IP: " + WiFi.localIP().toString());
            wasConnected = true;
        }
    }
    
    // Prevent watchdog reset
    yield();
}