# PipeSpectrum

Real-time FFT spectrum analyzer for PipeWire audio streams.

## Features

- **FFT Bar Spectrum**: Classic frequency band visualization like mixer/equalizer
- **Peak Hold**: Shows peak values on each frequency band
- **PipeWire Integration**: Captures audio from any PipeWire source
- **Hardware Accelerated**: OpenGL rendering for smooth performance
- **Configurable**: Customize bands, colors, sensitivity via YAML config

## Dependencies

### Arch Linux / Manjaro
```bash
sudo pacman -S pipewire sdl3 glew fftw yaml-cpp cmake gcc
```

**Note**: FFTW package on Arch includes both double and float versions.

### Ubuntu / Debian
```bash
sudo apt install libpipewire-0.3-dev libsdl3-dev libglew-dev libfftw3-single3 libfftw3-dev libyaml-cpp-dev cmake g++
```

### Fedora
```bash
sudo dnf install pipewire-devel SDL3-devel glew-devel fftw-devel yaml-cpp-devel cmake gcc-c++
```

## Building

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Installing

```bash
cd build
sudo make install
```

This installs:
- Binary to `/usr/local/bin/PipeSpectrum`
- Desktop file for application launchers
- Icons in multiple sizes
- Default config to `/usr/local/share/PipeSpectrum/`

## Running

From terminal:
```bash
./build/PipeSpectrum
```

Or launch from your desktop environment's application menu after installing.

Custom config:
```bash
./build/PipeSpectrum /path/to/config.yaml
```

**Requirements**:
- Running X11/Wayland session (GUI required)
- PipeWire audio system running
- Audio playing from some application to visualize

## Configuration

Edit `config.yaml` to customize:
- Number of frequency bands
- Color scheme
- Peak hold decay rate
- Window size
- Sensitivity

