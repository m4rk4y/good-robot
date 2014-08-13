@echo off

REM Runs on Windows but expects Unix-like commands in the path.
REM I use unxutils.

setlocal enabledelayedexpansion

call :testIt test_input1.txt test_output1.txt

goto :eof

:testIt
set in=%1
set out=%2
( good_robot %in% 2>&1 ) > out.txt
REM diff -q counts different line-endings. We don't care about those, so
REM use diff ... | wc -l
for /f %%t in ( 'diff out.txt %out% ^| wc -l' ) do set /a diffCount=%%t
if %diffCount% gtr 0 (
    echo ERROR: test %in% failed:
    diff -c out.txt %out%
) else (
    echo OK: test %in% succeeded
)


