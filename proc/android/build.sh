#!/bin/bash

# Clean
rm -rf libs
rm -rf obj

# TODO: env vars
rm -rf ~/AndroidStudioProjects/NativeTest/app/libs/*
rm -rf ~/AndroidStudioProjects/NativeTest/app/assets/*

cd jni
../../android-ndk-r25/ndk-build

cd ..
cp obj/local/* ~/AndroidStudioProjects/NativeTest/app/libs/ -r
cp assets/* ~/AndroidStudioProjects/NativeTest/app/assets/ -r
cp src/shaders ~/AndroidStudioProjects/NativeTest/app/assets/ -r
