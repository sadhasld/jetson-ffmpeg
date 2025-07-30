CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -O2 -std=c11
CXXFLAGS = -Wall -Wextra -O2 -std=c++11
LDFLAGS = 

# Use pkg-config to get NVMPI flags
NVMPI_CFLAGS = $(shell pkg-config --cflags nvmpi 2>/dev/null || echo "-I/usr/local/include")
NVMPI_LIBS = $(shell pkg-config --libs nvmpi 2>/dev/null || echo "-lnvmpi")

# FFmpeg flags (optional, for ffmpeg_wrapper example)
FFMPEG_CFLAGS = $(shell pkg-config --cflags libavformat libavcodec libavutil 2>/dev/null)
FFMPEG_LIBS = $(shell pkg-config --libs libavformat libavcodec libavutil 2>/dev/null)

# Threading support
THREAD_LIBS = -lpthread

# All example programs
PROGRAMS = simple_decode simple_encode transcode pipeline scaling_demo batch_process

# FFmpeg wrapper (only if FFmpeg is available)
ifneq ($(FFMPEG_LIBS),)
PROGRAMS += ffmpeg_wrapper
endif

# Live stream example (requires additional libraries)
# PROGRAMS += live_stream

.PHONY: all clean install test

all: $(PROGRAMS)

# Simple decode example
simple_decode: simple_decode.c
	$(CC) $(CFLAGS) $(NVMPI_CFLAGS) -o $@ $< $(NVMPI_LIBS)

# Simple encode example  
simple_encode: simple_encode.c
	$(CC) $(CFLAGS) $(NVMPI_CFLAGS) -o $@ $< $(NVMPI_LIBS)

# Transcode example
transcode: transcode.c
	$(CC) $(CFLAGS) $(NVMPI_CFLAGS) -o $@ $< $(NVMPI_LIBS)

# Multi-threaded pipeline example
pipeline: pipeline.cpp
	$(CXX) $(CXXFLAGS) $(NVMPI_CFLAGS) -o $@ $< $(NVMPI_LIBS) $(THREAD_LIBS)

# Hardware scaling demo
scaling_demo: scaling_demo.c
	$(CC) $(CFLAGS) $(NVMPI_CFLAGS) -o $@ $< $(NVMPI_LIBS)

# Batch processing example
batch_process: batch_process.c
	$(CC) $(CFLAGS) $(NVMPI_CFLAGS) -o $@ $< $(NVMPI_LIBS) $(THREAD_LIBS)

# FFmpeg wrapper (conditional)
ffmpeg_wrapper: ffmpeg_wrapper.c
	$(CC) $(CFLAGS) $(NVMPI_CFLAGS) $(FFMPEG_CFLAGS) -o $@ $< $(NVMPI_LIBS) $(FFMPEG_LIBS)

# Live stream example (requires librtmp or similar)
live_stream: live_stream.c
	$(CC) $(CFLAGS) $(NVMPI_CFLAGS) -o $@ $< $(NVMPI_LIBS) $(THREAD_LIBS) -lrtmp

clean:
	rm -f $(PROGRAMS) *.o

install: all
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(PROGRAMS) $(DESTDIR)/usr/local/bin/

test: all
	@echo "Running basic tests..."
	@echo "Testing simple_decode..."
	@if [ -f test_input.h264 ]; then \
		./simple_decode test_input.h264 > /dev/null && echo "✓ simple_decode test passed"; \
	else \
		echo "⚠ test_input.h264 not found, skipping test"; \
	fi
	@echo "Testing simple_encode..."
	@if [ -f test_input.yuv ]; then \
		./simple_encode test_input.yuv test_output.h264 1920 1080 > /dev/null && echo "✓ simple_encode test passed"; \
	else \
		echo "⚠ test_input.yuv not found, skipping test"; \
	fi
	@echo "All available tests completed"

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build all examples"
	@echo "  clean        - Remove built files"
	@echo "  install      - Install examples to /usr/local/bin"
	@echo "  test         - Run basic tests"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Individual programs:"
	@for prog in $(PROGRAMS); do echo "  $$prog"; done
	@echo ""
	@echo "Requirements:"
	@echo "  - NVMPI library installed"
	@echo "  - pkg-config support"
	@echo "  - GCC/G++ compiler"

# Debug target for troubleshooting
debug:
	@echo "Compiler information:"
	@echo "CC = $(CC)"
	@echo "CXX = $(CXX)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "CXXFLAGS = $(CXXFLAGS)"
	@echo ""
	@echo "NVMPI configuration:"
	@echo "NVMPI_CFLAGS = $(NVMPI_CFLAGS)"
	@echo "NVMPI_LIBS = $(NVMPI_LIBS)"
	@echo ""
	@echo "FFmpeg configuration:"
	@echo "FFMPEG_CFLAGS = $(FFMPEG_CFLAGS)"
	@echo "FFMPEG_LIBS = $(FFMPEG_LIBS)"
	@echo ""
	@echo "Programs to build: $(PROGRAMS)"