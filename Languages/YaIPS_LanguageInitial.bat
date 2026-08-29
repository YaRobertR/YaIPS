@echo off
rem File: Ya3dag_LanguageInitial.bat
rem
rem Extract language transtlations to folder Ya3dagInitial.
rem This are initial translation files for the game Ya3dag.
rem
rem NOTE: This file is located in Ya3dag\ModLego\languages.
rem
rem
rem 15.06.2008 RR: first editon
rem
rem 10.09.2008 RR: renamed executable to Ya3dag_LanguageExtract.exe
rem                Used relative path names.
rem
rem 25.04.2009 RR: * Game scripts have moved from 'scripts' to
rem                  'gamedata'. Adapted directory strings.
rem                * Renamed this batch file from Ya3dag_LanguagInitial.bat
rem                  to Ya3dag_LanguageInitial.bat.
rem
rem 17.02.2018 RR: * Support for game specific language files
rem                  This is for the
rem                  'ModYaVoxel' subdirectory of Ya3dag.
rem
rem
@echo on

mkdir YaIPS-Initial

Ya3dag_LanguageExtract -F -n YaIPS-Initial\Translation.txt ..\src\*.h ..\src\*.cpp

pause
