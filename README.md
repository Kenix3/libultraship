# libultraship
libultraship (LUS) is a library meant to provide reimplementations of libultra (n64 sdk) functions that run on modern hardware.

LUS uses an asset loading system where data is stored separately from the executable in an archive file ending in `.otr` or `.o2r`. `.otr` files are [`.mpq`](http://www.zezula.net/en/mpq/main.html) compatible files. `.o2r` files are `.zip` compatible files. This separation of data from executable follows modern design practices which are more mod friendly. All one needs to do is supply a patch `.otr` or `.o2r` and the system will automatically replace the data.

## Contributing
LUS accepts any and all contributions. You can interact with the project via PRs, issues, email (kenixwhisperwind@gmail.com), or [Discord](https://discord.gg/shipofharkinian).
Please see [CONTRIBUTING.md](https://github.com/Kenix3/libultraship/blob/main/CONTRIBUTING.md) file for more information.

## Versioning
We use semantic versioning. We have defined the API as: every C linkage function, variable, struct, class, public class method, or enum included from libultraship.h.

## Building on Linux/Mac
```
cmake -H. -Bbuild
cmake --build build
```

## Generating a Visual Studio `.sln` on Windows
```
# Visual Studio 2022
& 'C:\Program Files\CMake\bin\cmake' -DUSE_AUTO_VCPKG=true -S . -B "build/x64" -G "Visual Studio 17 2022" -T v142 -A x64
# Visual Studio 2019
& 'C:\Program Files\CMake\bin\cmake' -DUSE_AUTO_VCPKG=true -S . -B "build/x64" -G "Visual Studio 16 2019" -T v142 -A x64
```

## To build on Windows
```
& 'C:\Program Files\CMake\bin\cmake' --build .\build\x64
```

## Sponsors
Thank you to JetBrains for providing their IDE [CLion](https://www.jetbrains.com/clion/) to me for free!

## License
LUS is licensed under the [MIT](https://github.com/Kenix3/libultraship/blob/main/LICENSE) license.

LUS makes use of the following third party libraries and resources:
- [Fast3D](https://github.com/Kenix3/libultraship/blob/main/src/fast/LICENSE.txt) (MIT) render display lists.
- [prism-processor](https://github.com/KiritoDv/prism-processor/blob/main/LICENSE) (MIT) shader preprocessor
- [ImGui](https://github.com/ocornut/imgui/blob/master/LICENSE.txt) (MIT)  display UI.
- [StormLib](https://github.com/ladislav-zezula/StormLib/blob/master/LICENSE) (MIT) create and read `.mpq` compatible archive files.
- [libzip](https://github.com/nih-at/libzip/blob/main/LICENSE) (BSD-3-Clause) create and read `.zip` compatible archives
- [StrHash64](https://github.com/Kenix3/libultraship/blob/main/src/ship/utils/StrHash64.cpp) (MIT, zlib, BSD-3-Clause) provide crc64 implementation.
- [nlohmann-json](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT) (MIT) json parsing and saving.
- [spdlog](https://github.com/gabime/spdlog/blob/v1.x/LICENSE) (MIT) logging
- [stb](https://github.com/nothings/stb/blob/master/LICENSE) (MIT) image conversion
- [thread-pool](https://github.com/bshoshany/thread-pool/blob/master/LICENSE.txt) (MIT) thread pool for the resource manager
- [tinyxml2](https://github.com/leethomason/tinyxml2/blob/master/LICENSE.txt) (zlib) parse XML files for resource loaders
- [zlib](https://github.com/madler/zlib/blob/develop/LICENSE) (zlib) compression used in StormLib
- [bzip2](https://github.com/libarchive/bzip2?tab=License-1-ov-file#readme) (bzip2) compression used in StormLib
- [sdl2](https://github.com/libsdl-org/SDL/blob/main/LICENSE.txt) (zlib) window manager, controllers, and audio player
- [glob_match](https://github.com/torvalds/linux/blob/d1bd5fa07667fcc3e38996ec42aef98761f23039/lib/glob.c) (Dual MIT/GPL) Glob pattern matching.
- [libgfxd](https://github.com/glankk/libgfxd/blob/master/LICENSE) (MIT) display list disassembler.
- [metal-cpp](https://github.com/bkaradzic/metal-cpp/blob/metal-cpp_26/LICENSE.txt) (Apache 2.0) interface to the Apple Metal rendering backend.
- [glew](https://github.com/nigels-com/glew/blob/master/LICENSE.txt) (modified BSD-3-Clause and MIT) OpenGL extension loading library.
