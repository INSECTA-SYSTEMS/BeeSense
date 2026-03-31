#include "wifi_manager.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"

static const char *TAG = "WIFI_MANAGER";

// FreeRTOS Event Group für WiFi-Events
static EventGroupHandle_t s_wifi_event_group;

// Event Bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Retry Counter
static int s_retry_num = 0;
static bool s_wifi_initialized = false;

// Event Handler für WiFi und IP Events
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi Station gestartet, verbinde...");
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "Verbindung getrennt, Grund: %d", event->reason);
        
        if (s_retry_num < CONFIG_WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Erneuter Verbindungsversuch %d/%d", s_retry_num, CONFIG_WIFI_MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Verbindung fehlgeschlagen nach %d Versuchen", CONFIG_WIFI_MAXIMUM_RETRY);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "IP-Adresse erhalten: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init_sta(void)
{
    if (s_wifi_initialized) {
        ESP_LOGW(TAG, "WiFi bereits initialisiert");
        return ESP_OK;
    }

    // NVS initialisieren (benötigt für WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS Partition wird gelöscht und neu initialisiert");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Event Group erstellen
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Event Group konnte nicht erstellt werden");
        return ESP_ERR_NO_MEM;
    }

    // TCP/IP Stack initialisieren
    ESP_ERROR_CHECK(esp_netif_init());

    // Default Event Loop erstellen
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Default WiFi Station Interface erstellen
    esp_netif_create_default_wifi_sta();

    // WiFi mit Default-Konfiguration initialisieren
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Event Handler registrieren
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // WiFi-Konfiguration setzen
    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, CONFIG_WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, CONFIG_WIFI_PASSWORD, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_wifi_initialized = true;
    ESP_LOGI(TAG, "WiFi Station-Modus initialisiert, SSID: %s", CONFIG_WIFI_SSID);

    return ESP_OK;
}

bool wifi_is_connected(void)
{
    if (!s_wifi_initialized || s_wifi_event_group == NULL) {
        return false;
    }
    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    return (bits & WIFI_CONNECTED_BIT) != 0;
}

esp_err_t wifi_wait_connected(uint32_t timeout_ms)
{
    if (!s_wifi_initialized || s_wifi_event_group == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Warte auf WiFi-Verbindung...");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(timeout_ms));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Erfolgreich mit AP verbunden");
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Verbindung zum AP fehlgeschlagen");
        return ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "Timeout beim Warten auf Verbindung");
        return ESP_ERR_TIMEOUT;
    }
}

// HTTP Event Handler für Debugging
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER: %s: %s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t wifi_send_tracking_data(int einflug_count, int ausflug_count)
{
    if (!wifi_is_connected()) {
        ESP_LOGW(TAG, "Nicht verbunden, kann Daten nicht senden");
        return ESP_ERR_WIFI_NOT_CONNECT;
    }

    // JSON-Payload erstellen
    char json_data[128];
    snprintf(json_data, sizeof(json_data), 
             "{\"einflug\":%d,\"ausflug\":%d,\"timestamp\":%lld}",
             einflug_count, ausflug_count, (long long)esp_log_timestamp());

    // URL für Dashboard API
    char url[128];
    snprintf(url, sizeof(url), "http://%s:%d/api/tracking", 
             CONFIG_DASHBOARD_HOST, CONFIG_DASHBOARD_PORT);

    ESP_LOGI(TAG, "Sende Daten an %s: %s", url, json_data);

    // HTTP Client konfigurieren
    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_POST;
    config.event_handler = http_event_handler;
    config.timeout_ms = 5000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "HTTP Client konnte nicht initialisiert werden");
        return ESP_ERR_NO_MEM;
    }

    // Headers und Body setzen
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    // Request senden
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d", status_code, content_length);
        
        if (status_code >= 200 && status_code < 300) {
            ESP_LOGI(TAG, "Daten erfolgreich gesendet");
        } else {
            ESP_LOGW(TAG, "Server antwortete mit Status %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST fehlgeschlagen: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

void wifi_stop(void)
{
    if (!s_wifi_initialized) {
        return;
    }

    ESP_LOGI(TAG, "WiFi wird gestoppt");
    esp_wifi_stop();
    esp_wifi_deinit();
    
    if (s_wifi_event_group != NULL) {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }
    
    s_wifi_initialized = false;
}
