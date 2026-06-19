#ifndef WEB_APP_H
#define WEB_APP_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include <vector>

#include "leaderboard_store.h"
#include "reaction_game.h"

class WebApp {
 public:
  WebApp(LeaderboardStore& store, ReactionGame& game) : store_(store), game_(game), server_(80) {}

  bool begin() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD, 1, false, AP_MAX_CLIENTS);

    IPAddress ip = WiFi.softAPIP();
    Serial.print("AP started. SSID: ");
    Serial.print(AP_SSID);
    Serial.print(", IP: ");
    Serial.println(ip.toString());

    if (!store_.begin()) {
      Serial.println("SPIFFS / leaderboard storage init failed");
      return false;
    }

    if (MDNS.begin(WEB_MDNS_HOSTNAME)) {
      MDNS.addService("http", "tcp", 80);
      Serial.print("mDNS ready: http://");
      Serial.print(WEB_MDNS_HOSTNAME);
      Serial.println(".local");
    } else {
      Serial.println("mDNS start failed");
    }

    setupRoutes();
    server_.begin();
    Serial.println("Web server started");
    return true;
  }

  void loop() {
    if (currentUser_.length() == 0) {
      return;
    }

    if (WiFi.softAPgetStationNum() == 0) {
      logoutInternal();
      return;
    }

    if ((millis() - lastHeartbeatMs_) > WEB_SESSION_TIMEOUT_MS) {
      logoutInternal();
    }
  }

  void handleGameResult(uint32_t elapsedMs, const String& roundOwnerDeviceId, const String& roundOwnerName) {
    if (roundOwnerDeviceId.length() == 0 || roundOwnerName.length() == 0) {
      return;
    }

    store_.recordResult(roundOwnerDeviceId, roundOwnerName, elapsedMs);
    if (currentClientId_ == roundOwnerDeviceId && (sessionBestMs_ == 0 || elapsedMs < sessionBestMs_)) {
      sessionBestMs_ = elapsedMs;
    }
    Serial.printf("[WEB] result stored for %s (%s): %lu ms\n",
                  roundOwnerName.c_str(),
                  roundOwnerDeviceId.c_str(),
                  static_cast<unsigned long>(elapsedMs));
  }

  bool isLoggedIn() const {
    return currentUser_.length() > 0;
  }

  GameSessionContext sessionContext() const {
    GameSessionContext context;
    context.loggedIn = currentUser_.length() > 0;
    context.clientId = currentClientId_;
    context.userName = currentUser_;
    context.ownerKey = currentClientId_ + "|" + currentUser_;
    return context;
  }

  String currentUser() const {
    return currentUser_;
  }

  String currentClientId() const {
    return currentClientId_;
  }

 private:
  LeaderboardStore& store_;
  ReactionGame& game_;
  AsyncWebServer server_;
  String currentUser_;
  String currentClientId_;
  uint32_t lastHeartbeatMs_ = 0;
  uint32_t sessionBestMs_ = 0;
  uint32_t lastResultMs_ = 0;

  void setupRoutes() {
    server_.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
      if (SPIFFS.exists("/index.html")) {
        request->send(SPIFFS, "/index.html", "text/html");
      } else {
        request->send(404, "text/plain", "index.html not found");
      }
    });

    server_.on("/api/state", HTTP_GET, [this](AsyncWebServerRequest* request) {
      request->send(200, "application/json", buildStateJson());
    });

    server_.on("/api/login", HTTP_POST, [this](AsyncWebServerRequest* request) {
      if (!request->hasParam("deviceId", true)) {
        request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing deviceId\"}");
        return;
      }

      if (!request->hasParam("name", true)) {
        request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing name\"}");
        return;
      }

      String deviceId = request->getParam("deviceId", true)->value();
      deviceId.trim();
      if (deviceId.length() == 0) {
        request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing deviceId\"}");
        return;
      }

      String name = request->getParam("name", true)->value();
      name.trim();
      if (name.length() == 0) {
        request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing name\"}");
        return;
      }

      currentClientId_ = deviceId;
      currentUser_ = name;
      lastHeartbeatMs_ = millis();
      lastResultMs_ = 0;
      sessionBestMs_ = store_.bestTimeFor(currentClientId_, currentUser_);
      Serial.printf("[WEB] login device=%s user=%s\n", currentClientId_.c_str(), currentUser_.c_str());
      request->send(200, "application/json", buildStateJson());
    });

    server_.on("/api/logout", HTTP_POST, [this](AsyncWebServerRequest* request) {
      Serial.printf("[WEB] logout device=%s user=%s\n", currentClientId_.c_str(), currentUser_.c_str());
      logoutInternal();
      request->send(200, "application/json", "{\"ok\":true}");
    });

    server_.on("/api/ping", HTTP_POST, [this](AsyncWebServerRequest* request) {
      if (currentUser_.length() > 0) {
        lastHeartbeatMs_ = millis();
      }
      request->send(200, "application/json", buildStateJson());
    });
  }

  String buildStateJson() const {
    DynamicJsonDocument doc(8192);
    doc["loggedIn"] = currentUser_.length() > 0;
    doc["user"] = currentUser_;
    doc["deviceId"] = currentClientId_;
    doc["sessionBestMs"] = sessionBestMs_;
    doc["lastResultMs"] = lastResultMs_;
    doc["gameSwitchOn"] = game_.isPowerSwitchOn();

    JsonArray lb = doc.createNestedArray("leaderboard");
    std::vector<ScoreEntry> rows = store_.leaderboard();
    for (const auto& row : rows) {
      JsonObject item = lb.add<JsonObject>();
      item["name"] = row.name;
      item["ms"] = row.ms;
    }

    JsonArray history = doc.createNestedArray("history");
    if (currentUser_.length() > 0 && currentClientId_.length() > 0) {
      std::vector<uint32_t> times = store_.historyFor(currentClientId_, currentUser_);
      for (uint32_t ms : times) {
        history.add(ms);
      }
    }

    String output;
    serializeJson(doc, output);
    return output;
  }

  void logoutInternal() {
    currentUser_.remove(0);
    currentClientId_.remove(0);
    sessionBestMs_ = 0;
    lastResultMs_ = 0;
    lastHeartbeatMs_ = 0;
  }
};

#endif // WEB_APP_H