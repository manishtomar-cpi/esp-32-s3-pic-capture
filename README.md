

# ESP32-S3 Eye: Real-Time Image Capture and AWS S3 Upload

This project showcases how to use the **ESP32-S3 Eye** development board to capture images and upload them to an AWS S3 bucket. The captured images are in **RGB565** format and are uploaded via a pre-signed URL to the S3 bucket. In this README, we’ll walk through the various components such as the camera setup, Wi-Fi connection, cloud integration, and how to configure AWS S3 for future integration using AWS Signature v4.

## Table of Contents

- [Overview](#overview)
- [Hardware](#hardware)
- [Software Prerequisites](#software-prerequisites)
- [Camera Capture](#camera-capture)
- [Wi-Fi Connection](#wi-fi-connection)
- [Cloud Integration (AWS S3)](#cloud-integration-aws-s3)
- [Why Use Pre-Signed URLs?](#why-use-pre-signed-urls)
- [Future Integration: AWS Signature v4](#future-integration-aws-signature-v4)
- [Configuration (config.h)](#configuration-configh)
- [Compilation and Flashing](#compilation-and-flashing)
- [License](#license)

## Overview

The **ESP32-S3 Eye** board is a powerful tool for capturing and processing images. In this project, the ESP32-S3 Eye captures images in **RGB565 format** and uploads them directly to an **AWS S3 bucket**. The upload process uses a **pre-signed URL**, which simplifies the integration by removing the need for direct AWS credentials on the ESP32.

This project includes:
- **Camera initialization and image capture** with the **ESP32-S3 Eye**.
- **Wi-Fi connectivity** to allow the ESP32-S3 to communicate with cloud services.
- **Pre-signed URL-based image uploads** to AWS S3.

## Hardware

- **ESP32-S3 Eye** development board.
- On-board camera module configured for RGB565 format.

## Software Prerequisites

- **ESP-IDF v4.4 or later** (set up the ESP32-S3 development environment).
- **AWS S3 Bucket** (to store captured images).
- **Wi-Fi credentials** for the ESP32-S3 Eye to connect to.

## Camera Capture

The ESP32-S3 Eye comes with a built-in camera module. In this project, the camera is configured to capture images in **RGB565** format with a resolution of **QQVGA (160x120)**. The captured image data is stored in PSRAM, and the buffer is then uploaded to the cloud.

The camera pin configuration in the project is as follows:
- **XCLK**: GPIO 15
- **D0-D7**: GPIO 16-12
- **VSYNC**: GPIO 6
- **HREF**: GPIO 7
- **PCLK**: GPIO 13

```c
camera_config_t camera_config = {
    .pin_xclk       = 15,
    .pin_d7         = 11,
    .pin_d6         = 9,
    .pin_d5         = 8,
    .pin_d4         = 10,
    .pin_d3         = 12,
    .pin_d2         = 18,
    .pin_d1         = 17,
    .pin_d0         = 16,
    .pin_vsync      = 6,
    .pin_href       = 7,
    .pin_pclk       = 13,
    .pixel_format   = PIXFORMAT_RGB565,
    .frame_size     = FRAMESIZE_QQVGA,
    .fb_count       = 2,
    .fb_location    = CAMERA_FB_IN_PSRAM
};
```

## Wi-Fi Connection

The ESP32-S3 Eye connects to the cloud using Wi-Fi. The Wi-Fi credentials are hardcoded into the application, allowing the ESP32-S3 to connect to the specified access point.

```c
#define WIFI_SSID "your-ssid"
#define WIFI_PASS "your-password"
```

Upon connection, the IP address is retrieved and printed, confirming successful network connectivity. The program handles reconnections in case the Wi-Fi connection drops.

## Cloud Integration (AWS S3)

The captured images are uploaded to AWS S3 using a **pre-signed URL**. The **pre-signed URL** method allows for secure uploads without embedding AWS credentials directly in the ESP32 device, making it a safer option for constrained environments.

Once the image is captured, it is uploaded using an **HTTP PUT** request to the pre-signed URL. The unique filename for each image is generated using a timestamp, ensuring no conflicts when uploading multiple images.

```c
const char *presigned_url = "https://your-s3-bucket-presigned-url";
```

The image is uploaded as a binary payload with a **Content-Type** of `application/octet-stream`.

## Why Use Pre-Signed URLs?

Pre-signed URLs allow users to upload or download objects to/from an S3 bucket without needing AWS credentials. These URLs are time-bound and provide limited access to specific objects.

**Advantages of Pre-Signed URLs**:
- **Security**: No need to expose AWS credentials.
- **Simple Integration**: HTTP-based interaction without using AWS SDK.
- **Time-limited access**: The pre-signed URL expires after a defined period, reducing security risks.

## Future Integration: AWS Signature v4

For advanced integrations, the project can be modified to use **AWS Signature v4**, which allows direct interaction with AWS services using secure signatures. This requires signing each request using your AWS access key, secret key, and region information.

Steps to transition from pre-signed URLs to AWS Signature v4:
1. **AWS Credentials**: Store the AWS access key, secret key, and region in the `config.h` file.
2. **Request Signing**: Implement AWS Signature v4 signing algorithm to sign each request dynamically.
3. **Direct Uploads**: Once signed, the ESP32-S3 can upload images directly to AWS S3 using HTTP PUT requests.

### Sample config.h for AWS Credentials

Your AWS credentials should be stored in a `config.h` file like the example below. This file is used to configure the project for direct S3 integration in future implementations.

```c
#ifndef CONFIG_H
#define CONFIG_H

#define MY_AWS_ACCESS_KEY ""
#define MY_AWS_SECRET_KEY ""
#define MY_AWS_REGION ""
#define MY_AWS_BUCKET_NAME ""

#endif // CONFIG_H
```

By using these credentials, you can implement AWS Signature v4 to upload directly to S3 without relying on pre-signed URLs.

## Compilation and Flashing

1. Clone this repository and navigate to the project folder.
2. Set up your development environment with ESP-IDF (v4.4 or later).
3. Compile the code:
   ```bash
   idf.py build
   ```
4. Flash the compiled binary to the ESP32-S3 Eye board:
   ```bash
   idf.py flash monitor
   ```
5. Ensure the ESP32-S3 Eye is connected via USB and the correct serial port is selected.

After successful flashing, the ESP32-S3 will connect to the Wi-Fi network, capture images, and upload them to AWS S3 using the pre-signed URL provided in the source code.
