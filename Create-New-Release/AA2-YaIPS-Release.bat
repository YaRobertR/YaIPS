@echo off
rem ************************************************************
rem  File: AA2-YaIPS-Release.bat  Author: R. Reinhard
rem
rem   Greate YaIPS release ZIP file.
rem
rem   11.08.2025  First edition
rem               Create a zip file for a new release.
rem
rem ************************************************************

call AA1-YaIPS-Release-Data.bat

set tempDiX=.\New-Release
set tempDir=.\New-Release\%projectName%_V%releaseVersion%
set ZipOut=%projectName%_V%releaseVersion%.zip
set toolDir=.\
set MiniZip=Ya3dag_minizip -9 -o -s -i pspbrwse.jbf -i LanguageExtract

echo ************************************
echo %projectName% V%releaseVersion%
echo destination %tempDiX%
echo --
echo Manual-DE = %Manual-DE%
echo Manual-EN = %Manual-EN%
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

mkdir %tempDir%
mkdir %tempDir%\CustomColors
mkdir %tempDir%\Doku
mkdir %tempDir%\Images
mkdir %tempDir%\Images\Backgrounds
mkdir %tempDir%\Images\Charts
mkdir %tempDir%\Images\Demo
mkdir %tempDir%\Images\Various-Images
mkdir %tempDir%\Images\YaIPS
mkdir %tempDir%\Languages
mkdir %tempDir%\Presets
mkdir %tempDir%\Videos

echo ------ add files ...

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem Root directory
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

xcopy /q  ..\GeneralPublicLicense.txt    %tempDir%
xcopy /q  ..\README.md                   %tempDir%
xcopy /q  ..\README-ReleaseHistory.txt   %tempDir%
xcopy /q  ..\YaIPS.exe                   %tempDir%
xcopy /q  ..\*.dll                       %tempDir%

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem Custum colors
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

xcopy /q  "..\CustomColors\Default colors.txt"  %tempDir%\CustomColors

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem Doku
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

xcopy /q  ..\Doku\%Manual-DE%  %tempDir%\Doku
xcopy /q  ..\Doku\%Manual-EN%  %tempDir%\Doku
xcopy /q  ..\Doku\ReadMe-for-developers\ReadMe-for-developers.pdf  %tempDir%\Doku

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem Images
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

xcopy /q  ..\Images\Backgrounds\*.*       %tempDir%\Images\Backgrounds
xcopy /q  ..\Images\Charts\*.*            %tempDir%\Images\Charts
xcopy /q /s  ..\Images\Demo\*.*           %tempDir%\Images\Demo
xcopy /q  ..\Images\Various-Images\*.*    %tempDir%\Images\Various-Images

xcopy /q  ..\Images\YaIPS\Icon-YaIPS.png  %tempDir%\Images\YaIPS
xcopy /q  ..\Images\YaIPS\Screenshot-*.*  %tempDir%\Images\YaIPS

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem Languages
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

xcopy /q /s ..\Languages\*.txt    %tempDir%\Languages

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem Presets
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

xcopy /q  ..\Presets\ResetToDefaults.prefs    %tempDir%\Presets 

rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
rem Videos
rem XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

xcopy /q  ..\Videos\Test-Frame-Numbers-10-Sec.mp4    %tempDir%\Videos 

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
