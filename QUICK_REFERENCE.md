# NVMPI Quick Reference Guide

## Installation

```bash
# Build and install library
git clone https://github.com/Keylost/jetson-ffmpeg.git
cd jetson-ffmpeg
mkdir build && cd build
cmake ..
make
sudo make install
sudo ldconfig

# Patch and build FFmpeg
cd ../
./ffpatch.sh /path/to/ffmpeg
cd /path/to/ffmpeg
./configure --enable-nvmpi
make
sudo make install
```

## Core Data Types

```c
typedef struct nvmpictx nvmpictx;           // Opaque context

typedef enum {
    NV_PIX_NV12, NV_PIX_YUV420            // Pixel formats
} nvPixFormat;

typedef enum {
    NV_VIDEO_CodingH264,                   // H.264/AVC
    NV_VIDEO_CodingHEVC,                   // H.265/HEVC
    NV_VIDEO_CodingMPEG2,                  // MPEG-2
    NV_VIDEO_CodingMPEG4,                  // MPEG-4
    NV_VIDEO_CodingVP8,                    // VP8
    NV_VIDEO_CodingVP9                     // VP9
} nvCodingType;
```

## Decoder API

### Create Decoder
```c
nvDecParam params = {
    .frame_pool_size = 8,
    .codingType = NV_VIDEO_CodingH264,
    .pixFormat = NV_PIX_NV12,
    .resized = {1920, 1080}  // Optional scaling
};
nvmpictx* decoder = nvmpi_create_decoder(&params);
```

### Decode Process
```c
// Send packet
nvPacket packet = {.payload = data, .payload_size = size};
nvmpi_decoder_put_packet(decoder, &packet);

// Get frame
nvFrame frame;
if (nvmpi_decoder_get_frame(decoder, &frame, true) > 0) {
    // Process frame
}

// Cleanup
nvmpi_decoder_close(decoder);
```

## Encoder API

### Create Encoder
```c
nvEncParam params = {
    .width = 1920, .height = 1080,
    .bitrate = 5000000,
    .fps_n = 30, .fps_d = 1,
    .codingType = NV_VIDEO_CodingH264,
    .profile = 100, .level = 51
};
nvmpictx* encoder = nvmpi_create_encoder(&params);
```

### Encode Process
```c
// Send frame
nvFrame frame = {.width = 1920, .height = 1080, .type = NV_PIX_NV12};
nvmpi_encoder_put_frame(encoder, &frame);

// Get packet
nvPacket* packet;
if (nvmpi_encoder_get_packet(encoder, &packet) > 0) {
    // Process packet
    nvmpi_encoder_qEmptyPacket(encoder, packet);
}

// Cleanup
nvmpi_encoder_close(encoder);
```

## FFmpeg Codec Names

| Format | Decoder | Encoder |
|--------|---------|---------|
| H.264  | `h264_nvmpi` | `h264_nvmpi` |
| H.265  | `hevc_nvmpi` | `hevc_nvmpi` |
| MPEG-2 | `mpeg2_nvmpi` | ❌ |
| MPEG-4 | `mpeg4_nvmpi` | ❌ |
| VP8    | `vp8_nvmpi` | ❌ |
| VP9    | `vp9_nvmpi` | ❌ |

## FFmpeg Commands

```bash
# Decode with hardware acceleration
ffmpeg -c:v h264_nvmpi -i input.mp4 -f null -

# Decode with scaling
ffmpeg -c:v h264_nvmpi -resize:v 1280x720 -i input.mp4 output.yuv

# Encode with hardware acceleration
ffmpeg -i input.mp4 -c:v h264_nvmpi -b:v 5M output.mp4

# Transcode H.264 to H.265
ffmpeg -c:v h264_nvmpi -i input.mp4 -c:v hevc_nvmpi output.mp4
```

## Return Codes

| Value | Meaning |
|-------|---------|
| `> 0` | Success, data available |
| `0`   | Success, no data |
| `< 0` | Error |

## Common Parameters

### Decoder Parameters
```c
nvDecParam params = {
    .frame_pool_size = 4-32,        // Buffer pool size
    .codingType = NV_VIDEO_Coding*, // Input codec
    .pixFormat = NV_PIX_*,          // Output format
    .resized = {width, height}      // Optional scaling (0,0 = none)
};
```

### Encoder Parameters
```c
nvEncParam params = {
    .width = 1920, .height = 1080,  // Input resolution
    .bitrate = 5000000,             // Target bitrate (bps)
    .peak_bitrate = 10000000,       // Peak bitrate (VBR)
    .fps_n = 30, .fps_d = 1,        // Frame rate (30/1 = 30fps)
    .profile = 100,                 // H.264 profile (66=baseline, 77=main, 100=high)
    .level = 51,                    // H.264 level
    .iframe_interval = 30,          // I-frame interval
    .qmin = 20, .qmax = 40,         // QP range
    .mode_vbr = 1,                  // Variable bitrate
    .enableLossless = 0,            // Lossless encoding
    .codingType = NV_VIDEO_CodingH264
};
```

## Platform Support

| Platform | JetPack 4.5+ | JetPack 5.0+ | JetPack 6.0+ |
|----------|:-------------:|:-------------:|:-------------:|
| Nano     | ✅ | ❌ | ❌ |
| TX2      | ✅ | ❌ | ❌ |
| Xavier NX| ✅ | ✅ | ❌ |
| AGX Xavier| ✅ | ✅ | ❌ |
| AGX Orin | ❌ | ✅ | ✅ |
| Orin NX  | ❌ | ✅ | ✅ |
| Orin Nano| ❌ | ✅ | ✅ |

## Build Options

```bash
cmake \
  -DJETSON_MULTIMEDIA_API_DIR=/usr/src/jetson_multimedia_api \
  -DJETSON_MULTIMEDIA_LIB_DIR=/usr/lib/aarch64-linux-gnu/tegra \
  -DCUDA_INCLUDE_DIR=/usr/local/cuda/include \
  -DCUDA_LIB_DIR=/usr/local/cuda/lib64 \
  -DWITH_STUBS=OFF \
  ..
```

## Error Handling Template

```c
nvmpictx* ctx = nvmpi_create_decoder(&params);
if (!ctx) {
    fprintf(stderr, "Failed to create decoder\n");
    return -1;
}

int ret = nvmpi_decoder_put_packet(ctx, &packet);
if (ret < 0) {
    fprintf(stderr, "Decode error: %d\n", ret);
    nvmpi_decoder_close(ctx);
    return -1;
}

// Always cleanup
nvmpi_decoder_close(ctx);
```

## Memory Management

```c
// Frame structure
typedef struct _NVFRAME {
    unsigned long payload_size[3];  // Size of each plane
    unsigned char *payload[3];      // Plane data pointers
    unsigned int linesize[3];       // Line size for each plane
    nvPixFormat type;               // NV_PIX_NV12 or NV_PIX_YUV420
    unsigned int width, height;     // Frame dimensions
    time_t timestamp;               // Frame timestamp
} nvFrame;

// Packet structure
typedef struct _NVPACKET {
    unsigned long flags;            // Packet flags (keyframe = 1)
    unsigned long payload_size;     // Payload size in bytes
    unsigned char *payload;         // Packet data
    unsigned long pts;              // Presentation timestamp
} nvPacket;
```

## Threading Notes

- ✅ Multiple contexts in different threads
- ❌ Sharing context between threads (without sync)
- ✅ Built-in buffer pool thread safety
- ✅ Asynchronous processing support

## Performance Tips

1. **Buffer Pool Size**: Use 4-8 for low latency, 8-16 for throughput
2. **Hardware Presets**: `ULTRAFAST` for speed, `SLOW` for quality
3. **Zero-Copy**: Use DMA buffers when possible
4. **Pipeline**: Overlap decode/encode operations
5. **CPU Affinity**: Pin threads to specific cores for real-time

## Troubleshooting

| Issue | Solution |
|-------|----------|
| "Failed to create decoder" | Check hardware support, memory availability |
| "Buffer pool exhausted" | Increase pool size or process frames faster |
| "Invalid format" | Verify input codec matches decoder type |
| "Permission denied" | Run with proper permissions for hardware access |
| "Library not found" | Check `LD_LIBRARY_PATH` and installation |

## pkg-config Usage

```bash
# Get compile flags
pkg-config --cflags nvmpi

# Get link flags  
pkg-config --libs nvmpi

# Compile example
gcc $(pkg-config --cflags --libs nvmpi) -o app main.c
```

## Minimal Example

```c
#include <nvmpi.h>

int main() {
    // Create decoder
    nvDecParam dec_params = {8, NV_VIDEO_CodingH264, NV_PIX_NV12, {0,0}};
    nvmpictx* decoder = nvmpi_create_decoder(&dec_params);
    
    // Process data...
    nvPacket packet = {.payload = data, .payload_size = size};
    nvmpi_decoder_put_packet(decoder, &packet);
    
    nvFrame frame;
    if (nvmpi_decoder_get_frame(decoder, &frame, true) > 0) {
        printf("Decoded: %dx%d\n", frame.width, frame.height);
    }
    
    nvmpi_decoder_close(decoder);
    return 0;
}
```

---

*For detailed documentation, see `API_DOCUMENTATION.md` and `DEVELOPER_GUIDE.md`*