#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <SPIFFS.h>
#include <FS.h>
#include <WiFiUdp.h>
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <ArduinoJson.h>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#include <vector>
#include <algorithm>

// Global instances
extern WebServer server;
extern WiFiUDP udp;
// Configuration
#include "settings.h"

// DNS Server: responds to all queries with AP IP
void handleDNS() {
  int packetSize = udp.parsePacket();
  if (!packetSize) return;

  byte packet[512] = {0};
  int len = udp.read(packet, 512);
  IPAddress remote = udp.remoteIP();
  int port = udp.remotePort();

  byte response[512] = {0};
  memcpy(response, packet, 12);
  response[2] |= 0x80;
  response[6] = 0x00;
  response[7] = 0x01;

  int qlen = 0, i = 12;
  while (i < len && (packet[i] != 0 || qlen == 0)) {
    qlen++;
    i++;
  }
  qlen += 5;
  memcpy(response + 12, packet + 12, qlen);

  int anspos = 12 + qlen;
  response[anspos++] = 0xC0;
  response[anspos++] = 0x0C;
  response[anspos++] = 0x00;
  response[anspos++] = 0x01;
  response[anspos++] = 0x00;
  response[anspos++] = 0x01;
  response[anspos++] = 0x00;
  response[anspos++] = 0x00;
  response[anspos++] = 0x00;
  response[anspos++] = 0xFF;
  response[anspos++] = 0x00;
  response[anspos++] = 0x04;

  IPAddress apIP = WiFi.softAPIP();
  response[anspos++] = apIP[0];
  response[anspos++] = apIP[1];
  response[anspos++] = apIP[2];
  response[anspos++] = apIP[3];

  udp.beginPacket(remote, port);
  udp.write(response, anspos);
  udp.endPacket();
}

// Serve index.html from SPIFFS
void handleRoot() {
  if (SPIFFS.exists("/index.html")) {
    fs::File f = SPIFFS.open("/index.html", "r");
    server.streamFile(f, "text/html");
    f.close();
    return;
  }
  server.send(200, "text/plain", "Reaktionsspiel");
}

// GET /scores: return leaderboard as JSON
void handleGetScores() {
  if (!SPIFFS.exists(SCORES_FILE)) {
    server.send(200, "application/json", "[]");
    return;
  }
  fs::File f = SPIFFS.open(SCORES_FILE, "r");
  if (!f) {
    server.send(500, "text/plain", "Failed to open scores file");
    return;
  }
  String content;
  while (f.available()) content += (char)f.read();
  f.close();
  server.send(200, "application/json", content);
}

// POST /score: add new score to leaderboard
void handlePostScore() {
  String name;
  int score = 0;
  String body = server.arg("plain");

  if (body.length() > 0) {
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    StaticJsonDocument<512> bodyDoc;
    DeserializationError err = deserializeJson(bodyDoc, body);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    if (!err) {
      if (!bodyDoc["name"].isNull()) name = String(bodyDoc["name"].as<const char*>());
      if (!bodyDoc["score"].isNull()) score = bodyDoc["score"].as<int>();
    }
  } else {
    if (server.hasArg("name")) name = server.arg("name");
    if (server.hasArg("score")) score = server.arg("score").toInt();
  }

  if (name.length() == 0) {
    server.send(400, "text/plain", "Missing name");
    return;
  }

  // Load existing scores
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  StaticJsonDocument<16384> doc;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  std::vector<std::pair<int, String>> v;

  if (SPIFFS.exists(SCORES_FILE)) {
    fs::File f = SPIFFS.open(SCORES_FILE, "r");
    if (f) {
      DeserializationError err = deserializeJson(doc, f);
      f.close();
      if (!err) {
        JsonArray arr = doc.as<JsonArray>();
        for (JsonObject obj : arr) {
          const char* n = obj["name"] | "";
          int s = obj["score"] | 0;
          v.emplace_back(s, String(n));
        }
      }
    }
  }

  // Add new score, sort descending, keep top N
  v.emplace_back(score, name);
  std::sort(v.begin(), v.end(), [](const std::pair<int, String>& a, const std::pair<int, String>& b) {
    return a.first > b.first;
  });
  if (v.size() > MAX_SCORES) v.resize(MAX_SCORES);

  // Write back to SPIFFS
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  StaticJsonDocument<16384> outdoc;
  JsonArray outArr = outdoc.to<JsonArray>();
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  for (auto& p : v) {
    JsonObject o = outArr.add<JsonObject>();
    o["name"] = p.second.c_str();
    o["score"] = p.first;
  }

  fs::File wf = SPIFFS.open(SCORES_FILE, "w");
  if (!wf) {
    server.send(500, "text/plain", "Failed to open scores file for write");
    return;
  }
  serializeJson(outdoc, wf);
  wf.close();

  // Return updated list
  String resp;
  serializeJson(outdoc, resp);
  server.send(200, "application/json", resp);
}

// Initialize Access Point
void initAP() {
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP started. SSID: ");
  Serial.print(AP_SSID);
  Serial.print(", IP: ");
  Serial.println(ip.toString());
}

// Initialize DNS Server
void initDNS() {
  if (udp.begin(DNS_PORT)) {
    Serial.println("DNS server started on port 53");
  } else {
    Serial.println("DNS server failed to start");
  }
}

// Initialize OTA
void initOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.onStart([]() { Serial.println("OTA Update Start"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nOTA Update End"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

// Initialize WebServer with all routes
void initWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/scores", HTTP_GET, handleGetScores);
  server.on("/score", HTTP_POST, handlePostScore);
  server.begin();
  Serial.println("HTTP server started");
}

// Handle all network tasks (call from loop)
void networkLoop() {
  ArduinoOTA.handle();
  server.handleClient();
  handleDNS();
}

#endif
