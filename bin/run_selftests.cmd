@echo off

setlocal EnableDelayedExpansion

SET npassed=0
SET nfailed=0
SET nunknown=0
SET failed_list

FOR %%i IN (generators\b*.dll) DO (
    smokerand selftest %%i
    echo !ERRORLEVEL!
    IF !ERRORLEVEL! == 0 SET /a npassed=npassed+1
    IF !ERRORLEVEL! == 1 (
        SET /a nfailed=nfailed+1
        SET failed_list=!failed_list!%%i, 
    )
    IF !ERRORLEVEL! == 2 SET /a nunknown=nunknown+1
)

echo Passed:          %npassed%
echo Failed:          %nfailed%
echo Not implemented: %nunknown%
echo Failed generators: %failed_list%

endlocal
