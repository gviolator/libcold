echo "Setup llvm env..."
call %~dp0\env_msvc2019.cmd x64 x64
set PATH=%LLVM%;%dp0%;%PATH%