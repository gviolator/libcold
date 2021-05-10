#! /bin/bash
ThirdPartyPath=$(dirname $(readlink -f $0))
export THIRDPARTY_ROOT=$ThirdPartyPath

BuildscriptsPath=$(dirname $ThirdPartyPath)/buildscripts

echo "Third party: $ThirdPartyPath"
echo "Buildscripts: $BuildscriptsPath"

$BuildscriptsPath/linux/setenv.sh

# source "$BuildscriptsPath/.venv/bin/activate"

conan create $ThirdPartyPath/conan-recipe/conanfile-libuv.py libcold/local -o shared=True --profile $BuildscriptsPath/conan-profiles/x86_64-linux-v142-Debug

conan create $ThirdPartyPath/conan-recipe/conanfile-gtest.py libcold/local --profile $BuildscriptsPath/conan-profiles/x86_64-linux-v142-Debug
