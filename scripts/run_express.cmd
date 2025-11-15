@echo off

setlocal EnableDelayedExpansion

SET npassed=0
SET nfailed=0
SET nerror=0

FOR %%i IN (%~dp0..\bin\generators\*.dll) DO (
    %~dp0..\bin\smokerand express %%i --report-brief --threads
    echo !ERRORLEVEL!
    IF !ERRORLEVEL! == 0 SET /a npassed=npassed+1
    IF !ERRORLEVEL! == 1 SET /a nfailed=nfailed+1
    IF !ERRORLEVEL! == 2 SET /a nerror=nerror+1
)

echo Passed: %npassed%
echo Failed: %nfailed%
echo Error:  %nerror%

endlocal
