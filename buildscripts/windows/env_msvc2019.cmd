@echo off
pushd "%~dp0"

where /Q cl
IF %ERRORLEVEL% EQU 0 goto done

popd

for /f "usebackq tokens=*" %%i in (`call findvc.cmd 16.0 17.0`) do (
  set VS2019InstallDir=%%i
)


set DevCmd=Common7\Tools\vsdevcmd.bat

if not exist "%VS2019InstallDir%\%DevCmd%" (
	goto noVS2019
)

echo Visual Studio 2019 Installation [%VS2019InstallDir%].

echo WTF (%1) (%2)

call "%VS2019InstallDir%\%DevCmd%" -arch=%1 -host_arch=%2

rem -arch=x64 -host_arch=x64

set PlatformToolset=v142

goto done


:noVS2019

echo Visual Studio 2019 Installation not found. [%VS2019InstallDir%]

rem exit 1


:done

popd
