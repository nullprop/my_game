#!/bin/bash

rm -rf bin
mkdir bin
cd bin

proj_name=App
proj_root_dir=$(pwd)/../

flags=(
	-std=gnu99 -w -ldl -lGL -lX11 -pthread -lXi
)

# Include directories
inc=(
	-I ../third_party/include/             # gs
	-I ../third_party/assimp-5.0.1/include # assimp
)

# Source files
src=(
	../source/*.c
	../source/**/*.c
)

libs=(
	-L ../third_party/assimp-5.0.1/lib/Release
	-lassimp-vc142-mt
	-lIrrXML
	-lzlib
)

# Build
echo gcc -O3 ${inc[*]} ${src[*]} ${flags[*]} ${libs[*]} -lm -o ${proj_name}
gcc -O3 ${inc[*]} ${src[*]} ${flags[*]} ${libs[*]} -lm -o ${proj_name}

cd ..

cp assets/* bin/ -r
