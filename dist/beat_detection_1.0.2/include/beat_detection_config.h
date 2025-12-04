#pragma once

#define BEAT_DETECTION_DEFAULT_FFT_SIZE                                 (512)
#define BEAT_DETECTION_DEFAULT_SAMPLE_RATE                              (16000)
#define BEAT_DETECTION_DEFAULT_BASS_FREQ_MIN                            (200.0f)
#define BEAT_DETECTION_DEFAULT_BASS_FREQ_MAX                            (300.0f)
#define BEAT_DETECTION_DEFAULT_BASS_SURGE_THRESHOLD                     (6.0f)
#define BEAT_DETECTION_DEFAULT_TASK_PRIORITY                            (3)
#define BEAT_DETECTION_DEFAULT_TASK_STACK_SIZE                          (1024 * 8)
#define BEAT_DETECTION_DEFAULT_TASK_CORE_ID                             (0)

#define BEAT_DETECTION_DEFAULT_CFG() {                                          \
    .audio = {                                                                  \
        .sample_rate = BEAT_DETECTION_DEFAULT_SAMPLE_RATE,                      \
        .channel = 1,                                                           \
        .fft_size = BEAT_DETECTION_DEFAULT_FFT_SIZE,                            \
    },                                                                          \
    .bass = {                                                                   \
        .freq_start = BEAT_DETECTION_DEFAULT_BASS_FREQ_MIN,                     \
        .freq_end = BEAT_DETECTION_DEFAULT_BASS_FREQ_MAX,                       \
        .surge_threshold = BEAT_DETECTION_DEFAULT_BASS_SURGE_THRESHOLD,         \
    },                                                                          \
    .task = {                                                                   \
        .priority = BEAT_DETECTION_DEFAULT_TASK_PRIORITY,                       \
        .stack_size = BEAT_DETECTION_DEFAULT_TASK_STACK_SIZE,                   \
        .core_id = BEAT_DETECTION_DEFAULT_TASK_CORE_ID,                         \
    },                                                                          \
    .callback = {                                                               \
        .callback = NULL,                                                       \
        .ctx = NULL,                                                            \
    },                                                                          \
    .flags = {                                                                  \
        .enable_psram = false,                                                  \
    }                                                                           \
}