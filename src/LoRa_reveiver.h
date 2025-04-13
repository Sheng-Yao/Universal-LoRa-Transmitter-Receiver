#include <Arduino.h>
// SPI for LoRa module control
#include <SPI.h>
// Incuding LoRa library
#include <LoRa.h>
// WiFi and API (POST and GET) functionality
#include <WiFi.h>
#include <HTTPClient.h>
// JSON library for message extraction and JSON file creation
#include <ArduinoJson.h>

// Wi-Fi settings
const char WIFI_SSID[] = "XXX";  // Change to LoRa Receiver WiFi
const char WIFI_PASSWORD[] = "XXX";

// API link
String SERVER_LINK = "XXX";

//define the pins used by the transceiver module
#define NSS 4
#define RST 5
#define DI0 2

void setup() {

  //initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("LoRa Receiver");


  // WiFi connection
  Serial.print("Connecting to " + String(WIFI_SSID) + " ...");
  // Set ESP32 for station mode only
  WiFi.mode(WIFI_MODE_STA);
  // Wi-Fi connection setup
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Timeout timer
  unsigned long connectingTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // Restart ESP32 if timeout
    if(millis() - connectingTime >= 15000){
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());


  // LoRa setup
  Serial.print("Setup LoRa ...");
  // LoRa module setup
  LoRa.setPins(NSS, RST, DI0);
  
  while (!LoRa.begin(433E6)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  
  // LoRa parameter settings
  LoRa.setPreambleLength(6); // 6
  LoRa.enableCrc();
  LoRa.setSpreadingFactor(7); // 12
  LoRa.setSignalBandwidth(500E3); // 125E3
  LoRa.setCodingRate4(5); // 8
  LoRa.setSyncWord(0x14);

  Serial.println("LoRa Gateway Initializing Successful!");
}

HTTPClient http;

// Handle flow if message failed to sent
byte trials = 0;
unsigned long timer = 0;
String pendingAPI = "";
String pendingMessage = "";
String pendingMessageType = "";

String device_name = "XXX";
String x_api_key = "XXX";

void loop() {

  // Restart ESP32 every 20 days
  if(millis() >= 1728000){
    ESP.restart();
  }

  // Constantly check Wi-Fi connection (Looping in connecting)
  if(WiFi.status() != WL_CONNECTED){
    WiFi.reconnect();
    Serial.print("Reconnecting...");
    // Timeout timer
    unsigned long connectingTime = millis();
    while(WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(".");
      // Restart ESP32 if timeout
      if(millis() - connectingTime >= 60000){
        ESP.restart();
      }
    }
    Serial.println();
    Serial.println("Reconnected to " + String(WIFI_SSID));
  }


  String LoRaData;
  // LoRa data packet size received from LoRa sender
  int packetSize = LoRa.parsePacket();

  // if the packer size is not 0, then execute this if condition
  if (packetSize) {

    if(LoRa.packetRssi() < 0){
      Serial.println("Received packet with valid CRC");
    }else{
      Serial.println("Received packet with invalid CRC");
      return;
    }

    // received a packet
    Serial.print("Received packet: ");
    // receiving the data from LoRa sender
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
    }
    Serial.println(LoRaData);


    JsonDocument doc;
    String outputMessage = "";

    String APILink = "";
    
    LoRaData.remove(0,1);
    byte endingIndex = LoRaData.indexOf('/');
    if(LoRaData.substring(0,endingIndex) == "detect"){
        
        // Reset pending settings
        pendingAPI = "";
        pendingMessage = "";
        pendingMessageType = "";

        LoRaData.remove(0,endingIndex);
        byte element = 0;
        while(LoRaData.length() > 0){
            LoRaData.remove(0,1);
            byte endingIndex = LoRaData.indexOf('/');
            switch (element)
            {
            case 0:
                doc["confidence_level_audio"] = LoRaData.substring(0,endingIndex).toFloat();
                LoRaData.remove(0,endingIndex);
                element++;
                continue;
            
            case 1:
                doc["confidence_level_camera"] = LoRaData.substring(0,endingIndex).toFloat();
                LoRaData.remove(0,endingIndex);
                element++;
                continue;

            case 2:
                doc["device_name"] = device_name;
                doc["detected_at"] = LoRaData.substring(0,endingIndex);
                LoRaData.remove(0,endingIndex);
                element++;
                break;

            case 3:
                LoRaData.remove(0,1);
                break;

            default:
                continue;
            }
        }
        doc["audio_detected"] = false;
        doc["camera_detected"] = false;

        // Compiled and sent to server
        serializeJson(doc,outputMessage);
        Serial.println(outputMessage);
        outputMessage = "payload=" + outputMessage;
        
        APILink = SERVER_LINK + "detect";
        http.begin(APILink);
        http.addHeader("x-api-key",x_api_key);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        int httpCode = http.POST(outputMessage);

        if(httpCode == 200){
            Serial.println("Successfully done detect.");
            trials = 0;
            Serial.println(http.getString());
        }else{
            Serial.println("Failed to sent via /detect.");
            trials = 1;
            Serial.println(http.getString());
            pendingMessageType = "detect";
            pendingAPI = APILink;
            pendingMessage = outputMessage;
            timer = millis();
        }

        http.end();

    }else if(LoRaData.substring(0,endingIndex) == "handshake"){

        // Reset pending settings
        pendingAPI = "";
        pendingMessage = "";
        pendingMessageType = "";
        
        APILink = SERVER_LINK + "handshake";

        http.begin(APILink);
        http.addHeader("x-device-name",device_name);
        http.addHeader("x-api-key",x_api_key);
        int httpCode = http.POST("");

        Serial.println(httpCode);

        if(httpCode == 200){
            Serial.println("Successfully done handshake.");
            trials = 0;
            Serial.println(http.getString());

        }else{
            Serial.println("Failed to sent via /handshake.");
            trials = 1;
            Serial.println(http.getString());
            pendingMessageType = "handshake";
            pendingAPI = APILink;
            timer = millis();
        }

        http.end();

    }
  }else{
    if(trials >= 1 && trials < 3){

      // Trials being done every 10 second
      if(millis() - timer >= 10000){

        if(pendingMessageType == "detect"){
            http.begin(pendingAPI);
            http.addHeader("x-api-key",x_api_key);
            http.addHeader("Content-Type", "application/x-www-form-urlencoded");
            int httpCode = http.POST(pendingMessage);
            if(httpCode == 200){
                Serial.println("Successfully done detect.");
                trials = 0;
                Serial.println(http.getString());
                pendingMessageType = "";
                pendingAPI = "";
                pendingMessage = "";
            }else{
                Serial.println("Failed to sent via /detect.");
                trials++;
                Serial.println(http.getString());
            }

            http.end();

        }else if(pendingMessageType == "handshake"){
            http.begin(pendingAPI);
            http.addHeader("x-device-name",device_name);
            http.addHeader("x-api-key",x_api_key);
            int httpCode = http.POST("");
    
            Serial.println(httpCode);
    
            if(httpCode == 200){
                Serial.println("Successfully done handshake.");
                trials = 0;
                Serial.println(http.getString());
                pendingMessageType = "";
                pendingAPI = "";
                pendingMessage = "";

            }else{
                Serial.println("Failed to sent via /handshake.");
                trials++;
                Serial.println(http.getString());
            }

            http.end();
    
        }
        timer = millis();
      }
    }
  }
}