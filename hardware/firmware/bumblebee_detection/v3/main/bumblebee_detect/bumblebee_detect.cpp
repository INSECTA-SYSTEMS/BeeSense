#include "bumblebee_detect.hpp"
#include "esp_log.h"
#include <filesystem>

#if CONFIG_BUMBLEBEE_DETECT_MODEL_IN_FLASH_RODATA
extern const uint8_t bumblebee_detect_espdl[] asm("_binary_bumblebee_detect_espdl_start");
static const char *path = (const char *)bumblebee_detect_espdl;
#elif CONFIG_BUMBLEBEE_DETECT_MODEL_IN_FLASH_PARTITION
static const char *path = "bumblebee_det";
#else
#if !defined(CONFIG_BSP_SD_MOUNT_POINT)
#define CONFIG_BSP_SD_MOUNT_POINT "/sdcard"
#endif
#endif
namespace bumblebee_detect {
ESPDet::ESPDet(const char *model_name, float score_thr, float nms_thr)
{
#if !CONFIG_BUMBLEBEE_DETECT_MODEL_IN_SDCARD
    m_model =
        new dl::Model(path, model_name, static_cast<fbs::model_location_type_t>(CONFIG_BUMBLEBEE_DETECT_MODEL_LOCATION));
#else
    auto sd_path = std::filesystem::path(CONFIG_BSP_SD_MOUNT_POINT) / CONFIG_BUMBLEBEE_DETECT_MODEL_SDCARD_DIR / model_name;
    m_model = new dl::Model(sd_path.c_str(), fbs::MODEL_LOCATION_IN_SDCARD);
#endif
    m_model->minimize();
#if CONFIG_IDF_TARGET_ESP32P4
    m_image_preprocessor = new dl::image::ImagePreprocessor(m_model, {0, 0, 0}, {255, 255, 255});
#else
    m_image_preprocessor = new dl::image::ImagePreprocessor(
        m_model, {0, 0, 0}, {255, 255, 255}, dl::image::DL_IMAGE_CAP_RGB565_BIG_ENDIAN);
#endif
    m_image_preprocessor->enable_letterbox({114, 114, 114});
    m_postprocessor = new dl::detect::ESPDetPostProcessor(
        m_model, m_image_preprocessor, score_thr, nms_thr, 10, {{8, 8, 4, 4}, {16, 16, 8, 8}, {32, 32, 16, 16}});
}

} // namespace bumblebee_detect


BumblebeeDetect::BumblebeeDetect(bool lazy_load)
{
    m_score_thr[0] = bumblebee_detect::ESPDet::default_score_thr;
    m_nms_thr[0] = bumblebee_detect::ESPDet::default_nms_thr;
    if (lazy_load) {
        m_model = nullptr;
    } else {
        load_model();
    }
}

void BumblebeeDetect::load_model()
{
    #if CONFIG_FLASH_ESPDET_PICO_224_224_BUMBLEBEE || CONFIG_BUMBLEBEE_DETECT_MODEL_IN_SDCARD
        m_model = new bumblebee_detect::ESPDet("espdet_pico_224_224_bumblebee.espdl", m_score_thr[0], m_nms_thr[0]);
    #else
        ESP_LOGE("bumblebee_detect", "espdet_pico_224_224_bumblebee is not selected in menuconfig.");
    #endif
}
