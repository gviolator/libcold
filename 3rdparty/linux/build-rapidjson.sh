#! /bin/bash

BuildType=$1
Arch=x86_64
LibName=RapidJSON

SourceDir=$ThirdPartyPath/$LibName
BuildDir=$ThirdPartyPath/_build/$LibName-$Arch-$BuildType
InstallDir=$ThirdPartyPath/_dist/$Arch-$BuildType/$LibName
CompilerFlags=-Xclang -Wclang-cl-pch -Xclang -fdiagnostics-absolute-paths -Xclang -std=c++2a -Xclang -stdlib=libc++

echo Configure [$LibName]:$BuildType

cmake \
	-G Ninja \
	-DCMAKE_BUILD_TYPE=$BuildType \
	-DCMAKE_CXX_COMPILER=clang++ \
	-DCMAKE_CXX_FLAGS=$CompilerFlags \
	-DRAPIDJSON_BUILD_CXX17=ON \
	-DRAPIDJSON_BUILD_CXX11=OFF \
	-DRAPIDJSON_BUILD_TESTS=OFF\
	-DRAPIDJSON_BUILD_EXAMPLES=OFF \
	-DRAPIDJSON_BUILD_DOC=OFF \
	-DCMAKE_INSTALL_PREFIX=$InstallDir \
	-B $BuildDir \
	-S $SourceDir\

cmake --build "$BuildDir" --target install --config $BuildType
