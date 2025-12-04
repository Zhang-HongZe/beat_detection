# Beat Detection Component

## 概述

Beat Detection 组件是一个基于 FFT（快速傅里叶变换）的音频节奏检测模块，用于实时检测音频信号中的鼓点（beat）。该组件通过分析音频的低频能量变化来识别鼓点事件。

## 功能特性

- **实时鼓点检测**：基于 FFT 算法实时分析音频信号
- **低音频率检测**：专注于 200-300Hz 的低音频率范围（可配置）
- **能量突变检测**：通过检测低频能量的突然增加来识别鼓点
- **异步处理**：使用独立任务处理音频数据，不阻塞主流程
- **回调机制**：支持检测结果回调通知
- **内存优化**：支持 PSRAM 内存分配，减少内部 RAM 占用
- **多通道支持**：支持单声道和双声道音频输入

## 算法原理

### 检测流程

1. **音频预处理**
   - 接收音频数据（16位 PCM）
   - 应用 Hann 窗函数减少频谱泄漏
   - 转换为浮点数格式

2. **FFT 变换**
   - 对音频帧进行 FFT 变换
   - 计算频域幅度谱

3. **幅度平滑**
   - 使用指数移动平均（EMA）平滑幅度值
   - 平滑系数：0.9（当前帧） + 0.1（前一帧）

4. **低音能量计算**
   - 在配置的低音频率范围内（默认 200-300Hz）
   - 计算当前帧和前一帧的最大能量值
   - 计算能量总和的平均比值

5. **鼓点判定**
   - 能量突变检测：当前能量 / 前一帧能量 ≥ 阈值（默认 6.0）
   - 平均比值检测：平均能量比值 > 5.0
   - 能量阈值：当前低音能量 > 0.01
   - 时间间隔：距离上次检测 > 150ms（防止重复检测）

## API 文档

### 数据结构

#### `beat_detection_cfg_t`

配置结构体，用于初始化 beat detection 组件。

```c
typedef struct {
    int16_t                sample_rate;        // 采样率（Hz），默认 16000
    uint8_t                channel;            // 声道数：1=单声道，2=双声道
    int16_t                fft_size;           // FFT 大小（2的幂次），默认 512
    uint8_t                bass_freq_start;    // 低音频率起始（Hz），默认 200
    uint8_t                bass_freq_end;      // 低音频率结束（Hz），默认 300
    float                  bass_surge_threshold; // 能量突变阈值，默认 6.0
    UBaseType_t            task_priority;      // 任务优先级，默认 10
    uint32_t               task_stack_size;    // 任务栈大小（字节），默认 8192
    BaseType_t             task_core_id;       // 任务绑定的 CPU 核心，默认 0
    void*                  result_callback;    // 结果回调函数
    void*                  result_callback_ctx; // 回调函数上下文
    struct {
        bool enable_psram : 1;                 // 是否使用 PSRAM，默认 false
    }flags;
} beat_detection_cfg_t;
```

#### `beat_detection_result_t`

检测结果枚举。

```c
typedef enum {
    BEAT_NOT_DETECTED = 0,    // 未检测到鼓点
    BEAT_DETECTED = 1,         // 检测到鼓点
    BEAT_DETECTION_FAILED = 2, // 检测失败
} beat_detection_result_t;
```

#### `beat_detection_audio_buffer_t`

音频缓冲区结构体。

```c
typedef struct {
    int16_t *audio_buffer;  // 音频数据缓冲区（16位 PCM）
    size_t size;            // 缓冲区大小（样本数）
} beat_detection_audio_buffer_t;
```

### 函数接口

#### `beat_detection_init()`

初始化 beat detection 组件。

```c
esp_err_t beat_detection_init(beat_detection_cfg_t *cfg, beat_detection_handle_t *handle);
```

**参数：**
- `cfg`: 配置结构体指针
- `handle`: 输出参数，返回初始化后的句柄指针

**返回值：**
- `ESP_OK`: 初始化成功
- `ESP_ERR_INVALID_ARG`: 参数无效
- `ESP_ERR_NO_MEM`: 内存分配失败
- 其他错误码

**注意：**
- 如果初始化失败，`*handle` 会被设置为 `NULL`
- 函数会创建独立的任务来处理音频数据

#### `beat_detection()`

执行鼓点检测（内部使用，通常不需要直接调用）。

```c
beat_detection_result_t beat_detection(beat_detection_handle_t handle, int16_t *audio_buffer);
```

**参数：**
- `handle`: beat detection 句柄
- `audio_buffer`: 音频数据缓冲区（16位 PCM）

**返回值：**
- `BEAT_DETECTED`: 检测到鼓点
- `BEAT_NOT_DETECTED`: 未检测到鼓点
- `BEAT_DETECTION_FAILED`: 检测失败

#### `beat_detection_deinit()`

反初始化 beat detection 组件，释放所有分配的资源。

```c
esp_err_t beat_detection_deinit(beat_detection_handle_t *handle);
```

**参数：**
- `handle`: beat detection 句柄指针

**返回值：**
- `ESP_OK`: 反初始化成功
- `ESP_ERR_INVALID_ARG`: 参数无效

**注意：**
- 函数会将 `*handle` 设置为 `NULL`
- 会删除创建的任务并释放所有内存

## 配置说明

### 默认配置

使用 `BEAT_DETECTION_DEFAULT_CFG()` 宏可以快速创建默认配置：

```c
beat_detection_cfg_t cfg = BEAT_DETECTION_DEFAULT_CFG();
```

默认值：
- 采样率：16000 Hz
- FFT 大小：512
- 低音频率范围：200-300 Hz
- 能量突变阈值：6.0
- 任务优先级：3
- 任务栈大小：8192 字节
- CPU 核心：0
- PSRAM：禁用

### 自定义配置示例

```c
beat_detection_cfg_t cfg = {
    .sample_rate = 16000,
    .channel = 2,                    // 双声道
    .fft_size = 512,
    .bass_freq_start = 200,
    .bass_freq_end = 300,
    .bass_surge_threshold = 6.0f,
    .task_priority = 3,
    .task_stack_size = 8192,
    .task_core_id = 0,
    .result_callback = my_callback,
    .result_callback_ctx = my_ctx,
    .flags = {
        .enable_psram = true,        // 使用 PSRAM
    }
};
```

## 使用示例

### 基本使用

```c
#include "audio_beat_detection.h"
#include "audio_beat_detection_config.h"

// 结果回调函数
static void beat_detection_result_callback(beat_detection_result_t result, void *ctx)
{
    if (result == BEAT_DETECTED) {
        ESP_LOGI("APP", "Beat detected!");
        // 发送 UART 消息或其他处理
        const char *msg = "BEAT_DETECTED\n";
        uart_write_bytes(UART_NUM_0, msg, strlen(msg));
    }
}

void app_main(void)
{
    beat_detection_handle_t beat_detection_handle = NULL;
    
    // 配置 beat detection
    beat_detection_cfg_t cfg = BEAT_DETECTION_DEFAULT_CFG();
    cfg.result_callback = beat_detection_result_callback;
    cfg.result_callback_ctx = NULL;
    cfg.channel = 2;  // 双声道
    
    // 初始化
    esp_err_t ret = beat_detection_init(&cfg, &beat_detection_handle);
    if (ret != ESP_OK || beat_detection_handle == NULL) {
        ESP_LOGE("APP", "Failed to initialize beat detection");
        return;
    }
    
    // 通过 feed_audio 函数输入音频数据
    // beat_detection_handle->feed_audio(buffer, beat_detection_handle);
    
    // ... 其他代码 ...
    
    // 清理
    beat_detection_deinit(&beat_detection_handle);
}
```

### 与 GMF 框架集成

```c
// 在 GMF pipeline 的 outport_release_write 回调中
static esp_gmf_err_io_t outport_release_write(void *handle, esp_gmf_payload_t *load, int block_ticks)
{
    if (g_ctx.beat_detection == NULL) {
        return ESP_GMF_IO_OK;
    }
    
    beat_detection_audio_buffer_t buffer = {
        .audio_buffer = (int16_t *)load->buf,
        .size = load->buf_length / sizeof(int16_t),
    };
    
    if (g_ctx.beat_detection->feed_audio != NULL) {
        g_ctx.beat_detection->feed_audio(buffer, g_ctx.beat_detection);
    }
    
    return ESP_GMF_IO_OK;
}
```

## 参数调优

### FFT 大小

- **512**：默认值，适合大多数场景，平衡了精度和性能
- **256**：更快的处理速度，但频率分辨率较低
- **1024**：更高的频率分辨率，但需要更多内存和计算时间

### 低音频率范围

- **200-300 Hz**：默认值，适合大多数鼓点检测
- **150-250 Hz**：更低的频率范围，适合低音鼓
- **250-350 Hz**：稍高的频率范围，适合某些类型的鼓

### 能量突变阈值

- **6.0**：默认值，适合大多数音乐
- **4.0-5.0**：更敏感，可能产生更多误检
- **7.0-8.0**：更保守，可能漏检一些弱鼓点

### 任务配置

- **优先级**：建议设置为 5-10，确保及时处理音频数据
- **栈大小**：默认 8192 字节，如果出现栈溢出，可以增加到 10240 或更大
- **CPU 核心**：可以绑定到特定核心，避免与其他任务竞争

## 注意事项

1. **内存管理**
   - 组件会在初始化时分配大量内存（FFT 缓冲区、窗函数、幅度数组等）
   - 如果启用 PSRAM，会使用外部 RAM，减少内部 RAM 占用
   - 确保系统有足够的内存空间

2. **音频格式**
   - 输入音频必须是 16 位 PCM 格式
   - 采样率必须与配置的 `sample_rate` 一致
   - 支持单声道和双声道，双声道时使用右声道（索引 1, 3, 5...）

3. **数据流**
   - 音频数据通过 `feed_audio` 函数输入
   - 数据会被复制到内部队列，由独立任务处理
   - 如果队列满了，新的数据会被丢弃

4. **线程安全**
   - `feed_audio` 函数可以在任何线程中调用
   - 检测结果通过回调函数返回，回调在检测任务中执行
   - 回调函数应尽量简短，避免阻塞

5. **性能考虑**
   - FFT 计算需要一定的 CPU 资源
   - 建议在专用的 CPU 核心上运行检测任务
   - 如果处理速度跟不上，可以增加任务优先级或减少 FFT 大小

6. **初始化检查**
   - 初始化失败时，`*handle` 会被设置为 `NULL`
   - 使用前应检查句柄是否为 `NULL`
   - 在调用 `feed_audio` 前确保组件已正确初始化

## 依赖项

- **ESP-DSP**：用于 FFT 计算和窗函数生成
- **FreeRTOS**：用于任务管理和队列操作
- **ESP-IDF**：基础框架和内存管理

## 许可证

Apache-2.0

## 版本历史

- **v1.0.0**：初始版本
  - 基本的鼓点检测功能
  - 支持单声道和双声道
  - 回调机制
  - PSRAM 支持

