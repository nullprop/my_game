#!/bin/bash

rm -rf bin
mkdir bin
cd bin

proj_name=App
proj_root_dir=$(pwd)/../

flags=(
	-std=gnu99 -w -ldl -lGL -lX11 -pthread -lXi -g
)

# Include directories
inc=(
	-I ../third_party/include/
)

# Source files
src=(
	../source/*.c
	../source/**/*.c
)

# Build
echo gcc -O0 ${inc[*]} ${src[*]} ${flags[*]} -lm -o ${proj_name}
gcc -O0 ${inc[*]} ${src[*]} ${flags[*]} -lm -o ${proj_name}

cd ..

cp assets/* bin/ -r