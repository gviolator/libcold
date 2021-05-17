#! /bin/bash

BuildType=$1
Arch=x86_64
LibName=gtest

SourceDir=$ThirdPartyPath/googletest
BuildDir=$ThirdPartyPath/_build/$LibName-$Arch-$BuildType
InstallDir=$ThirdPartyPath/_dist/$Arch-$BuildType/$LibName
CompilerFlags=-Xclang -Wclang-cl-pch -Xclang -fdiagnostics-absolute-paths -Xclang -std=c++2a -Xclang -stdlib=libc++

echo Configure [$LibName]:$BuildType

cmake \
	-G Ninja \
	-DCMAKE_BUILD_TYPE=$BuildType \
	-DCMAKE_C_COMPILER=clang \
	-DCMAKE_CXX_COMPILER=clang++ \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=YES \
	-DCMAKE_CXX_FLAGS=$CompilerFlags \
	-DBUILD_GMOCK=YES \
	-DCMAKE_INSTALL_PREFIX=$InstallDir \
	-B $BuildDir \
	-S $SourceDir\

cmake --build "$BuildDir" --target install --config $BuildType
