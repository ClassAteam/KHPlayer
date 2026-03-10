package com.simplevideoplayer;

import org.libsdl.app.SDLActivity;

/**
 * Entry point. Extends SDLActivity which handles the Android lifecycle,
 * surface creation, and JNI bridge to our native C++ main().
 *
 * SDL2 Java sources (SDLActivity.java etc.) must be copied into
 * src/main/java/org/libsdl/app/ from third_party/SDL2/android-project/app/src/main/java/org/libsdl/app/
 * This is done automatically by scripts/setup-android-deps.sh
 */
public class MainActivity extends SDLActivity {

    /**
     * Native libraries loaded in order before SimpleVideoPlayer.
     * FFmpeg libs must come before the player since it depends on them.
     */
    @Override
    protected String[] getLibraries() {
        return new String[]{
            "avutil",
            "avcodec",
            "avformat",
            "swscale",
            "swresample",
            "SDL2",
            "SimpleVideoPlayer"
        };
    }
}
