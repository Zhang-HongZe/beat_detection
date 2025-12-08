#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_dsp.h"
#include "beat_detection_config.h"
#include "beat_detection.h"

static const char *TAG = "BEAT_DETECTION";

#ifdef __cplusplus
extern "C" {
#endif

static inline uint16_t beat_detection_hz_to_bin(uint16_t hz, beat_detection_handle_t handle)
{
    float bin_hz = (float)handle->audio.sample_rate / (float)handle->audio.fft_size;
    int bin = (int)roundf(hz / bin_hz);
    if (bin < 1) {
        return 1;
    }
    if (bin >= handle->audio.fft_size / 2) {
        return (handle->audio.fft_size / 2) - 1;
    }
    return (uint16_t)bin;
}

static bool detect_bass_surge(float current_bass, float prev_bass, beat_detection_handle_t handle)
{
    if (prev_bass <= 0.0f) {
        return false;
    }
    float ratio = current_bass / prev_bass;
    return (ratio >= handle->audio.threshold);
}

static beat_detection_result_t beat_detection(beat_detection_handle_t handle, int16_t *audio_buffer)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Invalid arguments");
        return BEAT_DETECTION_FAILED;
    }

    if (handle->audio.channel == 1) {
        for (int i = 0; i < handle->audio.fft_size; i++) {
            handle->audio.fft_buffer[2 * i] = ((float)(audio_buffer[i]) / 32768.0f) * handle->audio.window[i];
            handle->audio.fft_buffer[2 * i + 1] = 0.0f;
        }
    } else if (handle->audio.channel == 2) {
        for (int i = 0; i < handle->audio.fft_size; i++) {
            handle->audio.fft_buffer[2 * i] = ((float)(audio_buffer[2 * i + 1]) / 32768.0f) * handle->audio.window[i];
            handle->audio.fft_buffer[2 * i + 1] = 0.0f;
        }
    } else {
        ESP_LOGE(TAG, "Invalid channel");
        return BEAT_DETECTION_FAILED;
    }

    dsps_fft2r_fc32(handle->audio.fft_buffer, handle->audio.fft_size);
    dsps_bit_rev_fc32(handle->audio.fft_buffer, handle->audio.fft_size);

    for (int i = 0; i < handle->audio.fft_size / 2; ++i) {
        float real = handle->audio.fft_buffer[2 * i];
        float imag = handle->audio.fft_buffer[2 * i + 1];
        float a = 0.9f;
        float magnitude = sqrtf(real * real + imag * imag);
        handle->audio.magnitude[i] = a * magnitude + (1 - a) * handle->audio.magnitude_prev[i];
    }

    float current_bass = 0.0f;
    float prev_bass = 0.0f;
    float current_sum = 0.0f;
    float prev_sum = 0.0f;
    float average_ratio = 0.0f;
    for (int bin = handle->audio.bass_bin_start; bin <= handle->audio.bass_bin_end && bin < handle->audio.fft_size / 2; ++bin) {
        if (handle->audio.magnitude_prev[bin] > prev_bass) {
            prev_bass = handle->audio.magnitude_prev[bin];
        }
        if (handle->audio.magnitude[bin] > current_bass) {
            current_bass = handle->audio.magnitude[bin];
        }
        current_sum += handle->audio.magnitude[bin];
        prev_sum += handle->audio.magnitude_prev[bin];
    }
    average_ratio = current_sum / prev_sum;

    memcpy(handle->audio.magnitude_prev, handle->audio.magnitude, sizeof(float) * handle->audio.fft_size / 2);

    // detect bass drum
    if ((detect_bass_surge(current_bass, prev_bass, handle) || average_ratio > handle->audio.average_ratio) 
        && current_bass > handle->audio.min_energy 
        && xTaskGetTickCount() - handle->audio.last_bass_detected_time > pdMS_TO_TICKS(handle->audio.time_interval)) {
        // float surge_ratio = current_bass / prev_bass;
        // ESP_LOGI(TAG, "🎵 Bass drum detected! Energy surge: %.2fx (current=%.3f, prev=%.3f)", 
        //          surge_ratio, current_bass, prev_bass);
        handle->audio.last_bass_detected_time = xTaskGetTickCount();
        return BEAT_DETECTED;
    }

    return BEAT_NOT_DETECTED;
}

esp_err_t beat_detection_data_write(beat_detection_handle_t handle, beat_detection_audio_buffer_t buffer)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }

    if (buffer.bytes_size < handle->audio.channel * handle->audio.fft_size * sizeof(int16_t)) {
        ESP_LOGE(TAG, "Audio buffer size is less than FFT size");
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->status.is_calculating) {
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t loc = (handle->status.enable_psram) ? MALLOC_CAP_SPIRAM: MALLOC_CAP_INTERNAL;

    uint8_t *audio_buffer = (uint8_t *)heap_caps_malloc(handle->audio.channel * handle->audio.fft_size * sizeof(int16_t), loc | MALLOC_CAP_8BIT);
    if (audio_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for audio buffer");
        return ESP_ERR_NO_MEM;
    }
    memcpy(audio_buffer, buffer.audio_buffer, handle->audio.channel * handle->audio.fft_size * sizeof(int16_t));

    beat_detection_audio_buffer_t processed_buffer = {
        .audio_buffer = audio_buffer,
        .bytes_size = handle->audio.channel * handle->audio.fft_size * sizeof(int16_t),
    };
    if (xQueueSend(handle->task.audio_queue, &processed_buffer, pdMS_TO_TICKS(0)) != pdPASS) {
        ESP_LOGE(TAG, "Failed to send audio buffer to queue");
        heap_caps_free(audio_buffer);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void beat_detection_task(void *arg)
{
    beat_detection_handle_t handle = (beat_detection_handle_t)arg;
    while (true) {
        beat_detection_audio_buffer_t received_buffer;
        xQueueReceive(handle->task.audio_queue, &received_buffer, portMAX_DELAY);
        handle->status.is_calculating = true;
        if (received_buffer.bytes_size < handle->audio.channel * handle->audio.fft_size * sizeof(int16_t)) {
            ESP_LOGE(TAG, "Audio buffer size is less than FFT size");
            continue;
        }
        beat_detection_result_t result = beat_detection(handle, (int16_t *)received_buffer.audio_buffer);
        heap_caps_free(received_buffer.audio_buffer);
        if (handle->audio.result_callback != NULL) {
            handle->audio.result_callback(result, handle->audio.result_callback_ctx);
        }
        handle->status.is_calculating = false;
    }
    vTaskDelete(NULL);
}

esp_err_t beat_detection_init(beat_detection_cfg_t *cfg, beat_detection_handle_t *handle)
{
    if (cfg == NULL || handle == NULL) {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;

    *handle = NULL;
    uint32_t local_flags = (cfg->flags.enable_psram) ? MALLOC_CAP_SPIRAM: MALLOC_CAP_INTERNAL;

    dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize FFT");
        return ret;
    }

    *handle = (beat_detection_handle_t)heap_caps_malloc(sizeof(beat_detection_t), local_flags | MALLOC_CAP_8BIT);
    if (*handle == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for Beat detection handle");
        beat_detection_deinit(handle);
        return ESP_ERR_NO_MEM;
    }
    memset(*handle, 0, sizeof(beat_detection_t));

    (*handle)->audio.fft_size = cfg->audio_cfg.fft_size;
    (*handle)->audio.sample_rate = cfg->audio_cfg.sample_rate;
    (*handle)->audio.channel = cfg->audio_cfg.channel;
    (*handle)->audio.bass_bin_start = beat_detection_hz_to_bin(cfg->audio_cfg.bass_freq_start, *handle);
    (*handle)->audio.bass_bin_end = beat_detection_hz_to_bin(cfg->audio_cfg.bass_freq_end, *handle);
    (*handle)->audio.threshold = cfg->audio_cfg.threshold;
    (*handle)->audio.average_ratio = cfg->audio_cfg.average_ratio;
    (*handle)->audio.min_energy = cfg->audio_cfg.min_energy;
    (*handle)->audio.time_interval = cfg->audio_cfg.time_interval;
    (*handle)->audio.result_callback = cfg->result_callback;
    (*handle)->audio.result_callback_ctx = cfg->result_callback_ctx;
    (*handle)->status.is_calculating = false;
    (*handle)->status.enable_psram = cfg->flags.enable_psram;
    ESP_LOGI(TAG, "Bass frequency range: %d-%d Hz, bins: %d-%d", 
             (int)cfg->audio_cfg.bass_freq_start, (int)cfg->audio_cfg.bass_freq_end, (*handle)->audio.bass_bin_start, (*handle)->audio.bass_bin_end);

    (*handle)->audio.fft_buffer = (float *)heap_caps_malloc( 2 * (*handle)->audio.fft_size * sizeof(float), local_flags | MALLOC_CAP_8BIT);
    if ((*handle)->audio.fft_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for FFT buffer");
        beat_detection_deinit(handle);
        return ESP_ERR_NO_MEM;
    }
    memset((*handle)->audio.fft_buffer, 0, 2 * (*handle)->audio.fft_size * sizeof(float));

    (*handle)->audio.window = heap_caps_malloc((*handle)->audio.fft_size * sizeof(float), local_flags | MALLOC_CAP_8BIT);
    if ((*handle)->audio.window == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for window");
        beat_detection_deinit(handle);
        return ESP_ERR_NO_MEM;
    }
    memset((*handle)->audio.window, 0, (*handle)->audio.fft_size * sizeof(float));
    dsps_wind_hann_f32((*handle)->audio.window, (*handle)->audio.fft_size);

    (*handle)->audio.magnitude = (float *)heap_caps_malloc((*handle)->audio.fft_size / 2 * sizeof(float), local_flags | MALLOC_CAP_8BIT);
    if ((*handle)->audio.magnitude == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for magnitude");
        beat_detection_deinit(handle);
        return ESP_ERR_NO_MEM;
    }
    memset((*handle)->audio.magnitude, 0, (*handle)->audio.fft_size / 2 * sizeof(float));

    (*handle)->audio.magnitude_prev = (float *)heap_caps_malloc((*handle)->audio.fft_size / 2 * sizeof(float), local_flags | MALLOC_CAP_8BIT);
    if ((*handle)->audio.magnitude_prev == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for magnitude previous");
        beat_detection_deinit(handle);
        return ESP_ERR_NO_MEM;
    }
    memset((*handle)->audio.magnitude_prev, 0, (*handle)->audio.fft_size / 2 * sizeof(float));

    (*handle)->task.audio_queue = xQueueCreate(1, sizeof(beat_detection_audio_buffer_t));
    if ((*handle)->task.audio_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create audio queue");
        beat_detection_deinit(handle);
        return ESP_ERR_NO_MEM;
    }

    if (cfg->flags.enable_psram) {
        (*handle)->task.task_stack_buffer = (StackType_t *)heap_caps_malloc(cfg->task_cfg.stack_size, local_flags | MALLOC_CAP_8BIT);
        if ((*handle)->task.task_stack_buffer == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for task stack");
            beat_detection_deinit(handle);
            return ESP_ERR_NO_MEM;
        }
        memset((*handle)->task.task_stack_buffer, 0, sizeof(StackType_t) * cfg->task_cfg.stack_size);

        (*handle)->task.task_tcb = (StaticTask_t *)heap_caps_malloc(sizeof(StaticTask_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if ((*handle)->task.task_tcb == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for task TCB");
            beat_detection_deinit(handle);
            return ESP_ERR_NO_MEM;
        }
        memset((*handle)->task.task_tcb, 0, sizeof(StaticTask_t));

        (*handle)->task.task_handle = xTaskCreateStaticPinnedToCore(
            beat_detection_task,
            "beat_detection_task",
            cfg->task_cfg.stack_size, 
            *handle,
            cfg->task_cfg.priority, 
            (*handle)->task.task_stack_buffer, 
            (*handle)->task.task_tcb, 
            cfg->task_cfg.core_id
        );
    } else {
        xTaskCreatePinnedToCore(
            beat_detection_task,
            "beat_detection_task",
            cfg->task_cfg.stack_size, 
            *handle,
            cfg->task_cfg.priority, 
            &(*handle)->task.task_handle, 
            cfg->task_cfg.core_id
        );
    }

    if((*handle)->task.task_handle == NULL) {
        beat_detection_deinit(handle);
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t beat_detection_deinit(beat_detection_handle_t *handle)
{
    dsps_fft2r_deinit_fc32();

    if (*handle == NULL) {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }

    if ((*handle)->audio.fft_buffer != NULL) {
        heap_caps_free((*handle)->audio.fft_buffer);
    }
    if ((*handle)->audio.window != NULL) {
        heap_caps_free((*handle)->audio.window);
    }
    if ((*handle)->audio.magnitude != NULL) {
        heap_caps_free((*handle)->audio.magnitude);
    }
    if ((*handle)->audio.magnitude_prev != NULL) {
        heap_caps_free((*handle)->audio.magnitude_prev);
    }
    if ((*handle)->task.audio_queue != NULL) {
        vQueueDelete((*handle)->task.audio_queue);
    }
    if ((*handle)->task.task_stack_buffer != NULL) {
        heap_caps_free((*handle)->task.task_stack_buffer);
    }
    if ((*handle)->task.task_tcb != NULL) {
        heap_caps_free((*handle)->task.task_tcb);
    }
    if ((*handle)->task.task_handle != NULL) {
        vTaskDelete((*handle)->task.task_handle);
    }
    if (*handle != NULL) {
        heap_caps_free(*handle);
    }
    *handle = NULL;
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif