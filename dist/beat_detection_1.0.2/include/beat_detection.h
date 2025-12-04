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
 * @brief Task configuration for beat detection
 */
typedef struct {
    UBaseType_t            priority;      /*!< Task priority */
    uint32_t               stack_size;     /*!< Task stack size in bytes */
    BaseType_t             core_id;        /*!< Task core ID (0 or 1) */
} beat_detection_task_cfg_t;

/**
 * @brief Audio processing configuration for beat detection
 */
typedef struct {
    int16_t                sample_rate;    /*!< Audio sample rate in Hz */
    uint8_t                channel;        /*!< Number of audio channels (1 or 2) */
    int16_t                fft_size;       /*!< FFT size (must be power of 2) */
} beat_detection_audio_cfg_t;

/**
 * @brief Bass detection configuration for beat detection
 */
typedef struct {
    uint16_t               freq_start;     /*!< Start frequency of bass range in Hz */
    uint16_t               freq_end;       /*!< End frequency of bass range in Hz */
    float                  surge_threshold; /*!< Threshold for bass surge detection */
} beat_detection_bass_cfg_t;

/**
 * @brief Callback configuration for beat detection
 */
typedef struct {
    beat_detection_result_callback_t callback; /*!< Result callback function */
    void*                            ctx;       /*!< Callback context */
} beat_detection_callback_cfg_t;

/**
 * @brief Flags configuration for beat detection
 */
typedef struct {
    bool enable_psram : 1;  /*!< Enable PSRAM allocation */
} beat_detection_flags_t;

/**
 * @brief Beat detection configuration structure
 */
typedef struct {
    beat_detection_audio_cfg_t    audio;      /*!< Audio processing configuration */
    beat_detection_bass_cfg_t     bass;       /*!< Bass detection configuration */
    beat_detection_task_cfg_t     task;       /*!< Task configuration */
    beat_detection_callback_cfg_t callback;   /*!< Callback configuration */
    beat_detection_flags_t         flags;      /*!< Flags configuration */
} beat_detection_cfg_t;

/**
 * @brief Internal beat detection structure (opaque to users)
 */
typedef struct beat_detection {
    float*                              fft_buffer;
    int16_t                             fft_size;
    float*                              window;
    int16_t                             sample_rate;
    uint8_t                             channel;
    uint8_t                             bass_bin_start;
    uint8_t                             bass_bin_end;
    float                               bass_surge_threshold;
    float*                              magnitude;
    float*                              magnitude_prev;
    uint64_t                            last_bass_detected_time;
    StackType_t*                        task_stack_buffer;
    StaticTask_t*                       task_tcb;
    TaskHandle_t                        task_handle;
    void*                               task_ctx;
    beat_detection_result_callback_t    result_callback;
    void*                               result_callback_ctx;
    QueueHandle_t                       audio_queue;
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
 