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
   - 能量突变检测：当前能量 / 前一帧能量 ≥ 阈值（默认 6.0，可通过 Kconfig 配置）
   - 平均比值检测：平均能量比值 > 5.0（可通过 Kconfig 配置）
   - 能量阈值：当前低音能量 > 0.01（可通过 Kconfig 配置）
   - 时间间隔：距离上次检测 > 100ms（可通过 Kconfig 配置，防止重复检测）

## API 文档

### 数据结构

#### `beat_detection_cfg_t`

配置结构体，用于初始化 beat detection 组件。配置项已按功能分类为子结构体。

```c
typedef struct {
    struct {
        int16_t                sample_rate;                // 采样率（Hz），默认 16000
        uint8_t                channel;                    // 声道数：1=单声道，2=双声道
        int16_t                fft_size;                   // FFT 大小（2的幂次），默认 512
        int16_t                bass_freq_start;            // 低音频率起始（Hz），默认 200
        int16_t                bass_freq_end;              // 低音频率结束（Hz），默认 300
    } audio_cfg;
    struct {
        UBaseType_t            priority;                   // 任务优先级，默认 3
        uint32_t               stack_size;                 // 任务栈大小（字节），默认 5120
        BaseType_t             core_id;                    // 任务绑定的 CPU 核心，默认 0
    } task_cfg;
    beat_detection_result_callback_t result_callback;      // 结果回调函数
    void*                            result_callback_ctx;  // 回调函数上下文
    struct {
        bool enable_psram : 1;                             // 是否使用 PSRAM，默认 false
    } flags;
} beat_detection_cfg_t;
```

#### `beat_detection_result_t`

检测结果枚举。

```c
typedef enum {
    BEAT_NOT_DETECTED = 0,        // 未检测到鼓点
    BEAT_DETECTED = 1,            // 检测到鼓点
    BEAT_DETECTION_FAILED = 2,    // 检测失败
} beat_detection_result_t;
```

#### `beat_detection_audio_buffer_t`

音频缓冲区结构体。

```c
typedef struct {
    uint8_t *audio_buffer;        // 音频数据缓冲区（字节数组）
    size_t bytes_size;            // 缓冲区大小（字节数）
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

#### `beat_detection_data_write()`

向 beat detection 模块写入音频数据。

```c
esp_err_t beat_detection_data_write(beat_detection_handle_t handle, beat_detection_audio_buffer_t buffer);
```

**参数：**
- `handle`: beat detection 句柄
- `buffer`: 音频缓冲区结构体

**返回值：**
- `ESP_OK`: 音频数据写入成功
- `ESP_ERR_INVALID_ARG`: 参数无效
- `ESP_ERR_INVALID_STATE`: 组件正在计算中，无法接收新数据
- `ESP_ERR_NO_MEM`: 内存分配失败
- `ESP_FAIL`: 队列已满，数据写入失败

**注意：**
- 函数会将音频数据复制到内部队列，由独立任务异步处理
- 如果队列已满或组件正在计算，函数会返回错误
- 音频数据格式必须是 16 位 PCM

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
- 声道数：2（双声道）
- FFT 大小：512
- 低音频率范围：200-300 Hz
- 任务优先级：3
- 任务栈大小：5120 字节
- CPU 核心：0
- PSRAM：禁用

### Kconfig 配置

组件支持通过 ESP-IDF 的 menuconfig 系统配置检测参数。运行 `idf.py menuconfig`，进入 **Component config** → **Beat Detection Configuration** 可以配置以下参数：

- **BEAT_DETECTION_THRESHOLD**：低音能量突变阈值（默认 60，实际值为 6.0）
- **BEAT_DETECTION_AVERAGE_RATIO**：平均能量比值阈值（默认 50，实际值为 5.0）
- **BEAT_DETECTION_MIN_ENERGY**：最小能量阈值（默认 10，实际值为 0.01）
- **BEAT_DETECTION_TIME_INTERVAL**：两次检测之间的最小时间间隔（默认 100 ms）

这些参数在编译时确定，无需在运行时修改。

### 自定义配置示例

```c
beat_detection_cfg_t cfg = {
    .audio_cfg = {
        .sample_rate = 16000,
        .channel = 2,                    // 双声道
        .fft_size = 512,
        .bass_freq_start = 200,
        .bass_freq_end = 300,
    },
    .task_cfg = {
        .priority = 3,
        .stack_size = 5120,
        .core_id = 0,
    },
    .result_callback = my_callback,
    .result_callback_ctx = my_ctx,
    .flags = {
        .enable_psram = true,        // 使用 PSRAM
    }
};
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

### 能量突变阈值（Kconfig: BEAT_DETECTION_THRESHOLD）

- **6.0**：默认值，适合大多数音乐
- **4.0-5.0**：更敏感，可能产生更多误检
- **7.0-8.0**：更保守，可能漏检一些弱鼓点

可通过 `idf.py menuconfig` 在 **Component config** → **Beat Detection Configuration** 中配置。

### 平均能量比值（Kconfig: BEAT_DETECTION_AVERAGE_RATIO）

- **5.0**：默认值，适合大多数场景
- **3.0-4.0**：更敏感
- **6.0-8.0**：更保守

### 最小能量阈值（Kconfig: BEAT_DETECTION_MIN_ENERGY）

- **0.01**：默认值，过滤噪声
- **0.005**：更敏感，可能检测到噪声
- **0.02-0.05**：更保守，只检测强信号

### 时间间隔（Kconfig: BEAT_DETECTION_TIME_INTERVAL）

- **100 ms**：默认值，适合大多数音乐
- **50-80 ms**：更快的响应，可能重复检测
- **150-200 ms**：更保守，避免重复检测

### 任务配置

- **优先级**：默认 3，建议设置为 3-10，确保及时处理音频数据
- **栈大小**：默认 5120 字节，如果出现栈溢出，可以增加到 8192 或更大
- **CPU 核心**：默认 0，可以绑定到特定核心，避免与其他任务竞争

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
   - 音频数据通过 `beat_detection_data_write()` 函数输入
   - 数据会被复制到内部队列，由独立任务处理
   - 如果队列满了或组件正在计算，函数会返回错误，新的数据会被丢弃

4. **线程安全**
   - `beat_detection_data_write()` 函数可以在任何线程中调用
   - 检测结果通过回调函数返回，回调在检测任务中执行
   - 回调函数应尽量简短，避免阻塞

5. **性能考虑**
   - FFT 计算需要一定的 CPU 资源
   - 建议在专用的 CPU 核心上运行检测任务
   - 如果处理速度跟不上，可以增加任务优先级或减少 FFT 大小

6. **初始化检查**
   - 初始化失败时，`*handle` 会被设置为 `NULL`
   - 使用前应检查句柄是否为 `NULL`
   - 在调用 `beat_detection_data_write()` 前确保组件已正确初始化

7. **Kconfig 配置**
   - 检测参数（阈值、比值、能量、时间间隔）可通过 menuconfig 配置
   - 修改配置后需要重新编译项目
   - 配置值在编译时确定，运行时无法修改

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

- **v1.1.0**：重构版本
  - 重构配置结构体，按功能分类（audio_cfg, task_cfg）
  - 新增 `beat_detection_data_write()` API，替代内部回调机制
  - 添加 Kconfig 支持，可通过 menuconfig 配置检测参数
  - 优化内部结构，提高代码可维护性

