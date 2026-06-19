#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

// Centralized configuration/settings for the project.
// Put all configurable values here (network, files, OTA, ...)

// Access Point
static const char AP_SSID[] = "ReactionGame";
static const char AP_PASSWORD[] = "reaction-game"; // empty = open network

// DNS
static const int DNS_PORT = 53;

// Files
static const char SCORES_FILE[] = "/scores.json";
static const size_t MAX_SCORES = 100;
static const char LEADERBOARD_FILE[] = "/leaderboard.json";
static const char HISTORY_FILE[] = "/history.json";

// Maximum length for player names (not enforced in UI here, used for sizing)
static const size_t MAX_NAME_LENGTH = 30;

// I2C GPIO expander (MCP23017-based DIN rail module)
static const uint8_t I2C_SDA_PIN = 21;
static const uint8_t I2C_SCL_PIN = 22;
static const uint32_t I2C_CLOCK_HZ = 400000;
static const uint8_t I2C_MCP23017_BASE_ADDR = 0x20;
static const uint8_t I2C_MCP23017_DEVICE_COUNT = 2;

// Seven-segment display driver (TPIC6C596 shift-register 4x)
static const uint8_t SEVENSEG_DATA_PIN_1 = 4;
static const uint8_t SEVENSEG_CLOCK_PIN_1 = 5;
static const uint8_t SEVENSEG_LATCH_PIN_1 = 13;
static const uint8_t SEVENSEG_DATA_PIN_2 = 14;
static const uint8_t SEVENSEG_CLOCK_PIN_2 = 16;
static const uint8_t SEVENSEG_LATCH_PIN_2 = 17;
static const uint8_t SEVENSEG_DATA_PIN_3 = 18;
static const uint8_t SEVENSEG_CLOCK_PIN_3 = 19;
static const uint8_t SEVENSEG_LATCH_PIN_3 = 23;
static const uint8_t SEVENSEG_DATA_PIN_4 = 25;
static const uint8_t SEVENSEG_CLOCK_PIN_4 = 26;
static const uint8_t SEVENSEG_LATCH_PIN_4 = 27;
static const uint8_t SEVENSEG_DIGIT_COUNT = 4;
static const bool SEVENSEG_ACTIVE_LOW = false;

// Reaction game controls
static const uint8_t GAME_POWER_SWITCH_PIN = 32;
static const bool GAME_CONTROL_ACTIVE_LOW = true;
static const uint8_t GAME_BUTTON_COUNT = 15;
static const uint8_t GAME_BUTTON_ACTIVE_LOW = true;
static const uint8_t GAME_LED_COUNT = GAME_BUTTON_COUNT;
static const bool GAME_LED_ACTIVE_LOW = true;
static const uint8_t GAME_ROUND_LED_COUNT = 5;
static const uint8_t GAME_START_BUTTON_LEFT_INDEX = 0;
static const uint8_t GAME_START_BUTTON_RIGHT_INDEX = 1;
static const uint8_t GAME_PLAY_BUTTON_FIRST_INDEX = 0;
static const uint8_t GAME_TARGET_COUNT = GAME_ROUND_LED_COUNT;
static const uint32_t GAME_START_HOLD_MS = 3000;
static const uint32_t GAME_BLINK_INTERVAL_MS = 300;
static const uint32_t GAME_DISPLAY_REFRESH_MS = 50;
static const uint32_t GAME_INPUT_DEBOUNCE_MS = 25;

// Web UI / login
static const uint8_t AP_MAX_CLIENTS = 1;
static const uint32_t WEB_SESSION_TIMEOUT_MS = 20000;
static const uint8_t WEB_HISTORY_LIMIT = 10;
static const char WEB_MDNS_HOSTNAME[] = "reaction-game";

// OTA
static const char OTA_HOSTNAME[] = "ReactionGame";

#endif // SETTINGS_H
