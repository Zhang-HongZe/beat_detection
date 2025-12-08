#pragma once

/* default config */
#define BEAT_DETECTION_DEFAULT_SAMPLE_RATE                              (16000)
#define BEAT_DETECTION_DEFAULT_CHANNEL                                  (2)
#define BEAT_DETECTION_DEFAULT_FFT_SIZE                                 (512)
#define BEAT_DETECTION_DEFAULT_BASS_FREQ_MIN                            (200)
#define BEAT_DETECTION_DEFAULT_BASS_FREQ_MAX                            (300)
#define BEAT_DETECTION_DEFAULT_TASK_PRIORITY                            (3)
#define BEAT_DETECTION_DEFAULT_TASK_STACK_SIZE                          (1024 * 5)
#define BEAT_DETECTION_DEFAULT_TASK_CORE_ID                             (0)
#define BEAT_DETECTION_DEFAULT_ENABLE_PSRAM                             (false)
#define BEAT_DETECTION_DEFAULT_THRESHOLD                                (6.0f)
#define BEAT_DETECTION_DEFAULT_AVERAGE_RATIO                            (5.0f)
#define BEAT_DETECTION_DEFAULT_MIN_ENERGY                               (0.01f)
#define BEAT_DETECTION_DEFAULT_TIME_INTERVAL                            (100)

#define BEAT_DETECTION_DEFAULT_CFG() {                                          \
    .audio_cfg = {                                                              \
        .sample_rate = BEAT_DETECTION_DEFAULT_SAMPLE_RATE,                      \
        .channel = BEAT_DETECTION_DEFAULT_CHANNEL,                              \
        .fft_size = BEAT_DETECTION_DEFAULT_FFT_SIZE,                            \
        .bass_freq_start = BEAT_DETECTION_DEFAULT_BASS_FREQ_MIN,                \
        .bass_freq_end = BEAT_DETECTION_DEFAULT_BASS_FREQ_MAX,                  \
        .threshold = BEAT_DETECTION_DEFAULT_THRESHOLD,                          \
        .average_ratio = BEAT_DETECTION_DEFAULT_AVERAGE_RATIO,                  \
        .min_energy = BEAT_DETECTION_DEFAULT_MIN_ENERGY,                        \
        .time_interval = BEAT_DETECTION_DEFAULT_TIME_INTERVAL,                  \
    },                                                                          \
    .task_cfg = {                                                               \
        .priority = BEAT_DETECTION_DEFAULT_TASK_PRIORITY,                       \
        .stack_size = BEAT_DETECTION_DEFAULT_TASK_STACK_SIZE,                   \
        .core_id = BEAT_DETECTION_DEFAULT_TASK_CORE_ID,                         \
    },                                                                          \
    .result_callback = NULL,                                                    \
    .result_callback_ctx = NULL,                                                \
    .flags = {                                                                  \
        .enable_psram = false,                                                  \
    }                                                                           \
}
