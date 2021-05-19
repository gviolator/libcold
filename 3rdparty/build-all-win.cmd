@call %~dp0..\buildscripts\windows-setupenv.cmd

echo Buildscripts: %BuildScriptsPath%
echo Third party: %ThirdPartyPath%

call %~dp0\windows\build-gtest.cmd Debug
call %~dp0\windows\build-gtest.cmd RelWithDebInfo
call %~dp0\windows\build-libuv.cmd Debug
call %~dp0\windows\build-libuv.cmd RelWithDebInfo
call %~dp0\windows\build-rapidjson.cmd Debug
call %~dp0\windows\build-rapidjson.cmd RelWithDebInfo