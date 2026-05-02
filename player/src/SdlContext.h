#pragma once
#include "BoundedQueue.h"
#include "VideoContainer.h"
#include <SDL_audio.h>
#include <SDL_render.h>
#include <SDL_video.h>
extern "C" {
#include <libswresample/swresample.h>
}
#include <atomic>
#include <vector>
#ifdef __ANDROID__
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <jni.h>
#endif

class SdlContext {
public:
    SdlContext(const VideoContainer& container);
    ~SdlContext();

    // Returns the Java Surface jobject (Android only, null on other platforms).
    // Call this after the constructor, before VideoContainer::openCodecs().
    void* setupGLSurfaceRenderer();

    // Reinitialize audio and texture for a new video without recreating the window/renderer.
    void reinitForVideo(const VideoContainer& container);

    // Calls SurfaceTexture.updateTexImage(), draws the GL_TEXTURE_EXTERNAL_OES
    // quad, and presents. Only does anything on Android.
    void renderMediaCodecFrame();

    SDL_Renderer* getRenderer();
    SDL_Texture* getTexture();
    void pushAudioFrame(AVFrame* frame);
    double getAudioClock() const;
    void pauseAudio(bool paused);

private:
    void initWindow(int width, int height);
    void initRenderer();
    void createTexture(int width, int height);
    void initAudioDevice(int sample_rate, int channels);
    static void sdlAudioCallback(void* userdata, uint8_t* stream, int len);
    void audioCallback(uint8_t* stream, int len);
    void initResamplerContext(AVChannelLayout ch_layout, int sample_rate, AVSampleFormat fmt);

#ifdef __ANDROID__
    void compileGLShaders();
    void createQuadVBO();
#endif

    SwrContext* swr_ctx_;

    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    SDL_AudioDeviceID audio_device_;
    BoundedQueue<AVFrame*> audio_queue_{25};
    int number_of_channels_;
    double time_base_;
    std::atomic<double> audio_clock_{0.0};
    std::vector<uint8_t> audio_buf_;
    double audio_buf_start_pts_{0.0};
    int sample_rate_{0};

#ifdef __ANDROID__
    GLuint external_gl_tex_{0};
    GLuint shader_program_{0};
    GLuint quad_vbo_{0};
    GLint  position_loc_{-1};
    GLint  texcoord_loc_{-1};
    GLint  tex_matrix_loc_{-1};
    GLint  video_tex_loc_{-1};
    jobject surface_texture_{nullptr};   // global JNI ref
    jobject media_surface_{nullptr};     // global JNI ref
    jmethodID update_tex_image_{nullptr};
    jmethodID get_transform_matrix_{nullptr};
    int window_width_{0};
    int window_height_{0};
#endif
};
