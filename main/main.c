#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_camera.h"
#include "driver/gpio.h"

static const char *TAG = "example:take_picture";

#if ESP_CAMERA_SUPPORTED
camera_config_t camera_config = {
    .pin_pwdn       = -1,    // PWDN not connected
    .pin_reset      = -1,    // RESET not connected
    .pin_xclk       = 15,    // XCLK pin
    .pin_sccb_sda   = 4,     // SIOD (I2C Data) - GPIO4
    .pin_sccb_scl   = 5,     // SIOC (I2C Clock) - GPIO5
    .pin_d7         = 11,    // GPIO11
    .pin_d6         = 9,     // GPIO9
    .pin_d5         = 8,     // GPIO8
    .pin_d4         = 10,    // GPIO10
    .pin_d3         = 12,    // GPIO12
    .pin_d2         = 18,    // GPIO18
    .pin_d1         = 17,    // GPIO17
    .pin_d0         = 16,    // GPIO16
    .pin_vsync      = 6,     // VSYNC
    .pin_href       = 7,     // HREF
    .pin_pclk       = 13,    // PCLK

    .xclk_freq_hz   = 10000000, // XCLK frequency set to 10MHz
    .ledc_timer     = LEDC_TIMER_0,
    .ledc_channel   = LEDC_CHANNEL_0,
    .pixel_format   = PIXFORMAT_RGB565, // Testing with RGB565 format
    .frame_size     = FRAMESIZE_QQVGA, // Set to QQVGA (160x120)
    .jpeg_quality   = 12,    // JPEG quality
    .fb_count       = 2,     // Use double buffering
    .fb_location    = CAMERA_FB_IN_PSRAM // Store frame buffers in PSRAM
};

static esp_err_t init_camera(void)
{
    ESP_LOGI(TAG, "Initializing camera...");
    // Log all pin configurations
    ESP_LOGI(TAG, "Camera pins:");
    ESP_LOGI(TAG, "PWDN: %d", camera_config.pin_pwdn);
    ESP_LOGI(TAG, "RESET: %d", camera_config.pin_reset);
    ESP_LOGI(TAG, "XCLK: %d", camera_config.pin_xclk);
    ESP_LOGI(TAG, "SIOD: %d", camera_config.pin_sccb_sda);
    ESP_LOGI(TAG, "SIOC: %d", camera_config.pin_sccb_scl);
    ESP_LOGI(TAG, "D7: %d", camera_config.pin_d7);
    ESP_LOGI(TAG, "D6: %d", camera_config.pin_d6);
    ESP_LOGI(TAG, "D5: %d", camera_config.pin_d5);
    ESP_LOGI(TAG, "D4: %d", camera_config.pin_d4);
    ESP_LOGI(TAG, "D3: %d", camera_config.pin_d3);
    ESP_LOGI(TAG, "D2: %d", camera_config.pin_d2);
    ESP_LOGI(TAG, "D1: %d", camera_config.pin_d1);
    ESP_LOGI(TAG, "D0: %d", camera_config.pin_d0);
    ESP_LOGI(TAG, "VSYNC: %d", camera_config.pin_vsync);
    ESP_LOGI(TAG, "HREF: %d", camera_config.pin_href);
    ESP_LOGI(TAG, "PCLK: %d", camera_config.pin_pclk);

    // Initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed with error 0x%x", err);
        return err;
    }

    return ESP_OK;
}
#endif

void app_main_task(void *pvParameters)
{
#if ESP_CAMERA_SUPPORTED
    if (ESP_OK != init_camera()) {
        vTaskDelete(NULL);  // If camera init fails, delete the task
        return;
    }

    while (1)
    {
        ESP_LOGI(TAG, "Taking picture...");
        camera_fb_t *pic = esp_camera_fb_get();

        if (!pic) {
            ESP_LOGE(TAG, "Failed to capture image");
            vTaskDelay(pdMS_TO_TICKS(5000));  // Wait for 5 seconds before retrying
            continue;  // Retry after delay
        }

        ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);
        esp_camera_fb_return(pic);

        vTaskDelay(pdMS_TO_TICKS(5000));  // Wait for 5 seconds before taking the next picture
    }
#else
    ESP_LOGE(TAG, "Camera support is not available for this chip");
    vTaskDelete(NULL);  // Delete the task if camera not supported
    return;
#endif
}

void app_main(void)
{
    xTaskCreatePinnedToCore(
    app_main_task,   // Task function
    "app_main_task", // Task name
    16384,           // Stack size (16KB)
    NULL,            // Task input parameter
    5,               // Task priority
    NULL,            // Task handle
    1);              // Core ID (Core 1)
}
