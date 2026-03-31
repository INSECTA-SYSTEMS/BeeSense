#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

// WiFi Konfiguration - Kann über Menuconfig angepasst werden
#ifndef CONFIG_WIFI_SSID
#define CONFIG_WIFI_SSID "BeeSense_AP"
#endif

#ifndef CONFIG_WIFI_PASSWORD
#define CONFIG_WIFI_PASSWORD "beesense123"
#endif

#ifndef CONFIG_WIFI_MAXIMUM_RETRY
#define CONFIG_WIFI_MAXIMUM_RETRY 5
#endif

// Dashboard Server Konfiguration
#ifndef CONFIG_DASHBOARD_HOST
#define CONFIG_DASHBOARD_HOST "192.168.1.100"
#endif

#ifndef CONFIG_DASHBOARD_PORT
#define CONFIG_DASHBOARD_PORT 8080
#endif

/**
 * @brief Initialisiert die WiFi-Verbindung im Station-Modus
 * 
 * Diese Funktion initialisiert NVS, das Event-Loop-System, 
 * den TCP/IP Stack und konfiguriert WiFi im Station-Modus.
 * 
 * @return ESP_OK bei Erfolg, sonst Fehlercode
 */
esp_err_t wifi_init_sta(void);

/**
 * @brief Prüft ob WiFi verbunden ist
 * 
 * @return true wenn mit AP verbunden, false sonst
 */
bool wifi_is_connected(void);

/**
 * @brief Wartet bis WiFi verbunden ist oder Timeout erreicht
 * 
 * @param timeout_ms Maximale Wartezeit in Millisekunden
 * @return ESP_OK wenn verbunden, ESP_ERR_TIMEOUT bei Timeout
 */
esp_err_t wifi_wait_connected(uint32_t timeout_ms);

/**
 * @brief Sendet Trackingdaten ans Dashboard
 * 
 * @param einflug_count Anzahl der Einflüge
 * @param ausflug_count Anzahl der Ausflüge
 * @return ESP_OK bei Erfolg, sonst Fehlercode
 */
esp_err_t wifi_send_tracking_data(int einflug_count, int ausflug_count);

/**
 * @brief Stoppt und deinitialisiert WiFi
 */
void wifi_stop(void);

#ifdef __cplusplus
}
#endif
