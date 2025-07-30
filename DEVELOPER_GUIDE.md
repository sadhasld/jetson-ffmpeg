# NVMPI Developer Guide

## Table of Contents
1. [Advanced Usage Patterns](#advanced-usage-patterns)
2. [Memory Management](#memory-management)
3. [Performance Optimization](#performance-optimization)
4. [Threading and Concurrency](#threading-and-concurrency)
5. [Hardware Scaling](#hardware-scaling)
6. [Custom Integration](#custom-integration)
7. [Debugging and Profiling](#debugging-and-profiling)
8. [Common Pitfalls](#common-pitfalls)
9. [FFmpeg Integration Details](#ffmpeg-integration-details)
10. [Building Custom Applications](#building-custom-applications)

## Advanced Usage Patterns

### Asynchronous Processing

The NVMPI library supports asynchronous processing patterns for maximum throughput:

```c
#include <nvmpi.h>
#include <pthread.h>
#include <queue>
#include <mutex>

struct AsyncDecoder {
    nvmpictx* decoder;
    std::queue<nvPacket> input_queue;
    std::queue<nvFrame> output_queue;
    std::mutex input_mutex;
    std::mutex output_mutex;
    pthread_t decode_thread;
    bool running;
};

void* decode_thread_func(void* arg) {
    AsyncDecoder* async_dec = (AsyncDecoder*)arg;
    
    while (async_dec->running) {
        // Get input packet
        nvPacket packet;
        {
            std::lock_guard<std::mutex> lock(async_dec->input_mutex);
            if (async_dec->input_queue.empty()) {
                usleep(1000); // 1ms wait
                continue;
            }
            packet = async_dec->input_queue.front();
            async_dec->input_queue.pop();
        }
        
        // Decode packet
        if (nvmpi_decoder_put_packet(async_dec->decoder, &packet) >= 0) {
            nvFrame frame;
            while (nvmpi_decoder_get_frame(async_dec->decoder, &frame, false) > 0) {
                std::lock_guard<std::mutex> lock(async_dec->output_mutex);
                async_dec->output_queue.push(frame);
            }
        }
    }
    return nullptr;
}

// Usage example
AsyncDecoder async_decoder;
// Initialize async decoder...
pthread_create(&async_decoder.decode_thread, nullptr, decode_thread_func, &async_decoder);
```

### Pipeline Processing

For maximum throughput, implement a pipeline that overlaps decode and encode operations:

```c
struct Pipeline {
    nvmpictx* decoder;
    nvmpictx* encoder;
    pthread_t decode_thread;
    pthread_t encode_thread;
    // Shared frame buffer between decode and encode
    NVMPI_bufPool<nvFrame*> frame_pool;
    bool running;
};

void* pipeline_decode_thread(void* arg) {
    Pipeline* pipeline = (Pipeline*)arg;
    
    while (pipeline->running) {
        // Decode frames and put them in shared pool
        nvFrame* frame = allocate_frame();
        if (nvmpi_decoder_get_frame(pipeline->decoder, frame, true) > 0) {
            pipeline->frame_pool.qFilledBuf(frame);
        }
    }
    return nullptr;
}

void* pipeline_encode_thread(void* arg) {
    Pipeline* pipeline = (Pipeline*)arg;
    
    while (pipeline->running) {
        // Get frames from shared pool and encode
        nvFrame* frame = pipeline->frame_pool.dqFilledBuf();
        if (frame) {
            nvmpi_encoder_put_frame(pipeline->encoder, frame);
            
            nvPacket* packet;
            while (nvmpi_encoder_get_packet(pipeline->encoder, &packet) > 0) {
                // Process encoded packet
                process_encoded_packet(packet);
                nvmpi_encoder_qEmptyPacket(pipeline->encoder, packet);
            }
            
            // Return frame to pool
            pipeline->frame_pool.qEmptyBuf(frame);
        }
    }
    return nullptr;
}
```

## Memory Management

### DMA Buffer Management

The library uses DMA buffers for zero-copy operations. Understanding buffer lifecycle is crucial:

```c
// Buffer allocation example
NvBufferCreateParams create_params = {
    .width = 1920,
    .height = 1080,
    .layout = NvBufferLayout_Pitch,
    .colorFormat = NvBufferColorFormat_NV12,
    .nvbuf_tag = NvBufferTag_VIDEO_DEC,
    .payloadType = NvBufferPayload_SurfArray,
    .memspace = NvBufferMemSpace_SurfaceArray
};

NVMPI_frameBuf frame_buf;
if (!frame_buf.alloc(create_params)) {
    fprintf(stderr, "Failed to allocate DMA buffer\n");
    return -1;
}

// Use the buffer...

// Always destroy when done
frame_buf.destroy();
```

### Memory Pool Optimization

For high-performance applications, pre-allocate buffer pools:

```c
class FrameBufferPool {
private:
    std::vector<NVMPI_frameBuf*> buffers;
    std::queue<NVMPI_frameBuf*> available;
    std::mutex pool_mutex;
    
public:
    bool initialize(int pool_size, const NvBufferCreateParams& params) {
        for (int i = 0; i < pool_size; i++) {
            NVMPI_frameBuf* buf = new NVMPI_frameBuf();
            if (!buf->alloc(params)) {
                return false;
            }
            buffers.push_back(buf);
            available.push(buf);
        }
        return true;
    }
    
    NVMPI_frameBuf* acquire() {
        std::lock_guard<std::mutex> lock(pool_mutex);
        if (available.empty()) return nullptr;
        
        NVMPI_frameBuf* buf = available.front();
        available.pop();
        return buf;
    }
    
    void release(NVMPI_frameBuf* buf) {
        std::lock_guard<std::mutex> lock(pool_mutex);
        available.push(buf);
    }
    
    ~FrameBufferPool() {
        for (auto buf : buffers) {
            buf->destroy();
            delete buf;
        }
    }
};
```

## Performance Optimization

### Buffer Pool Sizing

Optimal buffer pool sizes depend on your use case:

```c
// For low-latency applications
nvDecParam low_latency_params = {
    .frame_pool_size = 4,  // Minimum for smooth playback
    .codingType = NV_VIDEO_CodingH264,
    .pixFormat = NV_PIX_NV12,
    .resized = {0, 0}
};

// For high-throughput applications
nvDecParam high_throughput_params = {
    .frame_pool_size = 16, // Larger pool for better throughput
    .codingType = NV_VIDEO_CodingH264,
    .pixFormat = NV_PIX_NV12,
    .resized = {0, 0}
};
```

### Hardware Preset Optimization

Choose appropriate hardware presets for your encoder:

```c
nvEncParam performance_params = {
    .width = 1920,
    .height = 1080,
    .bitrate = 5000000,
    .hw_preset_type = V4L2_ENC_HW_PRESET_ULTRAFAST, // Maximum speed
    .codingType = NV_VIDEO_CodingH264
};

nvEncParam quality_params = {
    .width = 1920,
    .height = 1080,
    .bitrate = 5000000,
    .hw_preset_type = V4L2_ENC_HW_PRESET_SLOW, // Better quality
    .codingType = NV_VIDEO_CodingH264
};
```

### CPU Affinity and Scheduling

For real-time applications, consider CPU affinity:

```c
#include <sched.h>

void set_thread_affinity(pthread_t thread, int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    
    int result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        fprintf(stderr, "Failed to set thread affinity\n");
    }
}

// Set real-time scheduling
void set_realtime_priority(pthread_t thread) {
    struct sched_param param;
    param.sched_priority = 80; // High priority
    
    int result = pthread_setschedparam(thread, SCHED_FIFO, &param);
    if (result != 0) {
        fprintf(stderr, "Failed to set real-time priority\n");
    }
}
```

## Threading and Concurrency

### Thread-Safe Operations

The NVMPI library is designed with thread safety in mind, but proper usage is important:

```c
// Safe: Multiple threads can use separate contexts
nvmpictx* decoder1 = nvmpi_create_decoder(&params);
nvmpictx* decoder2 = nvmpi_create_decoder(&params);

// Each thread uses its own context
void* thread1_func(void* arg) {
    // Use decoder1
    return nullptr;
}

void* thread2_func(void* arg) {
    // Use decoder2
    return nullptr;
}

// Unsafe: Multiple threads sharing the same context
// Don't do this without external synchronization
```

### Producer-Consumer Pattern

Implement efficient producer-consumer patterns:

```c
#include <condition_variable>

class ThreadSafeQueue {
private:
    std::queue<nvFrame> queue;
    std::mutex mutex;
    std::condition_variable condition;
    
public:
    void push(const nvFrame& frame) {
        std::unique_lock<std::mutex> lock(mutex);
        queue.push(frame);
        condition.notify_one();
    }
    
    bool pop(nvFrame& frame, int timeout_ms = -1) {
        std::unique_lock<std::mutex> lock(mutex);
        
        if (timeout_ms < 0) {
            condition.wait(lock, [this] { return !queue.empty(); });
        } else {
            if (!condition.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                                   [this] { return !queue.empty(); })) {
                return false; // Timeout
            }
        }
        
        frame = queue.front();
        queue.pop();
        return true;
    }
};
```

## Hardware Scaling

### Dynamic Resolution Changes

Handle resolution changes efficiently:

```c
struct AdaptiveDecoder {
    nvmpictx* decoder;
    nvSize current_output_size;
    nvSize max_input_size;
    bool needs_recreate;
};

bool check_and_update_resolution(AdaptiveDecoder* dec, nvSize new_size) {
    // Check if we need to recreate decoder for new resolution
    if (new_size.width > dec->max_input_size.width ||
        new_size.height > dec->max_input_size.height) {
        
        // Close current decoder
        nvmpi_decoder_close(dec->decoder);
        
        // Create new decoder with larger capacity
        nvDecParam new_params = {
            .frame_pool_size = 8,
            .codingType = NV_VIDEO_CodingH264,
            .pixFormat = NV_PIX_NV12,
            .resized = new_size
        };
        
        dec->decoder = nvmpi_create_decoder(&new_params);
        dec->max_input_size = new_size;
        dec->current_output_size = new_size;
        
        return dec->decoder != nullptr;
    }
    
    return true;
}
```

### Multi-Resolution Encoding

Support multiple output resolutions simultaneously:

```c
struct MultiResEncoder {
    struct Resolution {
        nvmpictx* encoder;
        nvSize size;
        unsigned int bitrate;
    };
    
    std::vector<Resolution> resolutions;
};

bool init_multi_res_encoder(MultiResEncoder* encoder) {
    // 1080p high quality
    nvEncParam params_1080p = {
        .width = 1920, .height = 1080,
        .bitrate = 8000000,
        .codingType = NV_VIDEO_CodingH264
    };
    
    // 720p medium quality
    nvEncParam params_720p = {
        .width = 1280, .height = 720,
        .bitrate = 4000000,
        .codingType = NV_VIDEO_CodingH264
    };
    
    // 480p low quality
    nvEncParam params_480p = {
        .width = 854, .height = 480,
        .bitrate = 2000000,
        .codingType = NV_VIDEO_CodingH264
    };
    
    encoder->resolutions.push_back({{nvmpi_create_encoder(&params_1080p), {1920, 1080}, 8000000}});
    encoder->resolutions.push_back({{nvmpi_create_encoder(&params_720p), {1280, 720}, 4000000}});
    encoder->resolutions.push_back({{nvmpi_create_encoder(&params_480p), {854, 480}, 2000000}});
    
    return true;
}
```

## Custom Integration

### Custom Memory Allocators

Implement custom memory allocators for specific use cases:

```c
// Custom allocator for aligned memory
void* aligned_alloc_custom(size_t alignment, size_t size) {
    void* ptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return nullptr;
    }
    return ptr;
}

// Custom packet allocator
nvPacket* create_custom_packet(size_t payload_size) {
    nvPacket* packet = (nvPacket*)malloc(sizeof(nvPacket));
    if (!packet) return nullptr;
    
    // Allocate aligned payload for DMA operations
    packet->payload = (unsigned char*)aligned_alloc_custom(256, payload_size);
    if (!packet->payload) {
        free(packet);
        return nullptr;
    }
    
    packet->payload_size = payload_size;
    packet->flags = 0;
    packet->pts = 0;
    packet->privData = nullptr;
    
    return packet;
}

void destroy_custom_packet(nvPacket* packet) {
    if (packet) {
        free(packet->payload);
        free(packet);
    }
}
```

### Hardware Feature Detection

Detect available hardware features at runtime:

```c
#include <sys/utsname.h>

enum JetsonPlatform {
    JETSON_UNKNOWN,
    JETSON_NANO,
    JETSON_TX2,
    JETSON_XAVIER_NX,
    JETSON_AGX_XAVIER,
    JETSON_AGX_ORIN,
    JETSON_ORIN_NX,
    JETSON_ORIN_NANO
};

JetsonPlatform detect_jetson_platform() {
    struct utsname sys_info;
    if (uname(&sys_info) != 0) {
        return JETSON_UNKNOWN;
    }
    
    // Read device tree model
    FILE* model_file = fopen("/proc/device-tree/model", "r");
    if (!model_file) {
        return JETSON_UNKNOWN;
    }
    
    char model[256];
    if (fgets(model, sizeof(model), model_file) == nullptr) {
        fclose(model_file);
        return JETSON_UNKNOWN;
    }
    fclose(model_file);
    
    if (strstr(model, "Jetson Nano")) return JETSON_NANO;
    if (strstr(model, "Jetson TX2")) return JETSON_TX2;
    if (strstr(model, "Jetson Xavier NX")) return JETSON_XAVIER_NX;
    if (strstr(model, "Jetson AGX Xavier")) return JETSON_AGX_XAVIER;
    if (strstr(model, "Jetson AGX Orin")) return JETSON_AGX_ORIN;
    if (strstr(model, "Jetson Orin NX")) return JETSON_ORIN_NX;
    if (strstr(model, "Jetson Orin Nano")) return JETSON_ORIN_NANO;
    
    return JETSON_UNKNOWN;
}

bool get_platform_capabilities(JetsonPlatform platform, 
                              bool* supports_hevc_encode,
                              bool* supports_vp9_decode,
                              int* max_decode_instances) {
    switch (platform) {
        case JETSON_NANO:
            *supports_hevc_encode = false;
            *supports_vp9_decode = true;
            *max_decode_instances = 2;
            return true;
            
        case JETSON_AGX_XAVIER:
            *supports_hevc_encode = true;
            *supports_vp9_decode = true;
            *max_decode_instances = 4;
            return true;
            
        case JETSON_AGX_ORIN:
            *supports_hevc_encode = true;
            *supports_vp9_decode = true;
            *max_decode_instances = 8;
            return true;
            
        default:
            return false;
    }
}
```

## Debugging and Profiling

### Debug Logging

Enable detailed logging for troubleshooting:

```c
#include <stdarg.h>

enum LogLevel {
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
};

static LogLevel current_log_level = LOG_INFO;

void nvmpi_log(LogLevel level, const char* format, ...) {
    if (level > current_log_level) return;
    
    const char* level_str[] = {"ERROR", "WARN", "INFO", "DEBUG"};
    
    printf("[NVMPI %s] ", level_str[level]);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
}

// Usage in your code
void debug_decoder_state(nvmpictx* ctx) {
    nvmpi_log(LOG_DEBUG, "Decoder state: ctx=%p", ctx);
    // Add more debug information as needed
}
```

### Performance Profiling

Implement performance profiling:

```c
#include <chrono>

class PerformanceProfiler {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    
public:
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    double stop_and_get_ms() {
        end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time);
        return duration.count() / 1000.0; // Convert to milliseconds
    }
};

// Usage example
PerformanceProfiler profiler;

profiler.start();
int ret = nvmpi_decoder_put_packet(decoder, &packet);
double decode_time = profiler.stop_and_get_ms();

nvmpi_log(LOG_DEBUG, "Decode time: %.2f ms", decode_time);
```

### Memory Usage Monitoring

Monitor memory usage:

```c
#include <sys/resource.h>

void print_memory_usage() {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        printf("Peak memory usage: %ld KB\n", usage.ru_maxrss);
    }
    
    // Read current memory usage from /proc/self/status
    FILE* status = fopen("/proc/self/status", "r");
    if (status) {
        char line[256];
        while (fgets(line, sizeof(line), status)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                printf("Current memory usage: %s", line + 6);
                break;
            }
        }
        fclose(status);
    }
}
```

## Common Pitfalls

### 1. Buffer Pool Exhaustion

```c
// Wrong: Not checking for buffer availability
nvPacket* packet;
nvmpi_encoder_dqEmptyPacket(encoder, &packet); // May return null
packet->payload_size = data_size; // Crash if packet is null

// Correct: Always check return values
nvPacket* packet;
if (nvmpi_encoder_dqEmptyPacket(encoder, &packet) == 0 && packet) {
    packet->payload_size = data_size;
    // ... use packet
    nvmpi_encoder_qEmptyPacket(encoder, packet);
} else {
    // Handle buffer exhaustion
    nvmpi_log(LOG_WARNING, "Encoder buffer pool exhausted");
}
```

### 2. Incorrect Frame Format

```c
// Wrong: Assuming frame format
nvFrame frame;
nvmpi_decoder_get_frame(decoder, &frame, true);
// Assuming NV12 format without checking
process_nv12_frame(&frame);

// Correct: Check frame format
nvFrame frame;
if (nvmpi_decoder_get_frame(decoder, &frame, true) > 0) {
    switch (frame.type) {
        case NV_PIX_NV12:
            process_nv12_frame(&frame);
            break;
        case NV_PIX_YUV420:
            process_yuv420_frame(&frame);
            break;
        default:
            nvmpi_log(LOG_ERROR, "Unsupported frame format: %d", frame.type);
    }
}
```

### 3. Resource Leaks

```c
// Wrong: Not cleaning up resources
nvmpictx* decoder = nvmpi_create_decoder(&params);
// ... use decoder
// Missing: nvmpi_decoder_close(decoder);

// Correct: Always clean up
nvmpictx* decoder = nvmpi_create_decoder(&params);
if (!decoder) {
    return -1;
}

// Use decoder...

// Always clean up
int ret = nvmpi_decoder_close(decoder);
if (ret < 0) {
    nvmpi_log(LOG_ERROR, "Failed to close decoder");
}
```

## FFmpeg Integration Details

### Custom AVCodec Implementation

Understanding the FFmpeg integration layer:

```c
// From ffmpeg_dev/common/libavcodec/nvmpi_dec.c
typedef struct {
    char eos_reached;
    nvmpictx* ctx;
    AVClass *av_class;
    AVFrame *bufFrame;
    char *resize_expr;
    int frame_pool_size;
} nvmpiDecodeContext;

// The integration maps AVCodecContext to nvmpictx
static int nvmpi_init_decoder(AVCodecContext *avctx) {
    nvmpiDecodeContext *ctx = avctx->priv_data;
    
    nvDecParam params = {
        .frame_pool_size = ctx->frame_pool_size,
        .codingType = nvmpi_get_codingtype(avctx),
        .pixFormat = NV_PIX_NV12,
        .resized = {0, 0}
    };
    
    // Parse resize option if provided
    if (ctx->resize_expr && strlen(ctx->resize_expr) > 0) {
        sscanf(ctx->resize_expr, "%dx%d", 
               &params.resized.width, &params.resized.height);
    }
    
    ctx->ctx = nvmpi_create_decoder(&params);
    return ctx->ctx ? 0 : AVERROR(ENOMEM);
}
```

### Custom Options in FFmpeg

Add custom options to your FFmpeg integration:

```c
// Define custom options
static const AVOption nvmpi_decode_options[] = {
    {"frame_pool_size", "Frame pool size", 
     offsetof(nvmpiDecodeContext, frame_pool_size), 
     AV_OPT_TYPE_INT, {.i64 = 5}, 1, 32, 
     AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM},
     
    {"resize", "Resize dimensions (WxH)", 
     offsetof(nvmpiDecodeContext, resize_expr), 
     AV_OPT_TYPE_STRING, {.str = NULL}, 0, 0,
     AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM},
     
    {NULL}
};

// Use in FFmpeg command line:
// ffmpeg -c:v h264_nvmpi -frame_pool_size 8 -resize 1280x720 -i input.mp4 ...
```

## Building Custom Applications

### CMake Integration

Integrate NVMPI into your CMake project:

```cmake
# FindNVMPI.cmake
find_path(NVMPI_INCLUDE_DIR
    NAMES nvmpi.h
    PATHS /usr/local/include /usr/include
)

find_library(NVMPI_LIBRARY
    NAMES nvmpi
    PATHS /usr/local/lib /usr/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NVMPI
    REQUIRED_VARS NVMPI_LIBRARY NVMPI_INCLUDE_DIR
)

if(NVMPI_FOUND)
    set(NVMPI_LIBRARIES ${NVMPI_LIBRARY})
    set(NVMPI_INCLUDE_DIRS ${NVMPI_INCLUDE_DIR})
endif()

# In your CMakeLists.txt
find_package(NVMPI REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app ${NVMPI_LIBRARIES})
target_include_directories(my_app PRIVATE ${NVMPI_INCLUDE_DIRS})
```

### pkg-config Integration

Use pkg-config for simpler builds:

```bash
# Compile with pkg-config
gcc $(pkg-config --cflags nvmpi) -o my_app main.c $(pkg-config --libs nvmpi)

# In Makefile
CFLAGS += $(shell pkg-config --cflags nvmpi)
LDFLAGS += $(shell pkg-config --libs nvmpi)
```

### Complete Application Template

```c
#include <nvmpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    nvmpictx* decoder;
    nvmpictx* encoder;
    FILE* input_file;
    FILE* output_file;
    bool running;
} AppContext;

int initialize_app(AppContext* app, const char* input_path, const char* output_path) {
    // Open files
    app->input_file = fopen(input_path, "rb");
    app->output_file = fopen(output_path, "wb");
    
    if (!app->input_file || !app->output_file) {
        fprintf(stderr, "Failed to open files\n");
        return -1;
    }
    
    // Create decoder
    nvDecParam dec_params = {
        .frame_pool_size = 8,
        .codingType = NV_VIDEO_CodingH264,
        .pixFormat = NV_PIX_NV12,
        .resized = {0, 0}
    };
    
    app->decoder = nvmpi_create_decoder(&dec_params);
    if (!app->decoder) {
        fprintf(stderr, "Failed to create decoder\n");
        return -1;
    }
    
    // Create encoder
    nvEncParam enc_params = {
        .width = 1920,
        .height = 1080,
        .bitrate = 5000000,
        .fps_n = 30,
        .fps_d = 1,
        .codingType = NV_VIDEO_CodingH264,
        .profile = 100,
        .level = 51
    };
    
    app->encoder = nvmpi_create_encoder(&enc_params);
    if (!app->encoder) {
        fprintf(stderr, "Failed to create encoder\n");
        nvmpi_decoder_close(app->decoder);
        return -1;
    }
    
    app->running = true;
    return 0;
}

int process_video(AppContext* app) {
    unsigned char buffer[64*1024];
    
    while (app->running) {
        // Read input data
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), app->input_file);
        if (bytes_read == 0) break;
        
        // Create packet
        nvPacket packet = {
            .payload = buffer,
            .payload_size = bytes_read,
            .pts = 0,
            .flags = 0
        };
        
        // Decode
        if (nvmpi_decoder_put_packet(app->decoder, &packet) >= 0) {
            nvFrame frame;
            while (nvmpi_decoder_get_frame(app->decoder, &frame, false) > 0) {
                // Encode
                if (nvmpi_encoder_put_frame(app->encoder, &frame) >= 0) {
                    nvPacket* enc_packet;
                    while (nvmpi_encoder_get_packet(app->encoder, &enc_packet) > 0) {
                        // Write output
                        fwrite(enc_packet->payload, 1, enc_packet->payload_size, 
                               app->output_file);
                        nvmpi_encoder_qEmptyPacket(app->encoder, enc_packet);
                    }
                }
            }
        }
    }
    
    return 0;
}

void cleanup_app(AppContext* app) {
    if (app->decoder) nvmpi_decoder_close(app->decoder);
    if (app->encoder) nvmpi_encoder_close(app->encoder);
    if (app->input_file) fclose(app->input_file);
    if (app->output_file) fclose(app->output_file);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input> <output>\n", argv[0]);
        return 1;
    }
    
    AppContext app = {0};
    
    if (initialize_app(&app, argv[1], argv[2]) < 0) {
        cleanup_app(&app);
        return 1;
    }
    
    int ret = process_video(&app);
    cleanup_app(&app);
    
    return ret;
}
```

---

*This developer guide provides advanced usage patterns and best practices for the NVMPI library. For basic usage, refer to the main API documentation.*