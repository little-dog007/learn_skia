# learn_skia

1.
CMake配置中加上：-G Ninja -DVCPKG_TARGET_TRIPLET="arm64-osx" --preset=vcpkg

2.  根目录下创建CMakePresets.json 文件，并拷贝下面内容，注意需要更改 CMAKE_TOOLCHAIN_FILE 
{
   "version": 2,
   "configurePresets": [
   {
   "name": "vcpkg",
   "generator": "Ninja",
   "binaryDir": "${sourceDir}/build",
   "cacheVariables": {
   "CMAKE_TOOLCHAIN_FILE": "path/to/.vcpkg/scripts/buildsystems/vcpkg.cmake"
   }
   }
   ]
   }

