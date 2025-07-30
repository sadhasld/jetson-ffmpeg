# NVMPI API Documentation

## Table of Contents
1. [Overview](#overview)
2. [Library Architecture](#library-architecture)
3. [Data Types and Structures](#data-types-and-structures)
4. [Decoder API](#decoder-api)
5. [Encoder API](#encoder-api)
6. [Buffer Management](#buffer-management)
7. [FFmpeg Integration](#ffmpeg-integration)
8. [Build Configuration](#build-configuration)
9. [Usage Examples](#usage-examples)
10. [Error Handling](#error-handling)
11. [Platform Support](#platform-support)

## Overview

The NVMPI (NVIDIA Multimedia Processing Interface) library provides hardware-accelerated video encoding and decoding capabilities for NVIDIA Jetson platforms. It serves as a bridge between FFmpeg and the Jetson Multimedia API, enabling efficient video processing using dedicated hardware acceleration units.

### Key Features
- **Hardware-accelerated decoding**: H.264/AVC, H.265/HEVC, MPEG2, MPEG4, VP8, VP9
- **Hardware-accelerated encoding**: H.264/AVC, H.265/HEVC
- **Hardware scaling**: Real-time video scaling during decode operations
- **Zero-copy operations**: Direct memory access using DMA buffers
- **Thread-safe buffer management**: Built-in buffer pooling system
- **FFmpeg integration**: Seamless integration with FFmpeg codec framework

## Library Architecture

The library is structured around several key components:

```
┌─────────────────────────────────────────────────────────────┐
│                     FFmpeg Integration Layer                │
├─────────────────────────────────────────────────────────────┤
│                        NVMPI API Layer                     │
│  ┌─────────────────┐              ┌─────────────────────┐   │
│  │   Decoder API   │              │    Encoder API      │   │
│  └─────────────────┘              └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    Buffer Management                        │
│  ┌─────────────────┐              ┌─────────────────────┐   │
│  │   Frame Buffers │              │  Packet Buffers     │   │
│  └─────────────────┘              └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                  Jetson Multimedia API                     │
└─────────────────────────────────────────────────────────────┘
```

## Data Types and Structures

### Core Context Type

```c
typedef struct nvmpictx nvmpictx;
```
Opaque context structure that maintains the state for encoder/decoder instances.

### Pixel Formats

```c
typedef enum {
    NV_PIX_NV12,     // NV12 format (Y plane + interleaved UV plane)
    NV_PIX_YUV420    // YUV420 planar format
} nvPixFormat;
```

### Codec Types

```c
typedef enum {
    NV_VIDEO_CodingUnused,   // Unspecified codec
    NV_VIDEO_CodingH264,     // H.264/AVC codec
    NV_VIDEO_CodingMPEG4,    // MPEG-4 codec
    NV_VIDEO_CodingMPEG2,    // MPEG-2 codec
    NV_VIDEO_CodingVP8,      // VP8 codec
    NV_VIDEO_CodingVP9,      // VP9 codec
    NV_VIDEO_CodingHEVC,     // H.265/HEVC codec
} nvCodingType;
```

### Size Structure

```c
typedef struct _NVSIZE {
    unsigned int width;   // Frame width in pixels
    unsigned int height;  // Frame height in pixels
} nvSize;
```

### Encoder Parameters

```c
typedef struct _NVENCPARAM {
    unsigned int width;               // Input frame width
    unsigned int height;              // Input frame height
    unsigned int profile;             // Codec profile
    unsigned int level;               // Codec level
    unsigned int bitrate;             // Target bitrate in bits/sec
    unsigned int peak_bitrate;        // Peak bitrate for VBR mode
    char enableLossless;              // Enable lossless encoding
    char mode_vbr;                    // Variable bitrate mode
    char insert_spspps_idr;           // Insert SPS/PPS at IDR frames
    unsigned int iframe_interval;     // I-frame interval
    unsigned int idr_interval;        // IDR frame interval
    unsigned int fps_n;               // Frame rate numerator
    unsigned int fps_d;               // Frame rate denominator
    int capture_num;                  // Number of capture buffers
    unsigned int max_b_frames;        // Maximum B-frames
    unsigned int refs;                // Reference frames count
    unsigned int qmax;                // Maximum quantization parameter
    unsigned int qmin;                // Minimum quantization parameter
    unsigned int hw_preset_type;      // Hardware preset type
    unsigned int vbv_buffer_size;     // VBV buffer size
    nvCodingType codingType;          // Codec type
} nvEncParam;
```

### Decoder Parameters

```c
typedef struct _NVDECPARAM {
    int frame_pool_size;      // Size of frame buffer pool (1-32)
    nvCodingType codingType;  // Input codec type
    nvPixFormat pixFormat;    // Output pixel format
    nvSize resized;           // Target resize dimensions (0,0 = no resize)
} nvDecParam;
```

### Packet Structure

```c
typedef struct _NVPACKET {
    unsigned long flags;         // Packet flags (keyframe, etc.)
    unsigned long payload_size;  // Size of payload data
    unsigned char *payload;      // Pointer to packet data
    unsigned long pts;           // Presentation timestamp
    void* privData;             // Private data (used internally)
} nvPacket;
```

### Frame Structure

```c
typedef struct _NVFRAME {
    unsigned long flags;           // Frame flags
    unsigned long payload_size[3]; // Size of each plane
    unsigned char *payload[3];     // Pointers to plane data
    unsigned int linesize[3];      // Line size for each plane
    nvPixFormat type;              // Pixel format
    unsigned int width;            // Frame width
    unsigned int height;           // Frame height
    time_t timestamp;              // Frame timestamp
} nvFrame;
```

## Decoder API

### Create Decoder

```c
nvmpictx* nvmpi_create_decoder(nvDecParam* param);
```

Creates a new decoder instance with the specified parameters.

**Parameters:**
- `param`: Pointer to decoder parameters structure

**Returns:**
- Pointer to decoder context on success
- `NULL` on failure

**Example:**
```c
nvDecParam dec_param = {
    .frame_pool_size = 8,
    .codingType = NV_VIDEO_CodingH264,
    .pixFormat = NV_PIX_NV12,
    .resized = {0, 0}  // No resizing
};

nvmpictx* decoder = nvmpi_create_decoder(&dec_param);
if (!decoder) {
    fprintf(stderr, "Failed to create decoder\n");
    return -1;
}
```

### Send Packet to Decoder

```c
int nvmpi_decoder_put_packet(nvmpictx* ctx, nvPacket* packet);
```

Sends an encoded packet to the decoder for processing.

**Parameters:**
- `ctx`: Decoder context
- `packet`: Pointer to input packet

**Returns:**
- `0` on success
- Negative value on error

**Example:**
```c
nvPacket packet = {
    .payload = encoded_data,
    .payload_size = data_size,
    .pts = timestamp,
    .flags = 0
};

int ret = nvmpi_decoder_put_packet(decoder, &packet);
if (ret < 0) {
    fprintf(stderr, "Failed to send packet to decoder\n");
}
```

### Receive Frame from Decoder

```c
int nvmpi_decoder_get_frame(nvmpictx* ctx, nvFrame* frame, bool wait);
```

Retrieves a decoded frame from the decoder.

**Parameters:**
- `ctx`: Decoder context
- `frame`: Pointer to output frame structure
- `wait`: Whether to wait for frame availability

**Returns:**
- `1` if frame is available
- `0` if no frame available (non-blocking mode)
- Negative value on error

**Example:**
```c
nvFrame frame;
int ret = nvmpi_decoder_get_frame(decoder, &frame, true);
if (ret > 0) {
    // Process decoded frame
    printf("Decoded frame: %dx%d, format=%d\n", 
           frame.width, frame.height, frame.type);
} else if (ret == 0) {
    // No frame available
} else {
    fprintf(stderr, "Decoder error\n");
}
```

### Close Decoder

```c
int nvmpi_decoder_close(nvmpictx* ctx);
```

Closes the decoder and releases all resources.

**Parameters:**
- `ctx`: Decoder context

**Returns:**
- `0` on success
- Negative value on error

## Encoder API

### Create Encoder

```c
nvmpictx* nvmpi_create_encoder(nvEncParam* param);
```

Creates a new encoder instance with the specified parameters.

**Parameters:**
- `param`: Pointer to encoder parameters structure

**Returns:**
- Pointer to encoder context on success
- `NULL` on failure

**Example:**
```c
nvEncParam enc_param = {
    .width = 1920,
    .height = 1080,
    .profile = 100,  // High profile
    .level = 51,     // Level 5.1
    .bitrate = 5000000,  // 5 Mbps
    .peak_bitrate = 10000000,  // 10 Mbps peak
    .fps_n = 30,
    .fps_d = 1,
    .iframe_interval = 30,
    .codingType = NV_VIDEO_CodingH264,
    .mode_vbr = 1,
    .qmin = 20,
    .qmax = 40
};

nvmpictx* encoder = nvmpi_create_encoder(&enc_param);
if (!encoder) {
    fprintf(stderr, "Failed to create encoder\n");
    return -1;
}
```

### Send Frame to Encoder

```c
int nvmpi_encoder_put_frame(nvmpictx* ctx, nvFrame* frame);
```

Sends a raw frame to the encoder for encoding.

**Parameters:**
- `ctx`: Encoder context
- `frame`: Pointer to input frame

**Returns:**
- `0` on success
- Negative value on error

**Example:**
```c
nvFrame frame = {
    .width = 1920,
    .height = 1080,
    .type = NV_PIX_NV12,
    .payload = {y_plane, uv_plane, NULL},
    .payload_size = {y_size, uv_size, 0},
    .linesize = {1920, 1920, 0},
    .timestamp = current_time
};

int ret = nvmpi_encoder_put_frame(encoder, &frame);
if (ret < 0) {
    fprintf(stderr, "Failed to send frame to encoder\n");
}
```

### Receive Packet from Encoder

```c
int nvmpi_encoder_get_packet(nvmpictx* ctx, nvPacket** packet);
```

Retrieves an encoded packet from the encoder.

**Parameters:**
- `ctx`: Encoder context
- `packet`: Pointer to output packet pointer

**Returns:**
- `1` if packet is available
- `0` if no packet available
- Negative value on error

**Example:**
```c
nvPacket* packet;
int ret = nvmpi_encoder_get_packet(encoder, &packet);
if (ret > 0) {
    // Process encoded packet
    printf("Encoded packet: size=%lu, pts=%lu, keyframe=%s\n",
           packet->payload_size, packet->pts,
           (packet->flags & 1) ? "yes" : "no");
    
    // Return packet to pool when done
    nvmpi_encoder_qEmptyPacket(encoder, packet);
}
```

### Packet Pool Management

```c
int nvmpi_encoder_dqEmptyPacket(nvmpictx* ctx, nvPacket** packet);
void nvmpi_encoder_qEmptyPacket(nvmpictx* ctx, nvPacket* packet);
```

Manages encoder packet buffer pool.

**dqEmptyPacket Parameters:**
- `ctx`: Encoder context
- `packet`: Pointer to output packet pointer

**qEmptyPacket Parameters:**
- `ctx`: Encoder context
- `packet`: Packet to return to pool

### Close Encoder

```c
int nvmpi_encoder_close(nvmpictx* ctx);
```

Closes the encoder and releases all resources.

## Buffer Management

### NVMPI_bufPool Template Class

The library uses a thread-safe buffer pool template for managing frame and packet buffers:

```cpp
template <typename T>
struct NVMPI_bufPool {
    T dqEmptyBuf();        // Dequeue empty buffer
    void qEmptyBuf(T buf); // Queue empty buffer
    T peekEmptyBuf();      // Peek at empty buffer without dequeuing
    T dqFilledBuf();       // Dequeue filled buffer
    void qFilledBuf(T buf); // Queue filled buffer
};
```

### NVMPI_frameBuf Structure

```cpp
struct NVMPI_frameBuf {
    NvBufSurface *dst_dma_surface;  // DMA surface (NvUtils API)
    int dst_dma_fd;                 // DMA file descriptor
    unsigned long long timestamp;    // Frame timestamp
    
    bool alloc(NvBufferCreateParams& params);  // Allocate DMA buffer
    bool destroy();                            // Destroy DMA buffer
};
```

## FFmpeg Integration

### Codec Names

The library integrates with FFmpeg using these codec names:

**Decoders:**
- `h264_nvmpi` - H.264/AVC hardware decoder
- `hevc_nvmpi` - H.265/HEVC hardware decoder
- `mpeg2_nvmpi` - MPEG-2 hardware decoder
- `mpeg4_nvmpi` - MPEG-4 hardware decoder
- `vp8_nvmpi` - VP8 hardware decoder
- `vp9_nvmpi` - VP9 hardware decoder

**Encoders:**
- `h264_nvmpi` - H.264/AVC hardware encoder
- `hevc_nvmpi` - H.265/HEVC hardware encoder

### FFmpeg Options

**Decoder Options:**
- `frame_pool_size`: Size of frame buffer pool (1-32, default: 5)
- `resize`: Resize dimensions in WxH format for hardware scaling

**Encoder Options:**
- Standard FFmpeg encoding options are supported
- Hardware-specific options are automatically configured

### Integration Architecture

```
FFmpeg AVCodec Layer
├── nvmpi_dec.c (FFmpeg decoder wrapper)
├── nvmpi_enc.c (FFmpeg encoder wrapper)
└── NVMPI Library
    ├── nvmpi_create_decoder/encoder
    ├── nvmpi_*_put_*/get_*
    └── Jetson Multimedia API
```

## Build Configuration

### CMake Options

```bash
cmake [OPTIONS] ..
```

**Available Options:**
- `-DJETSON_MULTIMEDIA_API_DIR=<path>`: Custom Jetson Multimedia API directory (default: `/usr/src/jetson_multimedia_api`)
- `-DJETSON_MULTIMEDIA_LIB_DIR=<path>`: Custom Jetson Multimedia libraries directory (default: `/usr/lib/aarch64-linux-gnu/tegra`)
- `-DCUDA_INCLUDE_DIR=<path>`: Custom CUDA headers directory (default: `/usr/local/cuda/include`)
- `-DCUDA_LIB_DIR=<path>`: Custom CUDA libraries directory (default: `/usr/local/cuda/lib64`)
- `-DWITH_STUBS=[ON/OFF]`: Build with stubs instead of original libraries (default: OFF)

### Dependencies

**Required Libraries:**
- `libnvv4l2` - V4L2 extensions for NVIDIA
- `libnvjpeg` - NVIDIA JPEG library
- `pthread` - POSIX threads

**Conditional Dependencies:**
- `libnvbufsurface` - NvUtils API (preferred)
- `libnvbufsurftransform` - NvUtils transform API
- `libnvbuf_utils` - Legacy buffer utilities (fallback)

### pkg-config Integration

The library provides pkg-config support:

```bash
pkg-config --cflags --libs nvmpi
```

## Usage Examples

### Basic Decoding Example

```c
#include <nvmpi.h>

int decode_video(const char* input_file) {
    // Initialize decoder parameters
    nvDecParam dec_param = {
        .frame_pool_size = 8,
        .codingType = NV_VIDEO_CodingH264,
        .pixFormat = NV_PIX_NV12,
        .resized = {0, 0}  // No resizing
    };
    
    // Create decoder
    nvmpictx* decoder = nvmpi_create_decoder(&dec_param);
    if (!decoder) {
        return -1;
    }
    
    // Decoding loop
    while (has_more_packets) {
        nvPacket packet;
        // Fill packet with encoded data
        read_packet_from_file(&packet, input_file);
        
        // Send packet to decoder
        if (nvmpi_decoder_put_packet(decoder, &packet) < 0) {
            break;
        }
        
        // Receive decoded frames
        nvFrame frame;
        while (nvmpi_decoder_get_frame(decoder, &frame, false) > 0) {
            // Process decoded frame
            process_frame(&frame);
        }
    }
    
    // Cleanup
    nvmpi_decoder_close(decoder);
    return 0;
}
```

### Basic Encoding Example

```c
#include <nvmpi.h>

int encode_video(const char* output_file) {
    // Initialize encoder parameters
    nvEncParam enc_param = {
        .width = 1920,
        .height = 1080,
        .bitrate = 5000000,
        .fps_n = 30,
        .fps_d = 1,
        .codingType = NV_VIDEO_CodingH264,
        .profile = 100,
        .level = 51
    };
    
    // Create encoder
    nvmpictx* encoder = nvmpi_create_encoder(&enc_param);
    if (!encoder) {
        return -1;
    }
    
    // Encoding loop
    while (has_more_frames) {
        nvFrame frame;
        // Fill frame with raw video data
        read_frame_from_source(&frame);
        
        // Send frame to encoder
        if (nvmpi_encoder_put_frame(encoder, &frame) < 0) {
            break;
        }
        
        // Receive encoded packets
        nvPacket* packet;
        while (nvmpi_encoder_get_packet(encoder, &packet) > 0) {
            // Write encoded packet to file
            write_packet_to_file(packet, output_file);
            
            // Return packet to pool
            nvmpi_encoder_qEmptyPacket(encoder, packet);
        }
    }
    
    // Cleanup
    nvmpi_encoder_close(encoder);
    return 0;
}
```

### Hardware Scaling During Decode

```c
// Decode with hardware scaling to 720p
nvDecParam dec_param = {
    .frame_pool_size = 8,
    .codingType = NV_VIDEO_CodingH264,
    .pixFormat = NV_PIX_NV12,
    .resized = {1280, 720}  // Scale to 720p
};

nvmpictx* decoder = nvmpi_create_decoder(&dec_param);
// Decoded frames will be automatically scaled to 1280x720
```

### FFmpeg Command Line Examples

```bash
# Decode H.264 video using hardware acceleration
ffmpeg -c:v h264_nvmpi -i input.mp4 -f null -

# Decode with hardware scaling
ffmpeg -c:v h264_nvmpi -resize:v 1920x1080 -i input.mp4 -f null -

# Encode H.264 video using hardware acceleration
ffmpeg -i input.mp4 -c:v h264_nvmpi output.mp4

# Transcode H.264 to H.265 with hardware acceleration
ffmpeg -c:v h264_nvmpi -i input.mp4 -c:v hevc_nvmpi output.mp4

# Hardware-accelerated encoding with custom parameters
ffmpeg -i input.mp4 -c:v h264_nvmpi -b:v 5M -maxrate 10M output.mp4
```

## Error Handling

### Return Codes

The library uses consistent return code conventions:

- **Positive values**: Success with additional information
  - `1`: Operation successful, data available
- **Zero**: Success, no data available
- **Negative values**: Error conditions

### Common Error Scenarios

1. **Initialization Failures**
   - Hardware not available
   - Insufficient memory
   - Invalid parameters

2. **Runtime Errors**
   - Buffer pool exhaustion
   - Hardware timeout
   - Stream format changes

3. **Resource Errors**
   - DMA buffer allocation failure
   - V4L2 device errors

### Error Handling Best Practices

```c
// Always check return values
nvmpictx* ctx = nvmpi_create_decoder(&params);
if (!ctx) {
    fprintf(stderr, "Failed to create decoder\n");
    return -1;
}

// Handle runtime errors gracefully
int ret = nvmpi_decoder_put_packet(ctx, &packet);
if (ret < 0) {
    fprintf(stderr, "Decoder error: %d\n", ret);
    // Attempt recovery or cleanup
}

// Proper cleanup
nvmpi_decoder_close(ctx);
```

## Platform Support

### Supported Jetson Platforms

| Platform | JetPack 4.5+ | JetPack 5.0+ | JetPack 6.0+ |
|----------|---------------|---------------|---------------|
| Nano     | ✅            | ❌            | ❌            |
| TX2      | ✅            | ❌            | ❌            |
| Xavier NX| ✅            | ✅            | ❌            |
| AGX Xavier| ✅           | ✅            | ❌            |
| AGX Orin | ❌            | ✅            | ✅            |
| Orin NX  | ❌            | ✅            | ✅            |
| Orin Nano| ❌            | ✅            | ✅            |

### FFmpeg Compatibility

- **Supported versions**: FFmpeg 4.2 to 7.1+
- **API compatibility**: Maintained across versions
- **Integration**: Automatic codec registration

### System Requirements

- **Architecture**: ARM64 (aarch64)
- **OS**: Linux with L4T (Linux for Tegra)
- **Memory**: Sufficient for buffer pools (depends on resolution and pool size)
- **Hardware**: NVENC/NVDEC units available on the platform

---

*This documentation covers NVMPI library version 1.0.0. For the latest updates and examples, please refer to the project repository.*