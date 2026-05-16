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

// Maximum length for player names (not enforced in UI here, used for sizing)
static const size_t MAX_NAME_LENGTH = 30;

// OTA
static const char OTA_HOSTNAME[] = "ReactionGame";

#endif // SETTINGS_H
