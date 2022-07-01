@echo off

SETLOCAL enabledelayedexpansion
set FAILCOUNT=0

cd ./Release

rem prepare files if not exist
call :genrnd "input"    100000000


call :makefiles "5m"    5000000
call :makefiles "50m"   50000000
call :makefiles "500m"  500000000
call :makefiles "5000m" 5000000000

rem run tests

call :test1 
call :test1 5m
call :test1 5m_bad
call :test1 5m_good

call :test1 50m
call :test1 50m_bad
call :test1 50m_good
rem #special cases
call :test1 50m 1000000
call :test1 50m 100000
call :test1 50m 98000
rem #intentionally failing cases
rem #test1 50m 98765
rem #test1 50m 10000

call :test1 500m
call :test1 500m_bad
call :test1 500m_good

call :test1 5000m
call :test1 5000m_bad
call :test1 5000m_good

echo FAILCOUNT=!FAILCOUNT!
pause
exit !FAILCOUNT!


:genrnd
  set FILE=%1
  set SIZE=%2
  if not exist %FILE% (
    if not exist "DummyCMD.exe" (
      echo "DummyCMD is simple way to create random files without admin rights (unlike fsutil)"
      echo download this util at http://www.mynikko.com/dummy/
      start "" http://www.mynikko.com/dummy/
      pause
      exit -1
    )
    DummyCMD.exe %FILE% %SIZE% 1
  ) else echo "File %FILE% exists, skipping"
  
exit /B

:genbad
  set FILE=%1
  if not exist %FILE% (
    makebad   %1    %2    %3
  ) else (
    echo "File %FILE% exists, skipping"
  )
exit /B

:makefiles
  set SUFFIX=%1
  set SIZE=%2
  call :genrnd "input_%SUFFIX%" %SIZE%
  call :genbad "input_%SUFFIX%_bad" %SIZE% 
  call :genbad "input_%SUFFIX%_good" %SIZE% "good"
exit /B

:test1
rem # %1 = custom SUFFIX
rem # %2 = custom MEMLIMIT

  if [%1]==[] ( 
    set INFILE=
    set OUTFILE=
    set LOGFILE=output.log

    del /f output
    del /f output.log

  ) else (
    set INFILE=input_%1
    set OUTFILE=output_%1
    set MEMLIMIT=%2
    if [%2] NEQ [] ( 
       set OUTFILE=output_%1_%2
    )
    set LOGFILE=!OUTFILE!.log

    del /f !OUTFILE!
    del /f !LOGFILE!
  )

  echo.
  echo ========================================================================
  echo bigsort '%INFILE%' '%OUTFILE%' '%MEMLIMIT%'
  echo ========================================================================
 
  bigsort %INFILE% %OUTFILE% %MEMLIMIT% > %LOGFILE%  2>&1
  if %ERRORLEVEL% EQU 0 (
    bigsort_test %$OUTFILE%  >> %LOGFILE% 2>&1
     if %ERRORLEVEL% NEQ 0 (
       echo "TEST FAILED: bigsort_test"
       set /A FAILCOUNT+=1
    )
  ) else (
    echo "TEST FAILED: bigsort"
    set /A FAILCOUNT+=1
  )

exit /B
