
#include "esp_imgfx_crop.h"
#include "dl_image.hpp"
#define MODEL_IMG_SIZE 224
#include <stdio.h>
#include <algorithm>
#include "bumblebee_detect.hpp"
#include "esp_camera.h"
#include "esp_log.h"
#include "sd_card.hpp"
#include <esp_system.h>
#include <string.h>
#include <vector>
#include "bsp/esp-bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "include/camera_pins.h"
#include "dl_image_draw.hpp"
#include "dl_image_color.hpp"

// WiFi Support
#include "include/wifi_manager.h"

// Time support for timestamp
#include "esp_sntp.h"
#include <time.h>

// Intervall für das Senden der Daten ans Dashboard (in Millisekunden)
#define WIFI_SEND_INTERVAL_MS 10000

extern const uint8_t bumblebee_jpg_start[] asm("_binary_bumblebee_jpg_start");
extern const uint8_t bumblebee_jpg_end[] asm("_binary_bumblebee_jpg_end");
const char *TAG = "bumblebee_detect";

// Get current time from NTP and set timezone
static void obtain_time(void)
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < 20) {
        vTaskDelay(pdMS_TO_TICKS(500));
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    // Set timezone for Germany (CET/CEST)
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
    
    ESP_LOGI(TAG, "Time synchronized. Current epoch=%lld", (long long)now);
}

// Periodically re-synchronize time with NTP
static void time_sync_task(void *pvParameters) {
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(1200000)); // Re-sync every 20 minutes
        obtain_time();
        ESP_LOGI(TAG, "Time re-synchronized");
    }
}

// Camera Module pin mapping
static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,

    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,

    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,

    .xclk_freq_hz = 20000000, // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565, // PIXFORMAT_RGB565 , PIXFORMAT_JPEG
    .frame_size = FRAMESIZE_QVGA, // [<<320x240>> (QVGA, 4:3); FRAMESIZE_320X320, 240x176 (HQVGA, 15:11); 400x296 (CIF,
                                  // 50:37)],FRAMESIZE_QVGA,FRAMESIZE_VGA

    .jpeg_quality = 8, // 0-63 lower number means higher quality.  Reduce quality if stack overflow in cam_task
    .fb_count = 2,     // if more than one, i2s runs in continuous mode. Use only with JPEG
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    .sccb_i2c_port = 0 // optional
};

static esp_err_t init_camera(void)
{
    // Initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE("CAM", "Camera Init Failed");
    }
    return err;
}

// Hilfsfunktion: Bild aufnehmen, croppen und in RGB888 konvertieren
static bool capture_and_convert_image(dl::image::img_t &cropped_img) {
    camera_fb_t *pic = esp_camera_fb_get();
    if (!pic) {
        ESP_LOGE("CAM", "Failed to capture image");
        return false;
    }
    dl::image::img_t img;
    img.height = pic->height;
    img.width = pic->width;
    img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565;
    img.data = malloc(pic->len);
    if (!img.data) {
        ESP_LOGE("MEM", "Memory allocation failed");
        esp_camera_fb_return(pic);
        return false;
    }

    memcpy(img.data, pic->buf, pic->len);
    esp_camera_fb_return(pic);

    // RGB565 -> RGB888 (ohne Crop)
    dl::image::img_t rgb888_img;
    rgb888_img.height = img.height;
    rgb888_img.width = img.width;
    rgb888_img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888;
    rgb888_img.data = malloc(img.width * img.height * 3);
    if (!rgb888_img.data) {
        ESP_LOGE("MEM", "Failed to allocate rgb888 buffer");
        free(img.data);
        return false;
    }
    dl::image::RGB5652RGB888<true, false> converter;
    uint8_t* src = (uint8_t*)img.data;
    uint8_t* dst = (uint8_t*)rgb888_img.data;
    for (int i = 0; i < img.width * img.height; ++i) {
        converter(src, dst);
        src += 2;
        dst += 3;
    }

    // Crop auf 224x224
    cropped_img.height = MODEL_IMG_SIZE;
    cropped_img.width = MODEL_IMG_SIZE;
    cropped_img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888;
    cropped_img.data = malloc(MODEL_IMG_SIZE * MODEL_IMG_SIZE * 3);
    if (!cropped_img.data) {
        ESP_LOGE("MEM", "Failed to allocate cropped buffer");
        free(img.data);
        free(rgb888_img.data);
        return false;
    }
    int x0 = (rgb888_img.width - MODEL_IMG_SIZE) / 2;
    int y0 = (rgb888_img.height - MODEL_IMG_SIZE) / 2;
    for (int y = 0; y < MODEL_IMG_SIZE; ++y) {
        int src_y = y0 + y;
        memcpy(
            (uint8_t*)cropped_img.data + y * MODEL_IMG_SIZE * 3,
            (uint8_t*)rgb888_img.data + (src_y * rgb888_img.width + x0) * 3,
            MODEL_IMG_SIZE * 3);
    }

    free(img.data);
    free(rgb888_img.data);
    return true;
}

extern "C" void app_main(void)
{
    // WiFi initialisieren und verbinden
    ESP_LOGI(TAG, "Initialisiere WiFi...");
    esp_err_t wifi_err = wifi_init_sta();
    if (wifi_err == ESP_OK) {
        // Warte max. 10 Sekunden auf Verbindung
        wifi_err = wifi_wait_connected(10000);
        if (wifi_err != ESP_OK) {
            ESP_LOGW(TAG, "WiFi-Verbindung fehlgeschlagen, fahre ohne WiFi fort");
        }
    } else {
        ESP_LOGW(TAG, "WiFi-Initialisierung fehlgeschlagen: %s", esp_err_to_name(wifi_err));
    }

    // Initialize time from NTP
    ESP_LOGI(TAG, "Synchronizing time with NTP...");
    obtain_time();

    ESP_LOGI("SD", "Mounting SD card...");
    bool mounted = sdcard::init();
    if (!mounted) {
        ESP_LOGE("SD", "SD card init/mount failed");
        return;
    }

    if (ESP_OK != init_camera()) {
        ESP_LOGE("APP", "Camera initialization failed");
        return;
    }

    #if CONFIG_BUMBLEBEE_DETECT_MODEL_IN_SDCARD
        ESP_ERROR_CHECK(bsp_sdcard_mount());
    #endif

    // Zählvariablen für Ein- und Ausflüge
    int einflug_count = 0;
    int ausflug_count = 0;
    const int y_line = 120; // Zähllinie
    // Für einfaches Tracking: Merke dir die y-Positionen der Hummeln aus dem letzten Frame
    std::vector<int> last_centers_y;
    
    // Timer für WiFi-Datenübertragung
    uint32_t last_wifi_send_time = 0;
    int last_sent_einflug = 0;
    int last_sent_ausflug = 0;

    while (true) {
        ESP_LOGI("MEM", "Free heap at start of loop: %lu bytes", esp_get_free_heap_size());

        dl::image::img_t cropped_img;
        if (!capture_and_convert_image(cropped_img)) {
            ESP_LOGE("CAM", "Could not take or convert picture");
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        BumblebeeDetect *detect = new BumblebeeDetect();
        auto &detect_results = detect->run(cropped_img);
        int result_count = 0;
        std::vector<int> current_centers_y;

        // Zähllinie zeichnen (grün)
        std::vector<uint8_t> line_color = {0, 255, 0};
        dl::image::draw_hollow_rectangle(cropped_img, 0, y_line, MODEL_IMG_SIZE-1, y_line+1, line_color, 2);

        for (const auto &res : detect_results) {
            if (res.category == 0 && res.score > 0.35f) {
                int x1 = res.box[0];
                int y1 = res.box[1];
                int x2 = res.box[2];
                int y2 = res.box[3];
                if (x2 < x1) std::swap(x1, x2);
                if (y2 < y1) std::swap(y1, y2);
                std::vector<uint8_t> color = {255, 0, 0}; // Rot
                dl::image::draw_hollow_rectangle(cropped_img, x1, y1, x2, y2, color, 2);
                
                // Confidence Score anzeigen
                ESP_LOGI(TAG, "Hummel erkannt mit Confidence: %.2f (Pos: x=%d, y=%d)", res.score, (x1+x2)/2, (y1+y2)/2);
                ++result_count;

                // Mittelpunkt der Box berechnen
                int center_x = (x1 + x2) / 2;
                int center_y = (y1 + y2) / 2;
                current_centers_y.push_back(center_y);

                // Mittelpunkt im Serial Monitor ausgeben
                ESP_LOGI(TAG, "Hummel-Mittelpunkt: x=%d, y=%d", center_x, center_y);

                // Auswertung: Liegt Mittelpunkt über oder unter der Linie?
                ESP_LOGI(TAG, "Hummel-Mitte y=%d, Linie y=%d", center_y, y_line);
            }
        }

        // Einfache Zähl-Logik: Wenn eine Hummel von unterhalb nach oberhalb der Linie wechselt = Einflug
        // Wenn von oberhalb nach unterhalb = Ausflug
        // (Vereinfachte Annahme: Nur Positionswechsel zwischen zwei Frames)
        for (size_t i = 0; i < current_centers_y.size(); ++i) {
            int curr_y = current_centers_y[i];
            // Suche im letzten Frame nach einem ähnlichen Mittelpunkt
            for (size_t j = 0; j < last_centers_y.size(); ++j) {
                int last_y = last_centers_y[j];
                if (abs(curr_y - last_y) < 30) { // Annahme: gleiche Hummel
                    // Von oben nach unten: Ausflug
                    if (last_y < y_line && curr_y >= y_line) {
                        ausflug_count++;
                        ESP_LOGI(TAG, "Ausflug erkannt!");
                    } 
                    // Von unten nach oben: Einflug
                    else if (last_y > y_line && curr_y <= y_line) {
                        einflug_count++;
                        ESP_LOGI(TAG, "Einflug erkannt!");
                    }
                }
            }
        }
        last_centers_y = current_centers_y;

        ESP_LOGI(TAG, "Einflüge: %d, Ausflüge: %d", einflug_count, ausflug_count);

        // Daten über WiFi senden (falls verbunden und Änderungen vorhanden)
        uint32_t current_time = esp_log_timestamp();
        if (wifi_is_connected()) {
            // Sende Daten wenn sich etwas geändert hat oder das Intervall erreicht ist
            bool data_changed = (einflug_count != last_sent_einflug) || (ausflug_count != last_sent_ausflug);
            bool interval_reached = (current_time - last_wifi_send_time) >= WIFI_SEND_INTERVAL_MS;
            
            if (data_changed || interval_reached) {
                esp_err_t send_err = wifi_send_tracking_data(einflug_count, ausflug_count);
                if (send_err == ESP_OK) {
                    last_wifi_send_time = current_time;
                    last_sent_einflug = einflug_count;
                    last_sent_ausflug = ausflug_count;
                }
            }
        }

        // Bild mit BBoxen und Linie speichern (optional)
        dl::cls::result_t dummy_result = {};
        sdcard::save_detected_jpeg(cropped_img, dummy_result, "/sdcard/bumblebee_tracking");

        delete detect;
        heap_caps_free(cropped_img.data);

        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms Intervall
    }

    #if CONFIG_BUMBLEBEE_DETECT_MODEL_IN_SDCARD
        ESP_ERROR_CHECK(bsp_sdcard_unmount());
    #endif

    // Start time sync task to periodically update time from NTP
    xTaskCreate(time_sync_task, "time_sync", 4096, NULL, 5, NULL);
}
