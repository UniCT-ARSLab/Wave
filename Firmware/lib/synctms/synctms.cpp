#include "synctms.h"

#include <Arduino.h>
#include <WiFi.h>
#include <sys/time.h>
#include <esp_sntp.h>

static const char *WIFI_SSID = "A56";
static const char *WIFI_PASSWORD = "123456789";
// static const char *WIFI_SSID = "iphone";
// static const char *WIFI_PASSWORD = "123456789";



static uint64_t baseTimeUs = 0;
static uint32_t baseMicros = 0;

static bool connectWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
        return true;

    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();

    while (WiFi.status() != WL_CONNECTED) {

        delay(500);
        Serial.print(".");

        if (millis() - start > 20000) {

            Serial.println();
            Serial.println("WiFi connection timeout");

            return false;
        }
    }

    Serial.println();
    Serial.println("WiFi connected");

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    return true;
}

bool timeSync()
{
    Serial.println("Synchronizing time with NTP...");

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi is not connected");
        return false;
    }

    configTime(
        0,
        0,
        "pool.ntp.org",
        "time.nist.gov"
    );

    uint32_t start = millis();

    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {

        if (millis() - start > 15000) {

            Serial.println("NTP synchronization timeout");

            return false;
        }

        delay(100);
    }

    struct timeval tv;

    if (gettimeofday(&tv, nullptr) != 0) {

        Serial.println("Failed to read system time");

        return false;
    }

    baseTimeUs =
        (uint64_t)tv.tv_sec * 1000000ULL +
        (uint64_t)tv.tv_usec;

    baseMicros = micros();

    Serial.println("NTP synchronization successful");

    Serial.print("Network time: ");
    Serial.println(baseTimeUs);

    return true;
}

bool timeInit()
{
    Serial.println("Initializing time synchronization...");

    /*
     * Connect to WiFi first.
     */
    if (!connectWiFi()) {
        Serial.println("Time initialization failed");
        return false;
    }

    /*
     * Perform the first NTP synchronization.
     */
    if (!timeSync()) {
        Serial.println("Initial NTP synchronization failed");
        return false;
    }

    Serial.println("Time initialization complete");

    return true;
}

uint64_t gettime()
{
    return baseTimeUs +
           (uint32_t)(micros() - baseMicros);
}
