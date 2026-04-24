#pragma once
extern "C" {
#include <libavutil/frame.h>
}

void frame_inspection(int frame_number, AVFrame* frame);
void bgra_frame_inspection(int frame_number, AVFrame* bgra_frame);

#ifdef __ANDROID__
#include <android/log.h>
#define TIMING_LOG(...) __android_log_print(ANDROID_LOG_INFO, "SimpleVideoPlayer", __VA_ARGS__)
#else
#include <cstdio>
#define TIMING_LOG(...) fprintf(stderr, __VA_ARGS__)
#endif
