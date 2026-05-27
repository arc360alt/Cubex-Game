## Building

### Android

Requires **NDK r16b** (last release with `stlport_static`).

```
cd handheld/project/android
$NDK_ROOT/ndk-build -j4 NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk
```

The APK targets API 28 with `minSdkVersion` 9. Runtime storage permissions are requested automatically on API 23+. On API 30+ (Android 11+), the app requests "All Files Access" on first launch.

---

### macOS

**Dependencies** — install via [Homebrew](https://brew.sh):

```
brew install sdl2 libpng zlib
```

System frameworks (no extra install needed): `OpenGL`, `OpenAL`, `Foundation`.

```
cd handheld/project/macos
make -j8
./minecraftpe
```

---

### Linux

**Dependencies** — install via your package manager:

```
# Ubuntu / Debian
sudo apt-get install libsdl2-dev libpng-dev zlib1g-dev libopenal-dev libgl1-mesa-dev

# Fedora / RHEL
sudo dnf install SDL2-devel libpng-devel zlib-devel openal-soft-devel mesa-libGL-devel

# Arch
sudo pacman -S sdl2 libpng zlib openal mesa
```

```
cd handheld/project/linux
make -j$(nproc)
./minecraftpe
```

---

### Windows

Open `handheld/project/win32/MinecraftWin32.sln` in **Visual Studio** (2010 or newer recommended).

Dependencies expected by the project (install via [vcpkg](https://vcpkg.io) or manually):
- [SDL2](https://libsdl.org) — windowing and input
- [GLEW](https://glew.sourceforge.net) — OpenGL extension loading
- [OpenAL Soft](https://openal-soft.org) — audio

Build the `minecraft` configuration in Release or Debug mode.

---

### Raspberry Pi

Uses SDL 1.2 and GLES 1.x via the Broadcom VideoCore headers (`/opt/vc/`).

```
cd handheld/project/raspberry
make -j4
./minecraftpe
```

Requires the legacy GL driver (`sudo raspi-config` > Advanced > GL Driver > Legacy).

---