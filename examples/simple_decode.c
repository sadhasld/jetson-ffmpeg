/**
 * Simple NVMPI Decoder Example
 * 
 * This example demonstrates basic usage of the NVMPI decoder API.
 * It reads H.264 data from a file and decodes it using hardware acceleration.
 */

#include <nvmpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define BUFFER_SIZE (64 * 1024)  // 64KB read buffer
#define MAX_FRAMES 1000          // Maximum frames to decode

// Performance measurement
double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

// Print frame information
void print_frame_info(const nvFrame* frame, int frame_num) {
    printf("[INFO] Frame %d: %dx%d, format=%s, planes=%d, timestamp=%ld\n",
           frame_num,
           frame->width,
           frame->height,
           frame->type == NV_PIX_NV12 ? "NV12" : "YUV420",
           frame->type == NV_PIX_NV12 ? 2 : 3,
           frame->timestamp);
    
    // Print plane information
    for (int i = 0; i < (frame->type == NV_PIX_NV12 ? 2 : 3); i++) {
        if (frame->payload_size[i] > 0) {
            printf("  Plane %d: size=%lu, linesize=%u\n",
                   i, frame->payload_size[i], frame->linesize[i]);
        }
    }
}

// Save frame to file (optional)
void save_frame_to_file(const nvFrame* frame, int frame_num, const char* output_prefix) {
    if (!output_prefix) return;
    
    char filename[256];
    snprintf(filename, sizeof(filename), "%s_frame_%04d.yuv", output_prefix, frame_num);
    
    FILE* output = fopen(filename, "wb");
    if (!output) {
        fprintf(stderr, "[ERROR] Failed to create output file: %s\n", filename);
        return;
    }
    
    // Write Y plane
    if (frame->payload_size[0] > 0) {
        fwrite(frame->payload[0], 1, frame->payload_size[0], output);
    }
    
    // Write UV/U/V planes
    for (int i = 1; i < (frame->type == NV_PIX_NV12 ? 2 : 3); i++) {
        if (frame->payload_size[i] > 0) {
            fwrite(frame->payload[i], 1, frame->payload_size[i], output);
        }
    }
    
    fclose(output);
    printf("[INFO] Saved frame to: %s\n", filename);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.h264> [output_prefix] [max_frames]\n", argv[0]);
        fprintf(stderr, "Examples:\n");
        fprintf(stderr, "  %s video.h264                    # Decode and show info\n", argv[0]);
        fprintf(stderr, "  %s video.h264 frames             # Decode and save frames\n", argv[0]);
        fprintf(stderr, "  %s video.h264 frames 100         # Decode first 100 frames\n", argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_prefix = (argc > 2) ? argv[2] : NULL;
    int max_frames = (argc > 3) ? atoi(argv[3]) : MAX_FRAMES;
    
    printf("[INFO] Starting NVMPI decoder example\n");
    printf("[INFO] Input file: %s\n", input_file);
    printf("[INFO] Max frames: %d\n", max_frames);
    if (output_prefix) {
        printf("[INFO] Output prefix: %s\n", output_prefix);
    }
    
    // Open input file
    FILE* input = fopen(input_file, "rb");
    if (!input) {
        fprintf(stderr, "[ERROR] Failed to open input file: %s\n", input_file);
        return 1;
    }
    
    // Configure decoder parameters
    nvDecParam dec_params = {
        .frame_pool_size = 8,              // Buffer pool size
        .codingType = NV_VIDEO_CodingH264, // H.264 codec
        .pixFormat = NV_PIX_NV12,          // NV12 output format
        .resized = {0, 0}                  // No scaling
    };
    
    // Create decoder
    printf("[INFO] Creating decoder...\n");
    nvmpictx* decoder = nvmpi_create_decoder(&dec_params);
    if (!decoder) {
        fprintf(stderr, "[ERROR] Failed to create decoder\n");
        fclose(input);
        return 1;
    }
    printf("[INFO] Decoder created successfully\n");
    
    // Decoding loop
    unsigned char buffer[BUFFER_SIZE];
    int frame_count = 0;
    int packet_count = 0;
    double total_decode_time = 0.0;
    double start_time = get_time_ms();
    
    printf("[INFO] Starting decode loop...\n");
    
    while (frame_count < max_frames) {
        // Read data from file
        size_t bytes_read = fread(buffer, 1, BUFFER_SIZE, input);
        if (bytes_read == 0) {
            printf("[INFO] End of file reached\n");
            break;
        }
        
        // Create packet
        nvPacket packet = {
            .flags = 0,
            .payload_size = bytes_read,
            .payload = buffer,
            .pts = packet_count * 33333, // Assume 30fps (33.33ms per frame)
            .privData = NULL
        };
        
        // Send packet to decoder
        double decode_start = get_time_ms();
        int ret = nvmpi_decoder_put_packet(decoder, &packet);
        if (ret < 0) {
            fprintf(stderr, "[ERROR] Failed to send packet to decoder (ret=%d)\n", ret);
            break;
        }
        packet_count++;
        
        // Get decoded frames
        nvFrame frame;
        while (nvmpi_decoder_get_frame(decoder, &frame, false) > 0) {
            double decode_end = get_time_ms();
            double decode_time = decode_end - decode_start;
            total_decode_time += decode_time;
            
            print_frame_info(&frame, frame_count);
            
            // Save frame if requested
            if (output_prefix && frame_count < 10) { // Save first 10 frames only
                save_frame_to_file(&frame, frame_count, output_prefix);
            }
            
            frame_count++;
            printf("[INFO] Decode time: %.2f ms\n", decode_time);
            
            if (frame_count >= max_frames) {
                break;
            }
        }
        
        // Show progress every 100 packets
        if (packet_count % 100 == 0) {
            printf("[INFO] Processed %d packets, decoded %d frames\n", 
                   packet_count, frame_count);
        }
    }
    
    // Send flush packet to get remaining frames
    printf("[INFO] Flushing decoder...\n");
    nvPacket flush_packet = {0};
    nvmpi_decoder_put_packet(decoder, &flush_packet);
    
    nvFrame frame;
    while (nvmpi_decoder_get_frame(decoder, &frame, false) > 0) {
        print_frame_info(&frame, frame_count);
        frame_count++;
        
        if (frame_count >= max_frames) {
            break;
        }
    }
    
    // Calculate statistics
    double end_time = get_time_ms();
    double total_time = end_time - start_time;
    double avg_decode_time = frame_count > 0 ? total_decode_time / frame_count : 0.0;
    double fps = frame_count > 0 ? (frame_count * 1000.0) / total_time : 0.0;
    
    printf("\n[INFO] Decoding completed\n");
    printf("[INFO] Statistics:\n");
    printf("  Total packets processed: %d\n", packet_count);
    printf("  Total frames decoded: %d\n", frame_count);
    printf("  Total time: %.2f ms\n", total_time);
    printf("  Average decode time: %.2f ms per frame\n", avg_decode_time);
    printf("  Average FPS: %.2f\n", fps);
    
    // Cleanup
    printf("[INFO] Cleaning up...\n");
    int close_ret = nvmpi_decoder_close(decoder);
    if (close_ret < 0) {
        fprintf(stderr, "[WARNING] Error closing decoder: %d\n", close_ret);
    }
    
    fclose(input);
    
    printf("[INFO] Example completed successfully\n");
    return 0;
}