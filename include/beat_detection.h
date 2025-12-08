/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "beat_detection_config.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef enum {
    BEAT_NOT_DETECTED = 0,
    BEAT_DETECTED = 1,
    BEAT_DETECTION_FAILED = 2,
} beat_detection_result_t;

typedef struct {
    uint8_t *audio_buffer;
    size_t bytes_size;
} beat_detection_audio_buffer_t;

typedef void (*beat_detection_result_callback_t)(beat_detection_result_t result, void *ctx);

/**
 * @brief Beat detection configuration structure
 */
typedef struct {
    struct {
        int16_t                         sample_rate;        // 采样率（Hz），默认 16000
        uint8_t                         channel;            // 声道数：1=单声道，2=双声道
        int16_t                         fft_size;           // FFT 大小（2的幂次），默认 512
        int16_t                         bass_freq_start;    // 低音频率起始（Hz），默认 200
        int16_t                         bass_freq_end;      // 低音频率结束（Hz），默认 300
    }audio_cfg;
    struct {
        UBaseType_t                     priority;           // 任务优先级，默认 3
        uint32_t                        stack_size;         // 任务栈大小（字节），默认 8192
        BaseType_t                      core_id;            // 任务绑定的 CPU 核心，默认 0
    }task_cfg;
    beat_detection_result_callback_t    result_callback;
    void*                               result_callback_ctx;
    struct {
        bool enable_psram : 1;
    }flags;
} beat_detection_cfg_t;

/**
 * @brief Internal beat detection structure (opaque to users)
 */
typedef struct beat_detection {
    struct {
        float*                              fft_buffer;
        int16_t                             fft_size;
        float*                              window;
        int16_t                             sample_rate;
        uint8_t                             channel;
        uint8_t                             bass_bin_start;
        uint8_t                             bass_bin_end;
        float*                              magnitude;
        float*                              magnitude_prev;
        uint64_t                            last_bass_detected_time;
        beat_detection_result_callback_t    result_callback;
        void*                               result_callback_ctx;
    }audio;
    struct {
        StackType_t*                        task_stack_buffer;
        StaticTask_t*                       task_tcb;
        TaskHandle_t                        task_handle;
        void*                               task_ctx;
        QueueHandle_t                       audio_queue;
    }task;
    struct {
        bool enable_psram : 1;
        bool is_calculating : 1;
    }status;
} beat_detection_t;

typedef beat_detection_t* beat_detection_handle_t;

/**
* @brief  Initialize Beat Detection module
*
*         Initialize the Beat Detection module and allocate the necessary memory.
*         This function is weak, and can be overridden by the user.
*
* @param  handle  Pointer to the Beat Detection handle
*
* @return
*       - ESP_OK  Beat detection initialization successful
*       - Other   Error code if initialization failed
*/
esp_err_t beat_detection_init(beat_detection_cfg_t *cfg, beat_detection_handle_t *handle);

/**
* @brief  Deinitialize Beat Detection module
*
*         Deinitialize the Beat Detection module and free the allocated memory.
*
* @param  handle  Pointer to the Beat Detection handle
*
* @return
*       - ESP_OK  Deinitialization successful
*/
esp_err_t beat_detection_deinit(beat_detection_handle_t *handle);

/**
* @brief  Write audio data to Beat Detection module
*
*         Write audio data to the Beat Detection module.
*
* @param  handle  Pointer to the Beat Detection handle
* @param  buffer  Audio buffer
*
* @return
*       - ESP_OK  Audio data written successfully
*       - Other   Error code if audio data writing failed
*/
esp_err_t beat_detection_data_write(beat_detection_handle_t handle, beat_detection_audio_buffer_t buffer);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
 