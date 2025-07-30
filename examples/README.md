# NVMPI Examples

This directory contains practical examples demonstrating how to use the NVMPI library for hardware-accelerated video processing on NVIDIA Jetson platforms.

## Building Examples

```bash
# Build all examples
cd examples
make all

# Build specific example
make simple_decode
```

## Example Programs

### 1. Simple Decode (`simple_decode.c`)
Basic H.264 decoder that reads from a file and outputs frame information.

**Usage:**
```bash
./simple_decode input.h264
```

**Features:**
- File-based H.264 decoding
- Frame information display
- Error handling demonstration

### 2. Simple Encode (`simple_encode.c`)
Basic H.264 encoder that reads raw YUV frames and outputs encoded stream.

**Usage:**
```bash
./simple_encode input.yuv output.h264 1920 1080
```

**Features:**
- Raw YUV frame encoding
- Configurable resolution
- Bitrate control

### 3. Transcode (`transcode.c`)
Complete transcoding example that decodes input and re-encodes with different parameters.

**Usage:**
```bash
./transcode input.mp4 output.h264 --bitrate 5000000 --resolution 1280x720
```

**Features:**
- Hardware decode + encode pipeline
- Resolution scaling during decode
- Configurable encoding parameters
- Performance measurement

### 4. Multi-threaded Pipeline (`pipeline.c`)
Advanced example showing asynchronous decode/encode pipeline.

**Usage:**
```bash
./pipeline input.mp4 output.mp4 --threads 2
```

**Features:**
- Producer-consumer threading model
- Buffer pool management
- Performance optimization
- Real-time processing

### 5. FFmpeg Integration (`ffmpeg_wrapper.c`)
Example showing how to integrate NVMPI with FFmpeg libraries.

**Usage:**
```bash
./ffmpeg_wrapper input.mp4 output.mp4
```

**Features:**
- FFmpeg demuxing/muxing
- NVMPI hardware acceleration
- Format conversion
- Metadata preservation

### 6. Live Stream Processing (`live_stream.c`)
Real-time video processing example for live streams.

**Usage:**
```bash
./live_stream rtmp://input.stream rtmp://output.stream
```

**Features:**
- RTMP input/output
- Low-latency processing
- Adaptive bitrate
- Error recovery

### 7. Batch Processing (`batch_process.c`)
Batch video processing with multiple input files.

**Usage:**
```bash
./batch_process input_dir/ output_dir/ --preset fast
```

**Features:**
- Directory processing
- Multiple encoder presets
- Progress reporting
- Parallel processing

### 8. Hardware Scaling (`scaling_demo.c`)
Demonstrates hardware scaling capabilities during decode.

**Usage:**
```bash
./scaling_demo input.mp4 --output-resolution 1280x720
```

**Features:**
- Hardware scaling during decode
- Multiple output resolutions
- Performance comparison
- Quality assessment

## Compilation Requirements

- NVMPI library installed
- pkg-config support
- GCC or Clang compiler
- CUDA toolkit (for some examples)

## Makefile Targets

```bash
make all          # Build all examples
make clean        # Clean build files
make install      # Install examples to /usr/local/bin
make test         # Run basic tests
```

## Common Issues

### Permission Errors
```bash
# Run with proper permissions
sudo ./example_program

# Or add user to video group
sudo usermod -a -G video $USER
```

### Library Not Found
```bash
# Check library path
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Or use pkg-config
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
```

### Hardware Not Available
- Ensure you're running on a supported Jetson platform
- Check that hardware acceleration is available:
  ```bash
  ls /dev/nvhost-*
  ```

## Performance Tips

1. **Buffer Pool Size**: Adjust based on your use case
   - Low latency: 4-6 buffers
   - High throughput: 8-16 buffers

2. **Threading**: Use separate threads for decode/encode when possible

3. **Memory Management**: Pre-allocate buffers for best performance

4. **Hardware Presets**: Choose appropriate encoder presets
   - `ULTRAFAST`: Maximum speed, lower quality
   - `FAST`: Good balance
   - `SLOW`: Better quality, slower encoding

## Example Output

```
$ ./simple_decode sample.h264
[INFO] Created decoder successfully
[INFO] Decoded frame 0: 1920x1080, format=NV12, pts=0
[INFO] Decoded frame 1: 1920x1080, format=NV12, pts=33333
[INFO] Decoded frame 2: 1920x1080, format=NV12, pts=66666
...
[INFO] Total frames decoded: 300
[INFO] Average decode time: 2.3ms per frame
```

## Integration with Your Project

To integrate these examples into your project:

1. Copy the relevant example as a starting point
2. Modify parameters for your specific use case
3. Add your custom processing logic
4. Handle errors appropriately for your application

## Support

For questions about these examples:
1. Check the main API documentation
2. Review the developer guide for advanced topics
3. Examine the source code for implementation details

---

*These examples demonstrate real-world usage of the NVMPI library. Modify them according to your specific requirements.*