#include "SdlContext.h"
#include <SDL.h>
extern "C" {
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
}
#include <stdexcept>
#ifdef __ANDROID__
#include <android/log.h>
#include <SDL_system.h>
#include <EGL/egl.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "SimpleVideoPlayer", __VA_ARGS__)
#else
#include <cstdio>
#define LOG(...) printf(__VA_ARGS__)
#endif

SdlContext::SdlContext(const VideoContainer& container)
    : swr_ctx_(nullptr), window_(nullptr), renderer_(nullptr), texture_(nullptr),
      audio_device_(0) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
        throw std::runtime_error(std::string("Failed to initialize SDL: ") + SDL_GetError());

    initWindow(container.getWidth(), container.getHeight());
    initRenderer();
    number_of_channels_ = container.getNumberOfChannels();
    sample_rate_ = container.sampleRate();
    time_base_ = container.audioTimeBase();
    initAudioDevice(container.sampleRate(), container.getNumberOfChannels());
    createTexture(container.getWidth(), container.getHeight());
    SDL_RaiseWindow(window_);
    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_NONE);
    initResamplerContext(container.channelLayout(), container.getNumberOfChannels(),
                         container.sampleFormat());
}

SdlContext::~SdlContext() {
    SDL_PauseAudioDevice(audio_device_, 1);
    SDL_CloseAudioDevice(audio_device_);
#ifdef __ANDROID__
    if (media_surface_ || surface_texture_) {
        JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
        if (media_surface_)  { env->DeleteGlobalRef(media_surface_);  media_surface_  = nullptr; }
        if (surface_texture_){ env->DeleteGlobalRef(surface_texture_); surface_texture_ = nullptr; }
    }
    if (quad_vbo_)       glDeleteBuffers(1, &quad_vbo_);
    if (shader_program_) glDeleteProgram(shader_program_);
    if (external_gl_tex_) glDeleteTextures(1, &external_gl_tex_);
#endif
}

void SdlContext::initWindow(int width, int height) {
    window_ = SDL_CreateWindow("Simple Video Player", SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, width, height,
                               SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window_)
        throw std::runtime_error(std::string("Failed to create window: ") + SDL_GetError());
}

void SdlContext::initRenderer() {
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_)
        throw std::runtime_error(std::string("Failed to create renderer: ") + SDL_GetError());
}

void SdlContext::createTexture(int width, int height) {
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_NV12, SDL_TEXTUREACCESS_STREAMING,
                                 width, height);
    if (!texture_)
        throw std::runtime_error(std::string("Failed to create texture: ") + SDL_GetError());
}

// ── Android GL surface renderer ───────────────────────────────────────────────
#ifdef __ANDROID__

static const char* VERT_SRC = R"glsl(
    attribute vec4 position;
    attribute vec2 texCoord;
    varying vec2 v_tc;
    void main() { gl_Position = position; v_tc = texCoord; }
)glsl";

static const char* FRAG_SRC = R"glsl(
    #extension GL_OES_EGL_image_external : require
    precision mediump float;
    uniform samplerExternalOES videoTex;
    uniform mat4 texMatrix;
    varying vec2 v_tc;
    void main() { gl_FragColor = texture2D(videoTex, (texMatrix * vec4(v_tc, 0.0, 1.0)).xy); }
)glsl";

static const float QUAD_VERTS[] = {
    // x      y     z     w      u    v
    -1.0f, -1.0f, 0.0f, 1.0f,  0.0f, 0.0f,  // BL
     1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 0.0f,  // BR
    -1.0f,  1.0f, 0.0f, 1.0f,  0.0f, 1.0f,  // TL
     1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 1.0f,  // TR
};

static GLuint make_shader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(s, sizeof(buf), nullptr, buf);
        LOG("GL shader compile error: %s", buf);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

void SdlContext::compileGLShaders() {
    GLuint vert = make_shader(GL_VERTEX_SHADER,   VERT_SRC);
    GLuint frag = make_shader(GL_FRAGMENT_SHADER, FRAG_SRC);
    if (!vert || !frag) return;

    shader_program_ = glCreateProgram();
    glAttachShader(shader_program_, vert);
    glAttachShader(shader_program_, frag);
    glLinkProgram(shader_program_);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok = 0;
    glGetProgramiv(shader_program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetProgramInfoLog(shader_program_, sizeof(buf), nullptr, buf);
        LOG("GL program link error: %s", buf);
        glDeleteProgram(shader_program_);
        shader_program_ = 0;
        return;
    }

    position_loc_   = glGetAttribLocation (shader_program_, "position");
    texcoord_loc_   = glGetAttribLocation (shader_program_, "texCoord");
    tex_matrix_loc_ = glGetUniformLocation(shader_program_, "texMatrix");
    video_tex_loc_  = glGetUniformLocation(shader_program_, "videoTex");
    LOG("GL shaders compiled OK (pos=%d tc=%d tm=%d vt=%d)",
        position_loc_, texcoord_loc_, tex_matrix_loc_, video_tex_loc_);
}

void SdlContext::createQuadVBO() {
    glGenBuffers(1, &quad_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTS), QUAD_VERTS, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void* SdlContext::setupGLSurfaceRenderer() {
    SDL_GetWindowSize(window_, &window_width_, &window_height_);

    // 1. Create GL texture that SurfaceTexture will write into
    glGenTextures(1, &external_gl_tex_);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, external_gl_tex_);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

    // 2. Create SurfaceTexture(texId) via JNI
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();

    jclass st_cls  = env->FindClass("android/graphics/SurfaceTexture");
    jmethodID ctor = env->GetMethodID(st_cls, "<init>", "(I)V");
    jobject st_loc = env->NewObject(st_cls, ctor, (jint)external_gl_tex_);
    surface_texture_    = env->NewGlobalRef(st_loc);
    update_tex_image_   = env->GetMethodID(st_cls, "updateTexImage", "()V");
    get_transform_matrix_ = env->GetMethodID(st_cls, "getTransformMatrix", "([F)V");
    env->DeleteLocalRef(st_loc);
    env->DeleteLocalRef(st_cls);

    // 3. Create Surface(surfaceTexture) via JNI — passed to MediaCodec
    jclass  surf_cls  = env->FindClass("android/view/Surface");
    jmethodID s_ctor  = env->GetMethodID(surf_cls, "<init>",
                                         "(Landroid/graphics/SurfaceTexture;)V");
    jobject surf_loc  = env->NewObject(surf_cls, s_ctor, surface_texture_);
    media_surface_    = env->NewGlobalRef(surf_loc);
    env->DeleteLocalRef(surf_loc);
    env->DeleteLocalRef(surf_cls);

    // 4. Compile shaders + create quad VBO
    compileGLShaders();
    createQuadVBO();

    LOG("GL surface renderer ready: tex=%u prog=%u surface=%p",
        external_gl_tex_, shader_program_, (void*)media_surface_);
    return (void*)media_surface_;
}

void SdlContext::renderMediaCodecFrame() {
    if (!shader_program_ || !external_gl_tex_) return;

    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();

    // Update the GL texture from the latest SurfaceTexture buffer
    env->CallVoidMethod(surface_texture_, update_tex_image_);

    // Get the transform matrix (handles Y-flip, rotation, etc.)
    jfloatArray jmat = env->NewFloatArray(16);
    env->CallVoidMethod(surface_texture_, get_transform_matrix_, jmat);
    float mat[16];
    env->GetFloatArrayRegion(jmat, 0, 16, mat);
    env->DeleteLocalRef(jmat);

    // Draw fullscreen quad with external OES texture
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, window_width_, window_height_);

    glUseProgram(shader_program_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, external_gl_tex_);
    glUniform1i(video_tex_loc_, 0);
    glUniformMatrix4fv(tex_matrix_loc_, 1, GL_FALSE, mat);

    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    glEnableVertexAttribArray(position_loc_);
    glEnableVertexAttribArray(texcoord_loc_);
    // stride = 6 floats = 24 bytes; position offset=0, texcoord offset=16
    glVertexAttribPointer(position_loc_, 4, GL_FLOAT, GL_FALSE, 24, (void*)0);
    glVertexAttribPointer(texcoord_loc_, 2, GL_FLOAT, GL_FALSE, 24, (void*)16);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(position_loc_);
    glDisableVertexAttribArray(texcoord_loc_);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

    GLenum gl_err = glGetError();
    if (gl_err != GL_NO_ERROR)
        LOG("renderMediaCodecFrame GL error: 0x%x", gl_err);

    // Present: use EGL directly to avoid SDL renderer flush side-effects
    EGLBoolean swap_ok = eglSwapBuffers(eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_DRAW));
    if (!swap_ok)
        LOG("eglSwapBuffers failed: 0x%x", eglGetError());
}

#else // !__ANDROID__

void* SdlContext::setupGLSurfaceRenderer() { return nullptr; }
void SdlContext::renderMediaCodecFrame() {}

#endif // __ANDROID__

// ── Audio ─────────────────────────────────────────────────────────────────────

void SdlContext::sdlAudioCallback(void* userdata, uint8_t* stream, int len) {
    static_cast<SdlContext*>(userdata)->audioCallback(stream, len);
}

void SdlContext::audioCallback(uint8_t* stream, int len) {
    SDL_memset(stream, 0, len);

    while ((int)audio_buf_.size() < len) {
        auto frame_opt = audio_queue_.try_pop();
        if (!frame_opt) break;

        AVFrame* frame = *frame_opt;

        int out_buffer_size = av_samples_get_buffer_size(nullptr, number_of_channels_,
                                                         frame->nb_samples, AV_SAMPLE_FMT_S16, 1);
        uint8_t* output = (uint8_t*)av_malloc(out_buffer_size);
        if (!output) { av_frame_free(&frame); break; }

        uint8_t* out_ptr = output;
        int out_samples = swr_convert(swr_ctx_, &out_ptr, frame->nb_samples,
                                      (const uint8_t**)frame->data, frame->nb_samples);
        if (out_samples > 0) {
            int data_size = out_samples * number_of_channels_ * sizeof(int16_t);
            if (audio_buf_.empty())
                audio_buf_start_pts_ = frame->pts * time_base_;
            audio_buf_.insert(audio_buf_.end(), output, output + data_size);
        }

        av_free(output);
        av_frame_free(&frame);
    }

    int copy_size = std::min((int)audio_buf_.size(), len);
    if (copy_size > 0) {
        audio_clock_.store(audio_buf_start_pts_);
        SDL_MixAudioFormat(stream, audio_buf_.data(), AUDIO_S16SYS, copy_size, SDL_MIX_MAXVOLUME);
        audio_buf_.erase(audio_buf_.begin(), audio_buf_.begin() + copy_size);
        audio_buf_start_pts_ +=
            (double)copy_size / (sample_rate_ * number_of_channels_ * sizeof(int16_t));
    }
}

void SdlContext::initAudioDevice(int sample_rate, int num_of_channels) {
    SDL_AudioSpec wanted_spec, obtained_spec;
    wanted_spec.freq = sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = num_of_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = 1024;
    wanted_spec.callback = sdlAudioCallback;
    wanted_spec.userdata = this;
    audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &wanted_spec, &obtained_spec, 0);
    if (audio_device_ == 0)
        throw std::runtime_error(std::string("Failed to open audio device: ") + SDL_GetError());
    SDL_PauseAudioDevice(audio_device_, 0);
}

void SdlContext::initResamplerContext(AVChannelLayout ch_layout, int sample_rate,
                                      AVSampleFormat fmt) {
    swr_alloc_set_opts2(&swr_ctx_, &ch_layout, AV_SAMPLE_FMT_S16, sample_rate,
                        &ch_layout, fmt, sample_rate, 0, nullptr);
    swr_init(swr_ctx_);
}

void SdlContext::pushAudioFrame(AVFrame* frame) { audio_queue_.push(frame); }
SDL_Renderer* SdlContext::getRenderer()    { return renderer_; }
SDL_Texture*  SdlContext::getTexture()     { return texture_;  }
double SdlContext::getAudioClock() const   { return audio_clock_.load(); }
void SdlContext::pauseAudio(bool paused)   {
    SDL_PauseAudioDevice(audio_device_, paused ? 1 : 0);
}
