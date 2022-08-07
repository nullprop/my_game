#!/bin/bash

rm -rf bin
mkdir bin
cd bin

flags=(
	-std=gnu99 -w -ldl -lGL -lX11 -pthread -lXi -g -O0
)

inc=(
	-I ../third_party/include/
)

libs=(
	-lm
)

# Build game
proj_name=game
echo Building ${proj_name}...
src=(
	../src/main.c
	../src/**/*.c
)
build_cmd="gcc ${inc[*]} ${src[*]} ${flags[*]} ${libs[*]} -o ${proj_name}"
echo ${build_cmd}
${build_cmd}

if [ "$?" -ne "0" ]; then
	exit 1
fi

if [ "$1" == "game" ]; then
	exit 1
fi

# Build model viewer
proj_name=modelviewer
echo Building ${proj_name}...
src=(
	../src/model_viewer.c
	../src/**/*.c
)
build_cmd="gcc ${inc[*]} ${src[*]} ${flags[*]} ${libs[*]} -o ${proj_name}"
echo ${build_cmd}
${build_cmd}

# Copy assets
cd ..
cp ./assets/* ./bin -r
cp ./src/shaders ./bin -r
