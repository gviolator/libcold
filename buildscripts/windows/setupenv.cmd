@echo off


@set ProjectRootPath=%~dp0..\..\
call :NORMALIZEPATH "%ProjectRootPath%"
set ProjectRootPath=%RETVAL%

@set BuildScriptsPath=%ProjectRootPath%buildscripts\
@set PythonEnvPath=%BuildScriptsPath%.venv\
@set ThirdPartyPath=%ProjectRootPath%3rdparty\
@set PYTHON3=C:\tools\Python38\python.exe



if not exist "%PythonEnvPath%Scripts\activate.bat" (
  echo setup python environment
  @%PYTHON3% -m venv %PythonEnvPath%
  call "%PythonEnvPath%Scripts\activate.bat"
  @python -m pip install --requirement %BuildScriptsPath%requirements.txt
) else (
  call "%PythonEnvPath%Scripts\activate.bat"
)




goto :eof

:NORMALIZEPATH
  SET RETVAL=%~f1
  EXIT /B

:eof