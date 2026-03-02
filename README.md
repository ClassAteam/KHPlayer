# Simple Video Player

A lightweight C++ video player built with FFmpeg and SDL2, featuring video decoding, audio playback, and basic playback controls.

## Features

- Video playback with audio synchronization
- Support for multiple video formats (MP4, AVI, MKV, etc.)
- Real-time playback controls
- Progress bar and time display
- Hardware-accelerated rendering via SDL2

## Requirements

### Dependencies

- **CMake** (>= 3.10)
- **FFmpeg libraries**:
  - libavcodec
  - libavformat
  - libavutil
  - libswscale
  - libswresample
- **SDL2**
- **pkg-config**
- **C++17 compatible compiler** (GCC 7+, Clang 5+, or MSVC 2017+)

### Installing Dependencies

#### Ubuntu/Debian

```bash
sudo apt update
sudo apt install cmake g++ pkg-config \
    libavcodec-dev libavformat-dev libavutil-dev \
    libswscale-dev libswresample-dev \
    libsdl2-dev
```

#### Fedora/RHEL

```bash
sudo dnf install cmake gcc-c++ pkg-config \
    ffmpeg-devel SDL2-devel
```

#### Arch Linux

```bash
sudo pacman -S cmake gcc pkg-config ffmpeg sdl2
```

#### macOS (Homebrew)

```bash
brew install cmake pkg-config ffmpeg sdl2
```

## Building

```bash
# Create build directory
mkdir build
cd build

# Generate build files
cmake ..

# Compile
make

# The executable will be in build/bin/SimpleVideoPlayer
```

## Usage

```bash
./bin/SimpleVideoPlayer <video_file>
```

### Example

```bash
./bin/SimpleVideoPlayer ../sample_video.mp4
```

## Controls

| Key | Action |
|-----|--------|
| **SPACE** | Play/Pause toggle |
| **LEFT** | Seek backward 10 seconds |
| **RIGHT** | Seek forward 10 seconds |
| **Q / ESC** | Quit player |

## Project Structure

```
video/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── include/
│   └── VideoPlayer.h       # Video player class header
└── src/
    ├── VideoPlayer.cpp     # Video player implementation
    └── main.cpp            # Application entry point
```

## Architecture

### Components

1. **VideoPlayer Class**: Core player functionality
   - Video/audio decoding using FFmpeg
   - Frame queue management
   - Audio/video synchronization
   - Playback control

2. **Decode Thread**: Reads and decodes video/audio packets
   - Maintains separate queues for video and audio frames
   - Handles stream demuxing

3. **Video Render Thread**: Displays video frames
   - Synchronizes video with audio clock
   - Converts frames to YUV420P for SDL2
   - Handles vsync and frame timing

4. **Audio Callback**: Outputs audio samples
   - SDL2 audio callback mechanism
   - Resamples audio to match output format
   - Updates audio clock for synchronization

## Technical Details

- **Threading Model**: Multi-threaded design with separate decode and render threads
- **Synchronization**: Audio clock-based video synchronization
- **Color Format**: YUV420P for efficient video rendering
- **Audio Format**: 16-bit signed integer samples
- **Frame Queue**: Bounded queue (max 25 frames) for memory management

## Limitations

- Seek functionality is basic (backward seeks only)
- No subtitle support
- No playlist or multi-file support
- Limited error recovery

## Future Enhancements

- Full bidirectional seeking
- Volume control
- Fullscreen mode
- Subtitle rendering
- Playback speed control
- Frame-by-frame stepping
- Screenshot capture

## Troubleshooting

### Build Errors

**Error: FFmpeg libraries not found**
```bash
# Ensure pkg-config can find FFmpeg
pkg-config --modversion libavcodec
```

**Error: SDL2 not found**
```bash
# Verify SDL2 installation
sdl2-config --version
```

### Runtime Errors

**Error: Failed to open video file**
- Check file path is correct
- Verify file format is supported by installed FFmpeg
- Ensure file is not corrupted

**Error: Failed to initialize SDL**
- Check display server is running (X11/Wayland)
- Verify audio device is available

**No audio playback**
- Check system audio is not muted
- Verify audio codec is supported
- Some video files may not have audio streams

## License

This project is provided as-is for educational purposes.

## Contributing

Feel free to submit issues and enhancement requests!
