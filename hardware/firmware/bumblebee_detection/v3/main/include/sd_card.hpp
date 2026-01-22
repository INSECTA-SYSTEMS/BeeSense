#pragma once

#include "dl_image_define.hpp"
#include "dl_cls_postprocessor.hpp"  // for dl::cls::result_t
#include <string>

namespace sdcard {

bool init();

bool create_dir(const char *full_path);

int count_files(const char *full_path);

bool save_detected_jpeg(const dl::image::img_t &img, const dl::cls::result_t &best, const char *dir_full_path);

// Generate timestamp-based filename (YYYYMMDD_HHMMSS format)
std::string get_timestamp_filename(const char *dir_path, const char *prefix, const char *extension);

} // namespace sdcard
