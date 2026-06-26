@echo off
setlocal

set "ACTION=%~1"
set "TARGET=%~2"

if "%ACTION%"=="" exit /b 1
if "%TARGET%"=="" exit /b 1

set "ROOT=%~dp0"
set "FM=%ROOT%WinZSTFM.exe"
set "GUI=%ROOT%WinZSTG.exe"
set "TARGET_DIR=%~dp2"
set "TARGET_NAME=%~nx2"

if not defined TARGET_NAME set "TARGET_NAME=Drive-%~d2"
set "TARGET_NAME=%TARGET_NAME::=%"

if /I "%ACTION%"=="open" (
  start "" "%FM%" "%TARGET%"
  exit /b 0
)

if /I "%ACTION%"=="extract-here" (
  start "" /D "%TARGET_DIR%" "%GUI%" x "%TARGET%" "-o%TARGET_DIR%"
  exit /b 0
)

if /I "%ACTION%"=="extract-to" (
  start "" /D "%TARGET_DIR%" "%GUI%" x "%TARGET%" -o*
  exit /b 0
)

if /I "%ACTION%"=="compress-tzs" (
  start "" /D "%TARGET_DIR%" "%GUI%" a "%TARGET_NAME%.tzs" "%TARGET%"
  exit /b 0
)

if /I "%ACTION%"=="compress-zip" (
  start "" /D "%TARGET_DIR%" "%GUI%" a "%TARGET_NAME%.zip" "%TARGET%"
  exit /b 0
)

if /I "%ACTION%"=="compress-7z" (
  start "" /D "%TARGET_DIR%" "%GUI%" a "%TARGET_NAME%.7z" "%TARGET%"
  exit /b 0
)

exit /b 1
