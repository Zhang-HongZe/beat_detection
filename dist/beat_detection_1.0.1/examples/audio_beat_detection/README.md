# Beat Detection 示例

## 概述

本示例展示了如何使用 Beat Detection 组件进行音频鼓点检测。示例通过生成模拟音频数据（250Hz 正弦波）来演示 Beat Detection 的功能，检测到鼓点时会通过串口输出 "audio detected!"。

## 功能特性

- **Beat Detection 初始化**：配置并初始化 Beat Detection 组件
- **模拟音频输入**：生成 250Hz 正弦波作为测试音频（在低音频率范围内）
- **实时检测**：持续处理音频数据并检测鼓点
- **回调通知**：检测到鼓点时通过回调函数打印 "audio detected!"
- **静音测试**：交替发送有信号和静音数据，验证检测逻辑

## 硬件要求

- ESP32 系列开发板（ESP32、ESP32-S2、ESP32-S3 等）
- USB 串口线用于查看输出

**注意**：本示例使用模拟音频数据，不需要实际的 I2S 音频输入设备。如需使用真实 I2S 输入，请参考主项目的实现。

## 配置说明

### 音频参数配置

在 `main.c` 中可以修改以下参数：

```c
#define I2S_SAMPLE_RATE         16000    // 采样率（Hz）
#define I2S_BITS_PER_SAMPLE     16       // 每样本位数
#define I2S_CHANNELS            1        // 声道数（1=单声道，2=双声道）
#define I2S_BUFFER_SIZE         2048     // 缓冲区大小（样本数）
#define FFT_SIZE                512      // FFT 大小
```

### Beat Detection 配置

Beat Detection 的配置在 `beat_detection_init_example()` 函数中：

```c
beat_detection_cfg_t cfg = BEAT_DETECTION_DEFAULT_CFG();
cfg.sample_rate = I2S_SAMPLE_RATE;      // 采样率，必须与音频数据一致
cfg.channel = I2S_CHANNELS;              // 声道数，必须与音频数据一致
cfg.fft_size = FFT_SIZE;                // FFT 大小（建议 512 或 1024）
cfg.bass_freq_start = 200;               // 低音频率起始（Hz）
cfg.bass_freq_end = 300;                 // 低音频率结束（Hz）
cfg.bass_surge_threshold = 6.0f;         // 能量突变阈值（默认 6.0）
cfg.task_priority = 5;                   // 处理任务优先级
cfg.task_stack_size = 8192;              // 任务栈大小（字节）
cfg.task_core_id = 0;                    // 任务运行的 CPU 核心
cfg.result_callback = callback;          // 结果回调函数
cfg.flags.enable_psram = false;         // 是否使用 PSRAM
```

### 参数调优建议

- **FFT 大小**：
  - `512`：默认值，平衡精度和性能
  - `256`：更快处理，但频率分辨率较低
  - `1024`：更高精度，但需要更多内存

- **低音频率范围**：
  - `200-300 Hz`：默认值，适合大多数鼓点
  - `150-250 Hz`：更低的频率，适合低音鼓
  - `250-350 Hz`：稍高的频率，适合某些类型的鼓

- **能量突变阈值**：
  - `6.0`：默认值，适合大多数音乐
  - `4.0-5.0`：更敏感，可能产生更多误检
  - `7.0-8.0`：更保守，可能漏检弱鼓点

## 使用方法

### 1. 编译和烧录

```bash
# 进入示例目录
cd components/beat_detection/examples/audio_beat_detection

# 编译项目
idf.py build

# 烧录到设备
idf.py flash

# 查看串口输出
idf.py monitor
```

或者使用一条命令完成编译、烧录和监控：

```bash
idf.py build flash monitor
```

### 2. 运行示例

示例启动后会：

1. 初始化 Beat Detection 组件
2. 分配音频缓冲区（4096 字节，2048 个样本）
3. 进入主循环：
   - 生成 250Hz 正弦波音频数据
   - 将数据输入到 Beat Detection
   - 等待 500ms
   - 发送静音数据
   - 等待 500ms
   - 重复

### 3. 查看输出

在串口监视器中，你会看到类似以下的输出：

```
I (xxx) BEAT_DETECTION_I2S_EXAMPLE: Beat Detection initialized successfully
I (xxx) BEAT_DETECTION_I2S_EXAMPLE: Audio buffer allocated: 4096 bytes (2048 samples)
I (xxx) BEAT_DETECTION_I2S_EXAMPLE: audio detected!
I (xxx) BEAT_DETECTION_I2S_EXAMPLE: audio not detected!
```

## 代码结构

```
main.c
├── beat_detection_result_callback()    # 检测结果回调函数
│   ├── BEAT_DETECTED: 打印 "audio detected!"
│   └── BEAT_NOT_DETECTED: 打印 "audio not detected!"
├── beat_detection_init_example()       # 初始化 Beat Detection
│   └── 配置并初始化 Beat Detection 组件
└── app_main()                          # 主函数
    ├── 初始化 Beat Detection
    ├── 分配音频缓冲区
    └── 主循环
        ├── 生成 250Hz 正弦波
        ├── 输入到 Beat Detection
        ├── 发送静音数据
        └── 重复
```

## 工作流程

1. **初始化阶段**
   - 配置 Beat Detection 参数（采样率、声道数、FFT 大小等）
   - 初始化 Beat Detection 组件
   - 分配音频缓冲区内存

2. **运行阶段**
   - 生成测试音频数据（250Hz 正弦波）
   - 通过 `feed_audio()` 将数据输入到 Beat Detection
   - Beat Detection 在后台任务中处理音频数据：
     - 应用窗函数
     - 执行 FFT 变换
     - 计算低音频率范围内的能量
     - 检测能量突变
   - 检测到鼓点时调用回调函数

3. **数据处理**
   - Beat Detection 需要至少 `channel * fft_size * sizeof(int16_t)` 字节的数据
   - 对于单声道和 512 的 FFT 大小，需要 1024 字节
   - 本示例使用 4096 字节缓冲区，满足要求

## 集成真实 I2S 输入

如需使用真实的 I2S 音频输入，可以参考以下步骤：

1. **添加 I2S 初始化代码**：
   ```c
   #include "driver/i2s_std.h"
   
   i2s_chan_handle_t i2s_rx_handle;
   // 配置并初始化 I2S...
   ```

2. **读取 I2S 数据**：
   ```c
   size_t bytes_read = 0;
   esp_err_t ret = i2s_channel_read(i2s_rx_handle, 
                                     audio_buffer, 
                                     buffer_size_bytes, 
                                     &bytes_read, 
                                     portMAX_DELAY);
   ```

3. **输入到 Beat Detection**：
   ```c
   beat_detection_audio_buffer_t buffer = {
       .audio_buffer = (uint8_t *)audio_buffer,
       .bytes_size = bytes_read,
   };
   g_beat_detection_handle->feed_audio(buffer, g_beat_detection_handle);
   ```

## 注意事项

1. **内存管理**
   - 确保有足够的内存用于音频缓冲区和 FFT 计算
   - 如果内存不足，可以启用 PSRAM（`cfg.flags.enable_psram = true`）
   - 缓冲区大小应至少为 `channel * fft_size * sizeof(int16_t)` 字节

2. **数据格式**
   - 音频数据必须是 16 位 PCM 格式（int16_t）
   - 采样率必须与配置的 `sample_rate` 一致
   - 声道数必须与配置的 `channel` 一致

3. **性能考虑**
   - FFT 计算需要一定的 CPU 资源
   - 建议在专用的 CPU 核心上运行处理任务
   - 如果处理速度跟不上，可以增加任务优先级或减少 FFT 大小

4. **回调函数**
   - 回调函数在 Beat Detection 的处理任务中执行
   - 回调函数应尽量简短，避免阻塞
   - 不要在回调中执行耗时操作

## 故障排除

### 问题：没有检测到鼓点

- **检查音频数据**：确保音频数据包含低音频率（200-300Hz）的能量
- **调整阈值**：降低 `bass_surge_threshold` 值（例如 4.0 或 5.0）
- **检查配置**：确认采样率和声道数配置正确
- **检查数据大小**：确保输入的数据量满足最小要求

### 问题：内存分配失败

- **启用 PSRAM**：设置 `cfg.flags.enable_psram = true`
- **减小缓冲区**：减小 `I2S_BUFFER_SIZE`
- **减小 FFT 大小**：使用 256 而不是 512

### 问题：编译错误

- **检查组件依赖**：确保 `beat_detection` 组件已正确添加到 `CMakeLists.txt`
- **检查路径**：确认 `EXTRA_COMPONENT_DIRS` 路径正确
- **清理重建**：运行 `idf.py fullclean` 后重新编译

### 问题：StoreProhibited 错误

- **检查缓冲区大小**：确保分配的内存足够大
- **检查数据类型**：确保音频数据是 `int16_t` 格式
- **检查指针**：确保所有指针在使用前已正确初始化

## 扩展功能

可以在此基础上添加以下功能：

- **LED 指示**：检测到鼓点时控制 LED 闪烁
- **UART 输出**：通过 UART 发送检测结果到其他设备
- **数据记录**：记录检测到的鼓点时间戳
- **统计分析**：统计单位时间内的鼓点数量（BPM）
- **多阈值检测**：使用不同的阈值检测不同类型的鼓点
- **可视化**：通过串口输出能量值，用于调试和可视化

## 示例输出说明

- `audio detected!`：检测到鼓点（能量突变满足阈值条件）
- `audio not detected!`：未检测到鼓点（能量变化不满足阈值）

## 相关文档

- [Beat Detection 组件文档](../../README.md)
- [ESP-IDF I2S 驱动文档](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/i2s.html)
- [ESP-IDF FreeRTOS 文档](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/freertos.html)

## 许可证

Apache-2.0
