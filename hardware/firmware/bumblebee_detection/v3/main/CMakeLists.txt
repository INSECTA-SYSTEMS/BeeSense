set(src_dirs        ./
                    ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(include_dirs    ./
                    ${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(requires        bumblebee_detect
                    esp_wifi
                    esp_event
                    esp_netif
                    nvs_flash
                    esp_http_client)

if (IDF_TARGET STREQUAL "esp32s3")
    list(APPEND requires esp32_s3_eye_noglib
                         esp_lcd)
elseif (IDF_TARGET STREQUAL "esp32p4")
    list(APPEND requires esp32_p4_function_ev_board_noglib
                         esp_lcd)
endif()

set(embed_files     "bumblebee.jpg")

idf_component_register(
    SRC_DIRS ${src_dirs}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES ${requires}
    PRIV_REQUIRES fatfs
    EMBED_FILES ${embed_files}
)
