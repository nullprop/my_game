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
	-I ../third_party/include/             # gs
	-I ../third_party/assimp-5.0.1/include # assimp
)

# Source files
src=(
	../source/*.c
	../source/**/*.c
)

libs=(
	-L ../third_party/assimp-5.0.1/lib/Debug
	-lassimp-vc142-mtd
	-lIrrXMLd
	-lzlibstaticd
)

# Build
echo gcc -O0 ${inc[*]} ${src[*]} ${flags[*]} ${libs[*]} -lm -o ${proj_name}
gcc -O0 ${inc[*]} ${src[*]} ${flags[*]} ${libs[*]} -lm -o ${proj_name}

cd ..

# Assets
cp ./assets/* ./bin -r

# Third party binaries
cp ./third_party/assimp-5.0.1/bin/Debug/* ./bin -r
