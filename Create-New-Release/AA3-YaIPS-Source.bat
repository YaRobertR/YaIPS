@echo off
rem ************************************************************
rem  File: AA3-YaIPS-Source.bat  Author: R. Reinhard
rem
rem   Greate YaIPS source ZIP file.
rem
rem   20.08.2028  First edition
rem               Create a zip file for a new source release.
rem
rem ************************************************************

call AA1-YaIPS-Release-Data.bat

set tempDiX=.\New-Source
set tempDir=.\New-Source\%projectName%_V%releaseVersion%
set ZipOut=%projectName%-Source_V%releaseVersion%.zip
set toolDir=.\
set MiniZip=Ya3dag_minizip -9 -o -s -i pspbrwse.jbf

echo ************************************
echo %projectName% V%releaseVersion%
echo destination %tempDiX%
echo --
echo Greate YaIPS source ZIP file
echo ************************************
echo ------------------------------------
echo VersionsNr, destination OK ?
echo New release compiled ?
echo ------------------------------------
%toolDir%\CHOICE.EXE /C:YND /N "all done ? [Y/N] "

set answer=illegal
if errorlevel 0  set answer=illegal
if errorlevel 1  set answer=Y
if errorlevel 2  set answer=N
if errorlevel 255  goto end

if %answer% == illegal goto end
if %answer% == N goto end

echo ------ delete %tempDiX% (remove old YaIPS release direcory) ...

if exist %tempDiX% rmdir /S /Q %tempDiX%

rem sleep for 5 seconds so all delete work is done
TYPE NUL | %toolDir%\CHOICE.EXE /N /CY /TY,5 >NUL

echo ------ create new %tempDiX% ...

mkdir %tempDiX%

rem echo on

echo ------ make directories ...

mkdir %tempDir%\Create-New-Release
mkdir %tempDir%\.settings
mkdir %tempDir%\Doku\%ManDir-DE%
mkdir %tempDir%\Doku\%ManDir-EN%
mkdir %tempDir%\Doku\ReadMe-for-developers
mkdir %tempDir%\Languages
mkdir %tempDir%\Third-party-libraries
mkdir %tempDir%\src

echo ------ add files ...

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem Root directory
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

rem copy eclipse workspace files

xcopy /q  "..\.cproject"        %tempDir% 
xcopy /q  "..\.project"         %tempDir%

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem .settings
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

xcopy /q ..\.settings\*.*  %tempDir%\.settings

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem Create-New-Release
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

xcopy /q ..\Create-New-Release\*.*  %tempDir%\Create-New-Release

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem Doku
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

xcopy /q ..\Doku\%ManDir-DE%  %tempDir%\Doku\%ManDir-DE%
xcopy /q ..\Doku\%ManDir-EN%  %tempDir%\Doku\%ManDir-EN%
xcopy /q ..\Doku\ReadMe-for-developers\*.*   %tempDir%\Doku\ReadMe-for-developers

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem Languages
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

xcopy /q ..\Languages\*.*    %tempDir%\Languages

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem Third-party-libraries
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

xcopy /q  /s ..\Third-party-libraries   %tempDir%\Third-party-libraries 

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem src
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

xcopy /q  /s ..\src            %tempDir%\src 

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem Create final zip file
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

echo ------ create final zip file ...

rem enshure downlaod dirtory is created
if not exist ..\Downloads mkdir ..\Downloads

cd %tempDiX%

..\%MiniZip% ..\..\Downloads\%ZipOut% %projectName%_V%releaseVersion%\*.*

cd ..

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem Remove temporary release directory
rem This prefenets the Eclipse IDE from searching
rem in this directory.
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

echo ------ delete %tempDiX% (temporary release direcory) ...

rmdir /S /Q %tempDiX%

rem --------------------------------------------------------------
rem Ende APPLIKATION
rem --------------------------------------------------------------

echo ------ Done

echo on

@echo off

pause

:end
echo on
