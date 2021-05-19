#! /bin/bash
ThirdPartyPath=$(dirname $(readlink -f $0))
export THIRDPARTY_ROOT=$ThirdPartyPath

BuildscriptsPath=$(dirname $ThirdPartyPath)/buildscripts

echo "Third party: $ThirdPartyPath"
echo "Buildscripts: $BuildscriptsPath"

source $BuildscriptsPath/linux-setupenv.sh

# source $ThirdPartyPath/linux/build-gtest.sh Debug
# source $ThirdPartyPath/linux/build-gtest.sh RelWithDebInfo

# source $ThirdPartyPath/linux/build-libuv.sh Debug
# source $ThirdPartyPath/linux/build-libuv.sh RelWithDebInfo

source $ThirdPartyPath/linux/build-rapidjson.sh Debug
source $ThirdPartyPath/linux/build-rapidjson.sh RelWithDebInfo