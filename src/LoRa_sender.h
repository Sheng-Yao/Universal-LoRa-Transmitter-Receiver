#include <Arduino.h>
//Incuding arduino default SPI library
#include <SPI.h>
//Incuding LoRa library
#include <LoRa.h>
// JSON library for message extraction and JSON file creation
#include <ArduinoJson.h>

//define the pins used by the transceiver module
#define NSS 4
#define RST 5
#define DI0 2

// Source: https://embeddedthere.com/esp32-lora-tutorial-using-arduino-ide/

void setup() {

  //initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("LoRa Sender");

  // LoRa setup
  Serial.print("Setup LoRa ...");
  // LoRa module setup
  LoRa.setPins(NSS, RST, DI0);

  while (!LoRa.begin(433E6)) {
    Serial.print(".");
    delay(500);
  }
  // LoRa parameter settings
  LoRa.setPreambleLength(6); // 6
  LoRa.enableCrc();
  LoRa.setSpreadingFactor(7); // 12
  LoRa.setSignalBandwidth(500E3); // 125E3
  LoRa.setCodingRate4(5); // 8
  LoRa.setSyncWord(0x14);

  Serial.println("LoRa Gateway Initializing Successful!");
}

int count = 0;
byte message = 1;

void loop() {
  
  String message1 = "/detect/2025-03-15T22:52:05/" + String(random(70,99)/100.0) + "/" + String(random(70,99)/100.00) + "/A01/x/";
  String message2 = "/handshake/2025-03-15T22:52:05/" + String(random(25,99)/100.0) + "/A01/";

  unsigned long currentTime = millis();

  while(true){

    String LoRaData = "";

    // LoRa data packet size received from LoRa sender
    int packetSize = LoRa.parsePacket();

    // if the packer size is not 0, then execute this if condition
    if (packetSize) {
      
      // received a packet
      Serial.print("Received packet: ");

      // receiving the data from LoRa sender
      while (LoRa.available()) {
        LoRaData = LoRa.readString();
      }
      Serial.println(LoRaData);
    }

    if(LoRaData == "Received"){
      break;
    }

    if(millis()-currentTime >= 2000){
      String outputMessage = "";
      
      if(message == 1){
        outputMessage = message1;
        message = 2;
      }else if(message == 2){
        outputMessage = message2;
        message = 1;
      }

      Serial.println("Sending packet: " + outputMessage);

      // Start sending message1 via LoRa
      LoRa.beginPacket();
      LoRa.print(outputMessage.c_str());
      LoRa.endPacket();
      currentTime = millis();
    }
  }
}