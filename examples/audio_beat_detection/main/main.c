/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file main.c
 * @brief Beat Detection with I2S Audio Input Example
 * 
 * 本示例展示了如何：
 * 1. 初始化 I2S 接口读取音频数据
 * 2. 初始化 Beat Detection 组件
 * 3. 将 I2S 读取的音频数据输入到 Beat Detection
 * 4. 在检测到鼓点时打印 "audio detected!"
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "beat_detection.h"
#include "beat_detection_config.h"

static const char *TAG = "BEAT_DETECTION_I2S_EXAMPLE";

#define I2S_SAMPLE_RATE         16000
#define I2S_CHANNELS            1
#define I2S_BUFFER_SIZE         2048

#define FFT_SIZE                512

static beat_detection_handle_t g_beat_detection_handle = NULL;

/**
 * @brief Beat Detection 结果回调函数
 * 
 * 当检测到鼓点时会调用此函数
 * 
 * @param result 检测结果
 * @param ctx 用户上下文指针
 */
static void beat_detection_result_callback(beat_detection_result_t result, void *ctx)
{
    if (result == BEAT_DETECTED) {
        ESP_LOGI(TAG, "audio detected!");
    } else if (result == BEAT_NOT_DETECTED) {
        ESP_LOGI(TAG, "audio not detected!");
    } else if (result == BEAT_DETECTION_FAILED) {
        ESP_LOGI(TAG, "beat detection failed!");
    }
}

/**
 * @brief 初始化 Beat Detection 组件
 * 
 * @return ESP_OK 成功，其他值表示失败
 */
static esp_err_t beat_detection_init_example(void)
{
    esp_err_t ret = ESP_OK;

    // 配置 Beat Detection
    beat_detection_cfg_t cfg = BEAT_DETECTION_DEFAULT_CFG();
    cfg.audio_cfg.channel = I2S_CHANNELS;
    cfg.result_callback = beat_detection_result_callback;
    cfg.result_callback_ctx = NULL;
    cfg.flags.enable_psram = false;

    // 初始化 Beat Detection
    ret = beat_detection_init(&cfg, &g_beat_detection_handle);
    if (ret != ESP_OK || g_beat_detection_handle == NULL) {
        ESP_LOGE(TAG, "Failed to initialize beat detection: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Beat Detection initialized successfully");

    return ESP_OK;
}

/**
 * @brief 主函数
 */
void app_main(void)
{
    esp_err_t ret = ESP_OK;

    // 初始化 Beat Detection
    ret = beat_detection_init_example();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize beat detection");
        return;
    }

    // 分配足够的缓冲区：I2S_BUFFER_SIZE * I2S_CHANNELS 个 int16_t 样本
    int16_t *audio_buffer = (int16_t *)heap_caps_malloc(I2S_BUFFER_SIZE * sizeof(int16_t) * I2S_CHANNELS, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (audio_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for audio buffer");
        return;
    }

    ESP_LOGI(TAG, "Audio buffer allocated: %d bytes (%d samples)", I2S_BUFFER_SIZE * I2S_CHANNELS * sizeof(int16_t), I2S_BUFFER_SIZE);

    int count = 0;
    while (count++ < 20) {
        // 生成一个250Hz的正弦波（在低音频率范围内，适合beat detection）
        for (size_t i = 0; i < I2S_BUFFER_SIZE; i++) {
            float sample = 10000 * sinf(2.0f * M_PI * 250.0f * i / I2S_SAMPLE_RATE);
            // 转换为 int16_t 格式（范围 -32768 到 32767）
            audio_buffer[i] = (int16_t)(sample);
        }
        
        beat_detection_audio_buffer_t buffer = {
            .audio_buffer = (uint8_t *)audio_buffer,
            .bytes_size = I2S_BUFFER_SIZE * I2S_CHANNELS * sizeof(int16_t),
        };
        
        beat_detection_data_write(g_beat_detection_handle, buffer);
        vTaskDelay(pdMS_TO_TICKS(500));

        // 发送静音数据
        memset(audio_buffer, 0, I2S_BUFFER_SIZE * I2S_CHANNELS * sizeof(int16_t));
        buffer.audio_buffer = (uint8_t *)audio_buffer;
        buffer.bytes_size = I2S_BUFFER_SIZE * I2S_CHANNELS * sizeof(int16_t);
        
        beat_detection_data_write(g_beat_detection_handle, buffer);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG, "=== Example Finished ===");
}
