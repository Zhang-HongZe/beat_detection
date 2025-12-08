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

/* configurable parameters from Kconfig */
#ifndef CONFIG_BEAT_DETECTION_THRESHOLD
#define CONFIG_BEAT_DETECTION_THRESHOLD 60
#endif
#ifndef CONFIG_BEAT_DETECTION_AVERAGE_RATIO
#define CONFIG_BEAT_DETECTION_AVERAGE_RATIO 50
#endif
#ifndef CONFIG_BEAT_DETECTION_MIN_ENERGY
#define CONFIG_BEAT_DETECTION_MIN_ENERGY 10
#endif
#ifndef CONFIG_BEAT_DETECTION_TIME_INTERVAL
#define CONFIG_BEAT_DETECTION_TIME_INTERVAL 100
#endif

/* Convert Kconfig values to actual float/int values */
#define BEAT_DETECTION_THRESHOLD                                  ((float)CONFIG_BEAT_DETECTION_THRESHOLD / 10.0f)
#define BEAT_DETECTION_AVERAGE_RATIO                              ((float)CONFIG_BEAT_DETECTION_AVERAGE_RATIO / 10.0f)
#define BEAT_DETECTION_MIN_ENERGY                                 ((float)CONFIG_BEAT_DETECTION_MIN_ENERGY / 1000.0f)
#define BEAT_DETECTION_TIME_INTERVAL                              (CONFIG_BEAT_DETECTION_TIME_INTERVAL)

#define BEAT_DETECTION_DEFAULT_CFG() {                                          \
    .audio_cfg = {                                                              \
        .sample_rate = BEAT_DETECTION_DEFAULT_SAMPLE_RATE,                      \
        .channel = BEAT_DETECTION_DEFAULT_CHANNEL,                              \
        .fft_size = BEAT_DETECTION_DEFAULT_FFT_SIZE,                            \
        .bass_freq_start = BEAT_DETECTION_DEFAULT_BASS_FREQ_MIN,                \
        .bass_freq_end = BEAT_DETECTION_DEFAULT_BASS_FREQ_MAX,                  \
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
