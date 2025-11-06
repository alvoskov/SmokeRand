@echo off

setlocal EnableDelayedExpansion

SET npassed=0
SET nfailed=0
SET nunknown=0

FOR %%i IN (generators\*.dll) DO (
    smokerand selftest %%i
    echo !ERRORLEVEL!
    IF !ERRORLEVEL! == 0 SET /a npassed=npassed+1
    IF !ERRORLEVEL! == 1 SET /a nfailed=nfailed+1
    IF !ERRORLEVEL! == 2 SET /a nunknown=nunknown+1
)

echo Passed:          %npassed%
echo Failed:          %nfailed%
echo Not implemented: %nunknown%

endlocal
