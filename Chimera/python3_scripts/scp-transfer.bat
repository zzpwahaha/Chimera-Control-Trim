@echo off
setlocal

:: Define variables
set FILE=./*.py
set USER=root
set HOST=10.10.0.2
set REMOTE_DIR=~/regalTcp/

:: Perform SCP transfer
scp %FILE% %USER%@%HOST%:%REMOTE_DIR%

:: Check if the SCP command was successful
if %errorlevel% neq 0 (
    echo Error: SCP transfer failed.
    exit /b %errorlevel%
)

echo File transfer completed successfully.

endlocal