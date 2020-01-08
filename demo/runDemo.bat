@ECHO OFF
REM SETLOCAL ENABLEDELAYEDEXPANSION
IF NOT EXIST ..\bin\getLNKinfo.exe (
  ECHO.
  ECHO Executable getLNKinfo.exe not found in bin directory. Please build/install it before runing the demo!
  ECHO.
  GOTO end
)

ECHO Printing information contained in file exampleLinkFile.lnk:
ECHO.
FOR /F "tokens=*" %%i IN ('..\bin\getLNKinfo.exe /F  exampleLinkFile.lnk') DO ECHO /F    Filename        %%i
FOR /F "tokens=*" %%i IN ('..\bin\getLNKinfo.exe /P  exampleLinkFile.lnk') DO ECHO /P    Path            %%i
FOR /F "tokens=*" %%i IN ('..\bin\getLNKinfo.exe     exampleLinkFile.lnk') DO ECHO /PF   Path+Filename   %%i
FOR /F "tokens=*" %%i IN ('..\bin\getLNKinfo.exe /PR exampleLinkFile.lnk') DO ECHO /PR   Relative path   %%i
FOR /F "tokens=*" %%i IN ('..\bin\getLNKinfo.exe /VL exampleLinkFile.lnk') DO ECHO /VL   Volume label    %%i
FOR /F "tokens=*" %%i IN ('..\bin\getLNKinfo.exe /VT exampleLinkFile.lnk') DO ECHO /VT   Volume type     %%i
FOR /F "tokens=*" %%i IN ('..\bin\getLNKinfo.exe /W  exampleLinkFile.lnk') DO ECHO /W    Working dir     %%i
FOR /F "tokens=*" %%i IN ('..\bin\getLNKinfo.exe /CL exampleLinkFile.lnk') DO ECHO /CL   Command line    %%i
FOR /F "tokens=*" %%i IN ('..\bin\getLNKinfo.exe /I  exampleLinkFile.lnk') DO ECHO /I    Icon            %%i
FOR /F "tokens=*" %%i IN ('..\bin\getLNKinfo.exe /N  exampleLinkFile.lnk') DO ECHO /N    Link name       %%i
ECHO.

:end
PAUSE
