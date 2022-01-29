#!bin/sh

rm -rf bin
mkdir bin
cd bin

flags=(
	-std=gnu99 -w
)

inc=(
	-I ../third_party/include/
)

libs=(
	-lopengl32
	-lkernel32
	-luser32
	-lshell32
	-lgdi32
	-lWinmm
	-lAdvapi32
)

# Build game
proj_name=Game
echo Building ${proj_name}...
src=(
	../src/main.c
	../src/**/*.c
)
build_cmd="gcc -O3 ${inc[*]} ${src[*]} ${flags[*]} ${libs[*]} -lm -o ${proj_name}"
echo ${build_cmd}
${build_cmd}

# Build model viewer
proj_name=ModelViewer
echo Building ${proj_name}...
src=(
	../src/model_viewer.c
	../src/**/*.c
)
build_cmd="gcc -O3 ${inc[*]} ${src[*]} ${flags[*]} ${libs[*]} -lm -o ${proj_name}"
echo ${build_cmd}
${build_cmd}

# Copy assets
cd ..
cp ./assets/* ./bin -r
cp ./src/shaders ./bin -r
