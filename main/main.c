#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_camera.h>
#include <driver/gpio.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_http_client.h>
#include "esp_netif.h"
#include "cJSON.h"
#include <stdio.h>
#include <inttypes.h>


static const char *TAG = "example:take_picture";

// Cloudinary information
const char *CLOUDINARY_UPLOAD_PRESET = "esp32_upload";  
const char *CLOUDINARY_UPLOAD_URL = "https://api.cloudinary.com/v1_1/dliospbvi/image/upload";  

// AWS S3 configuration

//for using the refvfevfdver


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

// Function to generate a unique file name using timestamp
static void generate_unique_filename(char *filename, size_t len)
{
    time_t now;
    time(&now);  // Get current system time
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // Generate a filename based on the current time
    snprintf(filename, len, "esp_image_%04d%02d%02d_%02d%02d%02d.raw",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

// Function to initialize the camera
static esp_err_t init_camera(void)
{
    ESP_LOGI(TAG, "Initializing camera...");
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed with error 0x%x", err);
        return err;
    }
    return ESP_OK;
}
#endif

// Wi-Fi SSID and password
#define WIFI_SSID "TP-Link_2A33"
#define WIFI_PASS "27552279"

// Wi-Fi event handler
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Disconnected from Wi-Fi, reconnecting...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        char ip_str[16];  // Buffer to hold IP address string
        esp_ip4addr_ntoa(&event->ip_info.ip, ip_str, sizeof(ip_str));
        ESP_LOGI(TAG, "Got IP: %s", ip_str);
    }
}

// Function to initialize Wi-Fi
void wifi_init_sta(void)
{
    ESP_LOGI(TAG, "Initializing Wi-Fi...");

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Wi-Fi initialized, connecting to SSID: %s", WIFI_SSID);
}


void print_network_info(void)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        ESP_LOGI(TAG, "IP Address: " IPSTR, IP2STR(&ip_info.ip));
        ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&ip_info.netmask));
        ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&ip_info.gw));
    }

    esp_netif_dns_info_t dns_info;
    if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info) == ESP_OK) {
        ESP_LOGI(TAG, "DNS Server: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
    }
}

esp_err_t upload_image_to_s3(uint8_t *image_data, size_t image_len, const char *presigned_url)
{
    ESP_LOGI(TAG, "Uploading image to S3...");

    // HTTP client configuration for the pre-signed URL
    esp_http_client_config_t config = {
        .url = presigned_url,  // Use the pre-signed URL here
        .method = HTTP_METHOD_PUT,
        .cert_pem = NULL,      // No SSL verification required for pre-signed URL
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Set headers and payload
    esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
    esp_http_client_set_post_field(client, (const char*)image_data, image_len);

    // Perform the PUT request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Image uploaded successfully to S3");

        // Check the HTTP status code
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status = %d", status_code);

        if (status_code == 200)
        {
            ESP_LOGI(TAG, "Upload successful");
        }
        else
        {
            ESP_LOGE(TAG, "Upload failed with status code %d", status_code);
        }
    }
    else
    {
        ESP_LOGE(TAG, "Image upload to S3 failed: %s", esp_err_to_name(err));
    }

    // Cleanup
    esp_http_client_cleanup(client);

    return err;
}




void app_main_task(void *pvParameters)
{
#if ESP_CAMERA_SUPPORTED
    if (ESP_OK != init_camera())
    {
        vTaskDelete(NULL); // If camera init fails, delete the task
        return;
    }

    ESP_LOGI(TAG, "Taking picture...");
    camera_fb_t *pic = esp_camera_fb_get();

    if (!pic)
    {
        ESP_LOGE(TAG, "Failed to capture image");
        vTaskDelete(NULL);  // Stop the task if the image capture fails
        return;
    }

    ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);

    // Generate unique filename for the image
    char unique_filename[64];
    generate_unique_filename(unique_filename, sizeof(unique_filename));

        // Replace with your pre-signed URL
    const char *presigned_url = "https://esp32-s3-images.s3.eu-north-1.amazonaws.com/esp_image_20241008_150711.raw?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAZI2LH2QM6QF4CTXW%2F20241008%2Feu-north-1%2Fs3%2Faws4_request&X-Amz-Date=20241008T140711Z&X-Amz-Expires=18000&X-Amz-Signature=0377876908da84769b3be186bf54d7c25f49fd0f370432d8c92023d415b49a12&X-Amz-SignedHeaders=host&x-id=PutObject";  // Replace with the actual pre-signed URL
    ESP_LOGI(TAG, "Generated presigned URL: %s", presigned_url);

    // Upload the picture to S3
    esp_err_t err = upload_image_to_s3(pic->buf, pic->len, presigned_url);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Image successfully uploaded with name: %s", unique_filename);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to upload image.");
    }

    esp_camera_fb_return(pic);  // Release the frame buffer

    // Stop the program after one successful upload
    ESP_LOGI(TAG, "Stopping program after successful image upload.");
    vTaskDelete(NULL);  // End the task after successful upload

#else
    ESP_LOGE(TAG, "Camera support is not available for this chip");
    vTaskDelete(NULL); // Delete the task if camera not supported
#endif
}


// Application entry point
void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Wi-Fi
    wifi_init_sta();
    
    print_network_info();

    // Create the main task
    xTaskCreatePinnedToCore(
        app_main_task,   // Task function
        "app_main_task", // Task name
        16384,           // Stack size (16KB)
        NULL,            // Task input parameter
        5,               // Task priority
        NULL,            // Task handle
        1);              // Core ID (Core 1)
}
