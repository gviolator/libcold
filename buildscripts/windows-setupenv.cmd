@echo off


@set ProjectRootPath=%~dp0..\
call :NORMALIZEPATH "%ProjectRootPath%"
set ProjectRootPath=%RETVAL%

@set BuildScriptsPath=%ProjectRootPath%buildscripts\
@set ThirdPartyPath=%ProjectRootPath%3rdparty\


goto :eof

:NORMALIZEPATH
  SET RETVAL=%~f1
  EXIT /B

:eof