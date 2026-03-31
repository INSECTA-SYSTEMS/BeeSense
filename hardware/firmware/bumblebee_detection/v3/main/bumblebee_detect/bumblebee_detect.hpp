#pragma once
#include "dl_detect_base.hpp"
#include "dl_detect_espdet_postprocessor.hpp"

namespace bumblebee_detect {
class ESPDet : public dl::detect::DetectImpl {
public:
    static inline constexpr float default_score_thr = 0.4;
    static inline constexpr float default_nms_thr = 0.45;
    ESPDet(const char *model_name, float score_thr, float nms_thr);
};
} // namespace bumblebee_detect

class BumblebeeDetect : public dl::detect::DetectWrapper {
public:
    BumblebeeDetect(bool lazy_load = true);

private:
    void load_model() override;
};
