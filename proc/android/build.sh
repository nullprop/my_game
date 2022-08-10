#!/bin/bash

# Clean
rm -rf libs
rm -rf obj

# TODO: env vars
rm -rf ~/AndroidStudioProjects/NativeTest/app/src/main/jniLibs
rm -rf ~/AndroidStudioProjects/NativeTest/app/src/main/assets

cd jni
../../android-ndk-r25/ndk-build

cd ..
cp obj/local ~/AndroidStudioProjects/NativeTest/app/src/main/jniLibs -r
cp assets ~/AndroidStudioProjects/NativeTest/app/src/main/assets -r
cp src/shaders ~/AndroidStudioProjects/NativeTest/app/src/main/assets/ -r
