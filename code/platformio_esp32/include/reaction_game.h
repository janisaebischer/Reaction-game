#ifndef REACTION_GAME_H
#define REACTION_GAME_H

#include <Arduino.h>
#include <array>

#include "i2c_gpio.h"
#include "seven_segment_display.h"

struct GameSessionContext {
  bool loggedIn = false;
  String clientId;
  String userName;
  String ownerKey;
};

class ReactionGame {
 public:
  ReactionGame(I2C_GPIO& io, SevenSegmentDisplay& display)
      : io_(io), display_(display) {}

  void setSessionContext(const GameSessionContext& sessionContext) {
    sessionContext_ = sessionContext;
  }

  void begin() {
    pinMode(GAME_POWER_SWITCH_PIN, INPUT_PULLUP);
    randomSeed(esp_random());
    Serial.println("[GAME] begin() complete, entering idle");
    resetToIdle();
  }

  void update() {
    const uint32_t now = millis();

    io_.refreshInputs();

    if (!io_.powerSwitchOn()) {
      if (state_ != State::Off) {
        Serial.println("[GAME] power switch OFF -> entering off state");
        enterOff();
      }
      return;
    }

    if (state_ == State::Off) {
      Serial.println("[GAME] power switch ON -> entering idle state");
      enterIdle();
    }

    const bool startPressed = startButtonsPressed();

    switch (state_) {
      case State::Idle:
        handleIdle(now, startPressed);
        break;
      case State::StartBlink:
        handleStartBlink(now, startPressed);
        break;
      case State::Playing:
        handlePlaying(now);
        break;
      case State::TimeoutBlink:
        handleTimeoutBlink(now);
        break;
      case State::Finished:
        handleFinished(now, startPressed);
        break;
      case State::Off:
      default:
        break;
    }
  }

  bool isPowerSwitchOn() const {
    return io_.powerSwitchOn();
  }

  bool hasRoundOwner() const {
    return roundOwnerValid_;
  }

  String roundOwnerDeviceId() const {
    return roundOwnerDeviceId_;
  }

  String roundOwnerName() const {
    return roundOwnerName_;
  }

 private:
  enum class State { Off, Idle, StartBlink, Playing, TimeoutBlink, Finished };
  static constexpr uint32_t kMaxPlayableTimeMs = 99990;
  static constexpr uint32_t kTimeoutBlinkDurationMs = 3000;

  I2C_GPIO& io_;
  SevenSegmentDisplay& display_;
  GameSessionContext sessionContext_;
  State state_ = State::Off;
  uint32_t startHoldBegin_ = 0;
  uint32_t blinkTick_ = 0;
  uint32_t displayTick_ = 0;
  uint32_t gameStartTick_ = 0;
  uint32_t timeoutStartTick_ = 0;
  uint32_t finishedElapsedMs_ = 0;
  bool blinkVisible_ = true;
  std::array<uint8_t, GAME_BUTTON_COUNT - GAME_PLAY_BUTTON_FIRST_INDEX> remainingTargets_{};
  uint8_t remainingTargetCount_ = 0;
  uint8_t currentTarget_ = UINT8_MAX;
  String frozenTime_ = "00.00";
  bool finishedResultPending_ = false;
  String roundOwnerDeviceId_;
  String roundOwnerName_;
  bool roundOwnerValid_ = false;

  bool hasSessionOwner() const {
    return sessionContext_.loggedIn &&
           sessionContext_.clientId.length() > 0 &&
           sessionContext_.userName.length() > 0;
  }

  void enterOff() {
    state_ = State::Off;
    Serial.println("[GAME] state=Off");
    startHoldBegin_ = 0;
    blinkTick_ = 0;
    displayTick_ = 0;
    gameStartTick_ = 0;
    timeoutStartTick_ = 0;
    finishedElapsedMs_ = 0;
    remainingTargetCount_ = 0;
    currentTarget_ = UINT8_MAX;
    frozenTime_ = "00.00";
    finishedResultPending_ = false;
    roundOwnerDeviceId_.remove(0);
    roundOwnerName_.remove(0);
    roundOwnerValid_ = false;
    io_.allOff();
    display_.clear();
  }

  void resetToIdle() {
    Serial.println("[GAME] resetToIdle()");
    io_.allOff();
    remainingTargetCount_ = 0;
    currentTarget_ = UINT8_MAX;
    frozenTime_ = "00.00";
    finishedResultPending_ = false;
    display_.showText("00.00");
    state_ = State::Idle;
  }

  void enterIdle() {
    Serial.println("[GAME] state=Idle");
    resetToIdle();
    startHoldBegin_ = 0;
  }

  bool startButtonsPressed() const {
    return io_.buttonPressed(GAME_START_BUTTON_LEFT_INDEX) && io_.buttonPressed(GAME_START_BUTTON_RIGHT_INDEX);
  }

  void handleIdle(uint32_t now, bool startPressed) {
    display_.showText("00.00");

    if (startPressed) {
      if (startHoldBegin_ == 0) {
        startHoldBegin_ = now;
        Serial.println("[GAME] start buttons pressed, waiting for hold");
      }

      if ((now - startHoldBegin_) >= GAME_START_HOLD_MS) {
        Serial.println("[GAME] start hold reached -> start blink");
        state_ = State::StartBlink;
        blinkVisible_ = true;
        blinkTick_ = now;
        display_.showText("00.00");
      }
    } else {
      startHoldBegin_ = 0;
    }
  }

  void handleStartBlink(uint32_t now, bool startPressed) {
    if (!startPressed) {
      Serial.println("[GAME] start buttons released -> starting game");
      startHoldBegin_ = 0;
      beginGame(now);
      return;
    }

    if ((now - blinkTick_) >= GAME_BLINK_INTERVAL_MS) {
      blinkTick_ = now;
      blinkVisible_ = !blinkVisible_;
      if (blinkVisible_) {
        display_.showText("00.00");
      } else {
        display_.clear();
      }
    }
  }

  void beginGame(uint32_t now) {
    Serial.printf("[GAME] beginGame(): round count=%u\n", static_cast<unsigned>(GAME_ROUND_LED_COUNT));
    state_ = State::Playing;
    gameStartTick_ = now;
    displayTick_ = 0;
    io_.allOff();
    if (hasSessionOwner()) {
      roundOwnerDeviceId_ = sessionContext_.clientId;
      roundOwnerName_ = sessionContext_.userName;
      roundOwnerValid_ = true;
      Serial.printf("[GAME] round owner captured: %s (%s)\n", roundOwnerName_.c_str(), roundOwnerDeviceId_.c_str());
    } else {
      roundOwnerDeviceId_.remove(0);
      roundOwnerName_.remove(0);
      roundOwnerValid_ = false;
      Serial.println("[GAME] no logged-in owner -> result will not be stored");
    }
    prepareRoundTargets();
    if (!chooseNextTarget()) {
      Serial.println("[GAME] no playable targets available -> ending round");
      finishRound(now);
      return;
    }
    showElapsed(now, true);
  }

  void prepareRoundTargets() {
    remainingTargetCount_ = 0;
    for (uint8_t button = GAME_PLAY_BUTTON_FIRST_INDEX; button < GAME_BUTTON_COUNT; ++button) {
      remainingTargets_[remainingTargetCount_++] = button;
    }

    const uint8_t targetCount = min<uint8_t>(GAME_ROUND_LED_COUNT, remainingTargetCount_);
    remainingTargetCount_ = targetCount;
    Serial.printf("[GAME] prepared %u targets from buttons %u..%u\n",
                  static_cast<unsigned>(remainingTargetCount_),
                  static_cast<unsigned>(GAME_PLAY_BUTTON_FIRST_INDEX),
                  static_cast<unsigned>(GAME_BUTTON_COUNT - 1));
  }

  bool chooseNextTarget() {
    if (remainingTargetCount_ == 0) {
      return false;
    }

    uint8_t lastTarget = currentTarget_;
    uint8_t nextTarget;

    do {
      nextTarget = random(GAME_PLAY_BUTTON_FIRST_INDEX,
                          GAME_BUTTON_COUNT);
    } while (nextTarget == lastTarget);

    currentTarget_ = nextTarget;

    io_.setLed(currentTarget_, true);

    Serial.printf("[GAME] target=%u, remaining=%u\n",
                  static_cast<unsigned>(currentTarget_ + 1),
                  static_cast<unsigned>(remainingTargetCount_));
    return true;
  }

  void finishRound(uint32_t now) {
    finishedElapsedMs_ = now - gameStartTick_;
    frozenTime_ = formatTime(finishedElapsedMs_);
    display_.showText(frozenTime_);
    finishedResultPending_ = true;
    state_ = State::Finished;
    Serial.printf("[GAME] round finished in %lu ms\n", static_cast<unsigned long>(finishedElapsedMs_));
  }

  void beginTimeoutBlink(uint32_t now) {
    timeoutStartTick_ = now;
    blinkTick_ = now;
    blinkVisible_ = true;
    finishedResultPending_ = false;
    io_.allOff();
    display_.showText("88.88");
    state_ = State::TimeoutBlink;
    Serial.println("[GAME] timeout >99.99s -> blinking 88.88 for 3s");
  }

  void handleCurrentTargetHit(uint32_t now) {
    Serial.printf("[GAME] target %u hit\n", static_cast<unsigned>(currentTarget_ + 1));
    io_.setLed(currentTarget_, false);

    remainingTargetCount_--;

    if (!chooseNextTarget()) {
      finishRound(now);
    }
  }

  void handlePlaying(uint32_t now) {
    const uint32_t elapsedMs = now - gameStartTick_;
    if (elapsedMs > kMaxPlayableTimeMs) {
      beginTimeoutBlink(now);
      return;
    }

    showElapsed(now, true);

    if (currentTarget_ != UINT8_MAX && io_.buttonPressed(currentTarget_)) {
      handleCurrentTargetHit(now);
    }
  }

  void handleTimeoutBlink(uint32_t now) {
    if ((now - timeoutStartTick_) >= kTimeoutBlinkDurationMs) {
      Serial.println("[GAME] timeout blink finished -> returning to idle");
      enterIdle();
      return;
    }

    if ((now - blinkTick_) >= GAME_BLINK_INTERVAL_MS) {
      blinkTick_ = now;
      blinkVisible_ = !blinkVisible_;
      if (blinkVisible_) {
        display_.showText("88.88");
      } else {
        display_.clear();
      }
    }
  }

  void handleFinished(uint32_t now, bool startPressed) {
    (void)now;
    display_.showText(frozenTime_);

    if (startPressed) {
      if (startHoldBegin_ == 0) {
        startHoldBegin_ = millis();
      }

      if ((millis() - startHoldBegin_) >= GAME_START_HOLD_MS) {
        state_ = State::StartBlink;
        blinkVisible_ = true;
        blinkTick_ = millis();
        display_.showText("00.00");
      }
    } else {
      startHoldBegin_ = 0;
    }
  }

  void showElapsed(uint32_t now, bool force) {
    if (!force && (now - displayTick_) < GAME_DISPLAY_REFRESH_MS) {
      return;
    }

    displayTick_ = now;
    display_.showText(formatTime(now - gameStartTick_));
  }

  String formatTime(uint32_t elapsedMs) const {
    const uint32_t seconds = elapsedMs / 1000;
    const uint32_t centis = (elapsedMs % 1000) / 10;
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%02lu.%02lu", static_cast<unsigned long>(seconds % 100), static_cast<unsigned long>(centis));
    return String(buffer);
  }

 public:
  bool consumeFinishedResult(uint32_t& elapsedMs) {
    if (!finishedResultPending_) {
      return false;
    }

    finishedResultPending_ = false;
    elapsedMs = finishedElapsedMs_;
    Serial.printf("[GAME] finished result consumed: %lu ms\n", static_cast<unsigned long>(elapsedMs));
    return true;
  }

  bool consumeFinishedResult(uint32_t& elapsedMs, String& ownerDeviceId, String& ownerName) {
    if (!consumeFinishedResult(elapsedMs)) {
      return false;
    }

    ownerDeviceId = roundOwnerValid_ ? roundOwnerDeviceId_ : String();
    ownerName = roundOwnerValid_ ? roundOwnerName_ : String();
    roundOwnerValid_ = false;
    return true;
  }
};

#endif // REACTION_GAME_H