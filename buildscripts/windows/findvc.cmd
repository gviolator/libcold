set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
vswhere -version [%1,%2) -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath