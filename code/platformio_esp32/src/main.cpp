// Reaction Game - ESP32 Access Point + Leaderboard
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>
#include "network.h"

// Global instances used by network.h
WebServer server(80);
WiFiUDP udp;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n=== ReactionGame Starting ===");

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed!");
    return;
  }
  Serial.println("SPIFFS initialized");

  // Initialize network components
  initAP();
  initDNS();
  initOTA();
  initWebServer();

  Serial.println("Setup complete - waiting for connections...");
}

void loop() {
  // Network loop handles OTA, HTTP requests, and DNS queries
  networkLoop();
}
