@call %~dp0..\buildscripts\windows\setupenv.cmd
set PATH=%LLVM%;%PATH%
set THIRDPARTY_ROOT=%ThirdPartyPath%

echo Buildscripts: %BuildScriptsPath%
echo Third party: %THIRDPARTY_ROOT%

conan create %ThirdPartyPath%conan-recipe/conanfile-libuv.py libcold/local -o shared=True --profile %BuildScriptsPath%conan-profiles/x86_64-Windows-clang-Debug
conan create %ThirdPartyPath%conan-recipe/conanfile-libuv.py libcold/local -o shared=True --profile %BuildScriptsPath%conan-profiles/x86_64-Windows-clang-Release