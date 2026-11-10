@echo off
rem Compiles the Campfire addon's two scripts against the stubs in Stubs\
rem (in place of Campfire's real sources and their dependencies).
rem Set SKYRIM to your Skyrim Special Edition folder, then run this file from
rem the folder it's in (Addons\Campfire\Scripts).
setlocal
set SKYRIM=D:\SteamLibrary\steamapps\common\Skyrim Special Edition
set COMPILER=%SKYRIM%\Papyrus Compiler\PapyrusCompiler.exe
set FLAGS=TESV_Papyrus_Flags.flg
rem Stubs first, so they're found before any Campfire sources you copied.
set IMPORTS=%~dp0Stubs;%~dp0Source;%SKYRIM%\Data\Source\Scripts
set OUT=%SKYRIM%\Data\Scripts

"%COMPILER%" "%~dp0Source\LZC_CampfireSkillsProxy.psc" -f="%FLAGS%" -i="%IMPORTS%" -o="%OUT%"
"%COMPILER%" "%~dp0Source\LZC_CampfireAddonPlayerScript.psc" -f="%FLAGS%" -i="%IMPORTS%" -o="%OUT%"
pause
