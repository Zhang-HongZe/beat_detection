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

typedef void (*beat_detection_feed_audio_callback_t)(beat_detection_audio_buffer_t buffer, void *ctx);

typedef struct beat_detection {
    float                  *fft_buffer;
    float                  *window;
    int16_t                fft_size;
    int16_t                sample_rate;
    uint8_t                channel;
    uint8_t                bass_bin_start;
    uint8_t                bass_bin_end;
    float                  bass_surge_threshold;
    float                  *magnitude;
    float                  *magnitude_prev;
    uint64_t               last_bass_detected_time;
    StackType_t*           task_stack_buffer;
    StaticTask_t*          task_tcb;
    TaskHandle_t           task_handle;
    void*                  task_ctx;
    beat_detection_feed_audio_callback_t feed_audio;
    void*                  feed_audio_ctx;
    beat_detection_result_callback_t result_callback;
    void*                  result_callback_ctx;
    QueueHandle_t          audio_queue;
    struct {
        bool enable_psram : 1;
        bool is_calculating : 1;
    }status;
} beat_detection_t;

typedef beat_detection_t* beat_detection_handle_t;

typedef struct {
    int16_t                sample_rate;
    uint8_t                channel;
    int16_t                fft_size;
    uint16_t               bass_freq_start;
    uint16_t               bass_freq_end;
    float                  bass_surge_threshold;
    UBaseType_t            task_priority;
    uint32_t               task_stack_size;
    BaseType_t             task_core_id;
    void*                  result_callback;
    void*                  result_callback_ctx;
    struct {
        bool enable_psram : 1;
    }flags;
} beat_detection_cfg_t;


/**
* @brief  Feed audio buffer to Beat Detection module
*
*         Feed audio buffer to Beat Detection module.
*         This function is weak, and can be overridden by the user.
*
* @param  buffer  Audio buffer to feed
* @param  ctx     Context pointer
*
* @return
*       - void
*/
static void beat_detection_feed(beat_detection_audio_buffer_t buffer, void *ctx);

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
* @brief  Perform beat detection
*
*         Perform beat detection on the audio buffer.
*         This function is weak, and can be overridden by the user.
*
* @param  handle  Pointer to the Beat Detection handle
* @param  audio_buffer  Pointer to the audio buffer
*
* @return
*       - ESP_OK  Beat detection successful
*       - Other   Error code if beat detection failed
*/
beat_detection_result_t beat_detection(beat_detection_handle_t handle, int16_t *audio_buffer);

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

#ifdef __cplusplus
}
#endif  /* __cplusplus */
 