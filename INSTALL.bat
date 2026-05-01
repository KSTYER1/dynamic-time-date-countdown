@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul
title Dynamic Texts (Filter Plugin) - Installer

set "PLUGIN_NAME=dynamic-texts"
set "PLUGIN_DISPLAY=Dynamic Texts (Filter Plugin)"
set "PLUGIN_VERSION=1.3.0"
set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

echo ============================================================
echo   %PLUGIN_DISPLAY% v%PLUGIN_VERSION% - Installer
echo ============================================================
echo.
echo Enthaelt zwei Filter:
echo   - Dynamic Time/Date  (Uhrzeit, Datum, Wochentag, ISO-KW)
echo   - Dynamic Countdown  (Countdown zu/seit einem Datum)
echo.

:ask_path
echo OBS-Hauptordner eingeben (in dem bin\64bit\obs64.exe liegt).
echo Beispiel: C:\OBS 32.1 BETA
echo.
set /p OBS_DIR="OBS-Pfad: "
if "%OBS_DIR%"=="" goto ask_path

if not exist "%OBS_DIR%\bin\64bit\obs64.exe" (
    echo.
    echo [Fehler] In "%OBS_DIR%" wurde keine obs64.exe gefunden.
    echo.
    goto ask_path
)

echo.
echo Ziel: "%OBS_DIR%"
set /p CONFIRM="Kopieren? [J/n] "
if /i "%CONFIRM%"=="n" goto cancel
if /i "%CONFIRM%"=="nein" goto cancel

echo.
echo ----- DLL -----
xcopy /Y /I "%SCRIPT_DIR%\obs-plugins\64bit\%PLUGIN_NAME%.dll" "%OBS_DIR%\obs-plugins\64bit\"
xcopy /Y /I "%SCRIPT_DIR%\obs-plugins\64bit\%PLUGIN_NAME%.pdb" "%OBS_DIR%\obs-plugins\64bit\"

echo.
echo ----- Locale -----
if not exist "%OBS_DIR%\data\obs-plugins\%PLUGIN_NAME%\locale" (
    mkdir "%OBS_DIR%\data\obs-plugins\%PLUGIN_NAME%\locale"
)
xcopy /Y /I "%SCRIPT_DIR%\data\obs-plugins\%PLUGIN_NAME%\locale\*.ini" "%OBS_DIR%\data\obs-plugins\%PLUGIN_NAME%\locale\"

echo.
echo ============================================================
echo   FERTIG. Pruefen:
echo ============================================================
if exist "%OBS_DIR%\obs-plugins\64bit\%PLUGIN_NAME%.dll" (echo   [OK] %PLUGIN_NAME%.dll) else (echo   [FEHLT] DLL!)
if exist "%OBS_DIR%\data\obs-plugins\%PLUGIN_NAME%\locale\de-DE.ini" (echo   [OK] de-DE.ini) else (echo   [FEHLT] de-DE.ini!)
if exist "%OBS_DIR%\data\obs-plugins\%PLUGIN_NAME%\locale\en-US.ini" (echo   [OK] en-US.ini) else (echo   [FEHLT] en-US.ini!)
echo.
echo OBS neu starten. Bei einer Textquelle: Rechtsklick -^> Filter -^> +
echo "Dynamic Time/Date" und "Dynamic Countdown" muessen waehlbar sein.
echo.
pause
exit /b 0

:cancel
echo Abgebrochen.
pause
exit /b 1
