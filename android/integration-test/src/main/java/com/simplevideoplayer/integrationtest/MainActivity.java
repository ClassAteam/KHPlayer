package com.simplevideoplayer.integrationtest;

import org.libsdl.app.SDLActivity;

public class MainActivity extends SDLActivity {

    @Override
    protected String[] getLibraries() {
        return new String[]{
            "avutil",
            "avcodec",
            "avformat",
            "swscale",
            "swresample",
            "SDL2",
            "IntegrationVideoTest"
        };
    }

    @Override
    protected String getMainSharedObject() {
        return "libIntegrationVideoTest.so";
    }
}
