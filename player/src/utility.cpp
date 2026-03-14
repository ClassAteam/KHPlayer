#include "utility.h"
#include <iostream>
#include <stdio.h>

void frame_inspection(int frame_number, AVFrame* frame) {

    std::cout << "frame " << frame_number << " recieved" << std::endl;

    char filename[64];
    snprintf(filename, sizeof(filename), "frame%d.pgm", frame_number);
    FILE* f = fopen(filename, "wb");
    fprintf(f, "P5\n%d %d\n255\n", frame->width, frame->height);
    for (int y = 0; y < frame->height; y++) {
        fwrite(frame->data[0] + y * frame->linesize[0], 1, frame->width, f);
    }
    fclose(f);
}

void bgra_frame_inspection(int frame_number, AVFrame* bgra_frame) {
    std::cout << "bgra_frame " << frame_number << " received" << std::endl;

    char filename[64];
    snprintf(filename, sizeof(filename), "bgra_frame%d.ppm", frame_number);
    FILE* f = fopen(filename, "wb");
    fprintf(f, "P6\n%d %d\n255\n", bgra_frame->width, bgra_frame->height);
    for (int y = 0; y < bgra_frame->height; y++) {
        uint8_t* row = bgra_frame->data[0] + y * bgra_frame->linesize[0];
        for (int x = 0; x < bgra_frame->width; x++) {
            uint8_t b = row[x * 4 + 0];
            uint8_t g = row[x * 4 + 1];
            uint8_t r = row[x * 4 + 2];
            fwrite(&r, 1, 1, f);
            fwrite(&g, 1, 1, f);
            fwrite(&b, 1, 1, f);
        }
    }
    fclose(f);
}
