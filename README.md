# The goal
- Client application runs on the client's Android TV or mobile device. The client also installs a daemon on their desktop machine that plays the role of a video streaming service.
- When the user starts the app it obtains the list of all media files from the server's disk.
- The user then selects the needed file and starts watching using our own hand-made video player that does its best to run every codec smoothly on poor hardware devices, using MediaCodec in the best-case scenario or falling back to the best available video pipeline on the client's machine.

# Architecture
- We are using FFmpeg for video processing and SDL for other auxiliary work with multimedia (surfaces, audio, etc.)
- The server is a custom C++ HTTP/1.0 server over raw TCP sockets that handles file listing and video streaming.
- The player library consists of VideoContainer (FFmpeg format/codec management), Decoder, Renderer, Converter (pixel format via libswscale), and SdlContext (SDL2 window/audio/texture).
- The Android client bridges to the native player library via JNI.
- More detailed documentation and architecture map will emerge soon.

# State so far
- The whole interaction pipeline from starting a client and obtaining information about video files from the server to playing back the heaviest compression codecs was tested on our machines and systems.
- MediaCodec is properly initialized and performs well on our test hardware with HEVC (H.265) via the `hevc_mediacodec` decoder.
