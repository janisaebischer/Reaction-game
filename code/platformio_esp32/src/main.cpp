// Reaction Game - ESP32 game controller
#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include "i2c_gpio.h"
#include "leaderboard_store.h"
#include "seven_segment_display.h"
#include "reaction_game.h"
#include "web_app.h"

// Global hardware instances
I2C_GPIO i2c_GPIO;
SevenSegmentDisplay sevenSegDisplay;
LeaderboardStore leaderboardStore;
ReactionGame reactionGame(i2c_GPIO, sevenSegDisplay);
WebApp webApp(leaderboardStore, reactionGame);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n=== ReactionGame Starting ===");

  // Initialize I2C board for buttons and LEDs
  if (i2c_GPIO.begin()) {
    Serial.println("I2C board initialized");
  } else {
    Serial.println("I2C board not detected - continuing with limited hardware");
  }

  if (sevenSegDisplay.begin()) {
    Serial.println("Seven-segment display initialized");
    sevenSegDisplay.clear();
  } else {
    Serial.println("Seven-segment display init failed");
  }

  reactionGame.begin();

  if (!webApp.begin()) {
    Serial.println("Web app failed to start");
  }

  Serial.println("Setup complete - waiting for connections...");
}

void loop() {
  reactionGame.setSessionContext(webApp.sessionContext());
  reactionGame.update();

  uint32_t elapsedMs = 0;
  String roundOwnerDeviceId;
  String roundOwnerName;
  if (reactionGame.consumeFinishedResult(elapsedMs, roundOwnerDeviceId, roundOwnerName)) {
    webApp.handleGameResult(elapsedMs, roundOwnerDeviceId, roundOwnerName);
  }

  webApp.loop();
}
