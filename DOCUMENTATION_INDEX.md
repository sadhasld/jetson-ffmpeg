# NVMPI Library Documentation Index

This document provides a comprehensive overview of all available documentation for the NVMPI (NVIDIA Multimedia Processing Interface) library, which enables hardware-accelerated video encoding and decoding on NVIDIA Jetson platforms.

## 📚 Documentation Structure

### Core Documentation

| Document | Description | Audience |
|----------|-------------|----------|
| **[API_DOCUMENTATION.md](API_DOCUMENTATION.md)** | Complete API reference with detailed function descriptions, parameters, and examples | All developers |
| **[DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)** | Advanced usage patterns, best practices, and optimization techniques | Experienced developers |
| **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** | Concise reference for common tasks and parameters | All developers |

### Practical Resources

| Resource | Description | Use Case |
|----------|-------------|----------|
| **[examples/](examples/)** | Working code samples and tutorials | Learning and integration |
| **[README.md](README.md)** | Project overview and basic setup | Getting started |

## 🎯 Getting Started Path

### 1. First-Time Users
```
README.md → QUICK_REFERENCE.md → examples/simple_decode.c
```

### 2. Integration Developers
```
API_DOCUMENTATION.md → examples/ → DEVELOPER_GUIDE.md
```

### 3. Performance Optimization
```
DEVELOPER_GUIDE.md → examples/pipeline.cpp → API_DOCUMENTATION.md (Performance sections)
```

## 📖 Documentation Contents

### [API_DOCUMENTATION.md](API_DOCUMENTATION.md)
**Complete API Reference (8,000+ words)**

- **Overview**: Library architecture and key features
- **Data Types**: All structures, enums, and type definitions
- **Decoder API**: Complete decoder function reference
- **Encoder API**: Complete encoder function reference
- **Buffer Management**: Memory and buffer pool management
- **FFmpeg Integration**: Codec names and integration details
- **Build Configuration**: CMake options and dependencies
- **Usage Examples**: Practical code examples
- **Error Handling**: Return codes and best practices
- **Platform Support**: Jetson platform compatibility matrix

### [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)
**Advanced Development Guide (6,000+ words)**

- **Advanced Usage Patterns**: Asynchronous processing, pipeline design
- **Memory Management**: DMA buffers, custom allocators
- **Performance Optimization**: Buffer sizing, hardware presets, CPU affinity
- **Threading and Concurrency**: Thread-safe operations, producer-consumer patterns
- **Hardware Scaling**: Dynamic resolution changes, multi-resolution encoding
- **Custom Integration**: Memory allocators, hardware feature detection
- **Debugging and Profiling**: Logging, performance measurement, memory monitoring
- **Common Pitfalls**: Frequent mistakes and how to avoid them
- **FFmpeg Integration Details**: Custom AVCodec implementation
- **Building Custom Applications**: CMake integration, pkg-config usage

### [QUICK_REFERENCE.md](QUICK_REFERENCE.md)
**Concise Reference Guide (2,000+ words)**

- **Installation**: Build and setup instructions
- **Core Data Types**: Essential structures and enums
- **API Quick Reference**: Function signatures and basic usage
- **FFmpeg Commands**: Command-line examples
- **Platform Support**: Compatibility matrix
- **Common Parameters**: Typical configuration values
- **Error Handling**: Return code meanings
- **Performance Tips**: Optimization guidelines
- **Troubleshooting**: Common issues and solutions

### [examples/](examples/)
**Practical Code Examples**

#### Available Examples:
1. **simple_decode.c** - Basic decoder usage
2. **simple_encode.c** - Basic encoder usage  
3. **transcode.c** - Complete transcoding pipeline
4. **pipeline.cpp** - Multi-threaded processing
5. **ffmpeg_wrapper.c** - FFmpeg integration
6. **scaling_demo.c** - Hardware scaling demonstration
7. **batch_process.c** - Batch file processing

#### Build System:
- **Makefile** - Automated build system
- **README.md** - Example documentation and usage

## 🔧 API Quick Access

### Essential Functions

```c
// Decoder
nvmpictx* nvmpi_create_decoder(nvDecParam* param);
int nvmpi_decoder_put_packet(nvmpictx* ctx, nvPacket* packet);
int nvmpi_decoder_get_frame(nvmpictx* ctx, nvFrame* frame, bool wait);
int nvmpi_decoder_close(nvmpictx* ctx);

// Encoder
nvmpictx* nvmpi_create_encoder(nvEncParam* param);
int nvmpi_encoder_put_frame(nvmpictx* ctx, nvFrame* frame);
int nvmpi_encoder_get_packet(nvmpictx* ctx, nvPacket** packet);
int nvmpi_encoder_close(nvmpictx* ctx);
```

### Key Data Structures

```c
typedef struct _NVDECPARAM {
    int frame_pool_size;      // Buffer pool size (1-32)
    nvCodingType codingType;  // Input codec type
    nvPixFormat pixFormat;    // Output pixel format
    nvSize resized;           // Optional scaling
} nvDecParam;

typedef struct _NVENCPARAM {
    unsigned int width, height;    // Input resolution
    unsigned int bitrate;          // Target bitrate
    unsigned int fps_n, fps_d;     // Frame rate
    nvCodingType codingType;       // Output codec
    // ... additional parameters
} nvEncParam;
```

## 🎨 FFmpeg Integration

### Supported Codecs

| Format | Decoder | Encoder | Notes |
|--------|---------|---------|-------|
| H.264  | `h264_nvmpi` | `h264_nvmpi` | Full support |
| H.265  | `hevc_nvmpi` | `hevc_nvmpi` | Full support |
| MPEG-2 | `mpeg2_nvmpi` | ❌ | Decode only |
| MPEG-4 | `mpeg4_nvmpi` | ❌ | Decode only |
| VP8    | `vp8_nvmpi` | ❌ | Decode only |
| VP9    | `vp9_nvmpi` | ❌ | Decode only |

### Command Examples

```bash
# Hardware decode
ffmpeg -c:v h264_nvmpi -i input.mp4 -f null -

# Hardware encode
ffmpeg -i input.mp4 -c:v h264_nvmpi output.mp4

# Transcode with scaling
ffmpeg -c:v h264_nvmpi -resize:v 1280x720 -i input.mp4 -c:v hevc_nvmpi output.mp4
```

## 🏗️ Build and Installation

### Library Build
```bash
git clone https://github.com/Keylost/jetson-ffmpeg.git
cd jetson-ffmpeg
mkdir build && cd build
cmake ..
make
sudo make install
sudo ldconfig
```

### FFmpeg Integration
```bash
./ffpatch.sh /path/to/ffmpeg
cd /path/to/ffmpeg
./configure --enable-nvmpi
make && sudo make install
```

### Examples Build
```bash
cd examples
make all
```

## 🎯 Use Case Scenarios

### Real-time Video Processing
- **Documents**: DEVELOPER_GUIDE.md (Threading section)
- **Examples**: pipeline.cpp, live_stream.c
- **Key Features**: Low-latency decode, buffer pool optimization

### Batch Video Processing
- **Documents**: API_DOCUMENTATION.md, examples/README.md
- **Examples**: batch_process.c, transcode.c
- **Key Features**: High throughput, parallel processing

### Custom Application Integration
- **Documents**: DEVELOPER_GUIDE.md (Custom Integration section)
- **Examples**: ffmpeg_wrapper.c
- **Key Features**: CMake integration, custom memory management

### Hardware Optimization
- **Documents**: DEVELOPER_GUIDE.md (Performance section)
- **Examples**: scaling_demo.c
- **Key Features**: Hardware scaling, platform detection

## 🔍 Troubleshooting Guide

### Common Issues Quick Reference

| Issue | Solution Document | Section |
|-------|------------------|---------|
| Build failures | API_DOCUMENTATION.md | Build Configuration |
| Runtime errors | DEVELOPER_GUIDE.md | Common Pitfalls |
| Performance issues | DEVELOPER_GUIDE.md | Performance Optimization |
| Integration problems | examples/README.md | Common Issues |
| FFmpeg integration | API_DOCUMENTATION.md | FFmpeg Integration |

### Debug Resources

1. **Logging**: DEVELOPER_GUIDE.md → Debugging section
2. **Performance profiling**: DEVELOPER_GUIDE.md → Profiling section
3. **Memory issues**: DEVELOPER_GUIDE.md → Memory Management section
4. **Platform compatibility**: API_DOCUMENTATION.md → Platform Support

## 📊 Platform Compatibility

### Supported Platforms
- **Jetson Nano**: JetPack 4.5+ (H.264 encode/decode, limited HEVC)
- **Jetson TX2**: JetPack 4.5+ (Full codec support)
- **Jetson Xavier NX**: JetPack 4.5+ and 5.0+ (Full support)
- **Jetson AGX Xavier**: JetPack 4.5+ and 5.0+ (Full support)
- **Jetson AGX Orin**: JetPack 5.0+ and 6.0+ (Latest features)
- **Jetson Orin NX/Nano**: JetPack 5.0+ and 6.0+ (Latest features)

### FFmpeg Compatibility
- **Supported versions**: 4.2 to 7.1+
- **API stability**: Maintained across versions
- **Integration method**: Patch-based codec registration

## 📝 Contributing to Documentation

### Documentation Standards
- **Code examples**: All examples must compile and run
- **API coverage**: All public functions documented
- **Platform testing**: Examples tested on multiple Jetson platforms
- **Version compatibility**: Documentation updated for each release

### File Organization
```
├── API_DOCUMENTATION.md      # Complete API reference
├── DEVELOPER_GUIDE.md        # Advanced usage guide
├── QUICK_REFERENCE.md        # Concise reference
├── DOCUMENTATION_INDEX.md    # This file
├── examples/                 # Working code examples
│   ├── README.md            # Examples documentation
│   ├── Makefile             # Build system
│   └── *.c, *.cpp           # Example programs
└── README.md                 # Project overview
```

## 🔄 Documentation Updates

This documentation is maintained alongside the NVMPI library. Key update triggers:

- **API changes**: Function signatures, new features
- **Platform support**: New Jetson platforms, JetPack versions
- **Performance improvements**: New optimization techniques
- **Bug fixes**: Updated troubleshooting information
- **Example additions**: New use cases and integration patterns

---

## 📞 Support and Resources

- **Primary documentation**: Start with this index
- **API questions**: API_DOCUMENTATION.md
- **Implementation help**: DEVELOPER_GUIDE.md
- **Quick answers**: QUICK_REFERENCE.md
- **Working examples**: examples/ directory
- **Project repository**: [jetson-ffmpeg GitHub](https://github.com/Keylost/jetson-ffmpeg)

---

*This documentation covers NVMPI library version 1.0.0. For the latest updates, refer to the project repository and release notes.*