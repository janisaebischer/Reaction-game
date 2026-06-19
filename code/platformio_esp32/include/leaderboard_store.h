#ifndef LEADERBOARD_STORE_H
#define LEADERBOARD_STORE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

#include <algorithm>
#include <vector>

#include "settings.h"

struct ScoreEntry {
  String deviceId;
  String name;
  uint32_t ms = 0;
};

struct UserHistory {
  String deviceId;
  String name;
  std::vector<uint32_t> times;
};

class LeaderboardStore {
 public:
  bool begin() {
    if (!SPIFFS.begin(true)) {
      return false;
    }

    loadLeaderboard();
    loadHistory();
    return true;
  }

  void recordResult(const String& deviceId, const String& name, uint32_t ms) {
    const String cleanedName = normalizeName(name);
    if (cleanedName.length() == 0) {
      return;
    }

    appendHistory(deviceId, cleanedName, ms);
    updateLeaderboard(deviceId, cleanedName, ms);
    saveLeaderboard();
    saveHistory();
  }

  bool hasLeaderboardEntry(const String& deviceId, const String& name) const {
    return findLeaderboardIndex(deviceId, name) >= 0;
  }

  uint32_t bestTimeFor(const String& deviceId, const String& name) const {
    const int index = findHistoryIndex(deviceId, name);
    if (index < 0) {
      return 0;
    }

    uint32_t bestMs = 0;
    for (uint32_t ms : histories_[static_cast<size_t>(index)].times) {
      if (bestMs == 0 || ms < bestMs) {
        bestMs = ms;
      }
    }
    return bestMs;
  }

  std::vector<ScoreEntry> leaderboard() const {
    return leaderboard_;
  }

  std::vector<uint32_t> historyFor(const String& deviceId, const String& name) const {
    const int index = findHistoryIndex(deviceId, name);
    if (index < 0) {
      return {};
    }

    return histories_[static_cast<size_t>(index)].times;
  }

 private:
  std::vector<ScoreEntry> leaderboard_;
  std::vector<UserHistory> histories_;

  String normalizeName(String name) const {
    name.trim();
    if (name.length() > MAX_NAME_LENGTH) {
      name.remove(MAX_NAME_LENGTH);
    }
    return name;
  }

  int findLeaderboardIndex(const String& deviceId, const String& name) const {
    for (size_t i = 0; i < leaderboard_.size(); ++i) {
      if (leaderboard_[i].name == name && leaderboard_[i].deviceId == deviceId) {
        return static_cast<int>(i);
      }
    }
    return -1;
  }

  int findHistoryIndex(const String& deviceId, const String& name) const {
    for (size_t i = 0; i < histories_.size(); ++i) {
      if (histories_[i].name == name && histories_[i].deviceId == deviceId) {
        return static_cast<int>(i);
      }
    }
    return -1;
  }

  UserHistory& ensureHistory(const String& deviceId, const String& name) {
    const int index = findHistoryIndex(deviceId, name);
    if (index >= 0) {
      return histories_[static_cast<size_t>(index)];
    }

    UserHistory history;
    history.deviceId = deviceId;
    history.name = name;
    histories_.push_back(history);
    return histories_.back();
  }

  void appendHistory(const String& deviceId, const String& name, uint32_t ms) {
    UserHistory& history = ensureHistory(deviceId, name);
    history.times.insert(history.times.begin(), ms);
    if (history.times.size() > WEB_HISTORY_LIMIT) {
      history.times.resize(WEB_HISTORY_LIMIT);
    }
  }

  void updateLeaderboard(const String& deviceId, const String& name, uint32_t ms) {
    const int existing = findLeaderboardIndex(deviceId, name);
    if (existing >= 0) {
      if (ms < leaderboard_[static_cast<size_t>(existing)].ms) {
        leaderboard_[static_cast<size_t>(existing)].ms = ms;
      }
    } else {
      ScoreEntry entry;
      entry.deviceId = deviceId;
      entry.name = name;
      entry.ms = ms;
      leaderboard_.push_back(entry);
    }

    std::sort(leaderboard_.begin(), leaderboard_.end(), [](const ScoreEntry& a, const ScoreEntry& b) {
      if (a.ms != b.ms) return a.ms < b.ms;
      return a.name < b.name;
    });

    if (leaderboard_.size() > MAX_SCORES) {
      leaderboard_.resize(MAX_SCORES);
    }
  }

  bool loadLeaderboard() {
    leaderboard_.clear();
    if (!SPIFFS.exists(LEADERBOARD_FILE)) {
      return true;
    }

    File file = SPIFFS.open(LEADERBOARD_FILE, "r");
    if (!file) {
      return false;
    }

    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error || !doc.is<JsonArray>()) {
      return false;
    }

    for (JsonObject item : doc.as<JsonArray>()) {
      ScoreEntry entry;
      entry.deviceId = normalizeName(String(item["deviceId"] | ""));
      entry.name = normalizeName(String(item["name"] | ""));
      entry.ms = item["ms"] | 0;
      if (entry.name.length() > 0 && entry.ms > 0) {
        leaderboard_.push_back(entry);
      }
    }

    std::sort(leaderboard_.begin(), leaderboard_.end(), [](const ScoreEntry& a, const ScoreEntry& b) {
      if (a.ms != b.ms) return a.ms < b.ms;
      return a.name < b.name;
    });

    if (leaderboard_.size() > MAX_SCORES) {
      leaderboard_.resize(MAX_SCORES);
    }

    return true;
  }

  bool loadHistory() {
    histories_.clear();
    if (!SPIFFS.exists(HISTORY_FILE)) {
      return true;
    }

    File file = SPIFFS.open(HISTORY_FILE, "r");
    if (!file) {
      return false;
    }

    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error || !doc.is<JsonArray>()) {
      return false;
    }

    for (JsonObject item : doc.as<JsonArray>()) {
      UserHistory history;
      history.deviceId = normalizeName(String(item["deviceId"] | ""));
      history.name = normalizeName(String(item["name"] | ""));
      JsonArray times = item["times"].as<JsonArray>();
      for (JsonVariant v : times) {
        history.times.push_back(v.as<uint32_t>());
      }
      if (history.name.length() > 0) {
        if (history.times.size() > WEB_HISTORY_LIMIT) {
          history.times.resize(WEB_HISTORY_LIMIT);
        }
        histories_.push_back(history);
      }
    }

    return true;
  }

  bool saveLeaderboard() const {
    DynamicJsonDocument doc(8192);
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& entry : leaderboard_) {
      JsonObject obj = arr.add<JsonObject>();
      obj["deviceId"] = entry.deviceId;
      obj["name"] = entry.name;
      obj["ms"] = entry.ms;
    }

    File file = SPIFFS.open(LEADERBOARD_FILE, "w");
    if (!file) {
      return false;
    }

    serializeJson(doc, file);
    file.close();
    return true;
  }

  bool saveHistory() const {
    DynamicJsonDocument doc(8192);
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& history : histories_) {
      JsonObject obj = arr.add<JsonObject>();
      obj["deviceId"] = history.deviceId;
      obj["name"] = history.name;
      JsonArray times = obj.createNestedArray("times");
      for (uint32_t ms : history.times) {
        times.add(ms);
      }
    }

    File file = SPIFFS.open(HISTORY_FILE, "w");
    if (!file) {
      return false;
    }

    serializeJson(doc, file);
    file.close();
    return true;
  }
};

#endif // LEADERBOARD_STORE_H