# libultraship

## Building
```
cmake -H. -Bbuild
cmake --build build
```

## Generating a Visual Studio `.sln` on Windows
```
# Visual Studio 2022
& 'C:\Program Files\CMake\bin\cmake' -S . -B "build/x64" -G "Visual Studio 17 2022" -T v142 -A x64
# Visual Studio 2019
& 'C:\Program Files\CMake\bin\cmake' -S . -B "build/x64" -G "Visual Studio 16 2019" -T v142 -A x64
```