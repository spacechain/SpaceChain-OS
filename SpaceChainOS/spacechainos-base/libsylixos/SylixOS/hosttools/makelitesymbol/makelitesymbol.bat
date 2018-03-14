@echo off
setlocal enabledelayedexpansion

set srcfile=%1
set destfile=%2

nm %srcfile% > %srcfile%_nm

findstr /C:" T "   < %srcfile%_nm    > func.txt
findstr /C:" W "   < %srcfile%_nm   >> func.txt
findstr /C:" D "   < %srcfile%_nm    > obj.txt
findstr /C:" B "   < %srcfile%_nm   >> obj.txt
findstr /C:" R "   < %srcfile%_nm   >> obj.txt
findstr /C:" S "   < %srcfile%_nm   >> obj.txt
findstr /C:" C "   < %srcfile%_nm   >> obj.txt
findstr /C:" V "   < %srcfile%_nm   >> obj.txt
findstr /C:" G "   < %srcfile%_nm   >> obj.txt

del %srcfile%_nm

set num=0

del symbol.c 1>NUL 2>&1
del symbol.h 1>NUL 2>&1

for /f "tokens=1,3 delims= " %%i in (func.txt) do @(
    echo %%j = 0x%%i; >> %destfile%
    set /a num+=1
)


for /f "tokens=1,3 delims= " %%i in (obj.txt) do @(
    echo %%j = 0x%%i; >> %destfile%
    set /a num+=1
)

del func.txt
del obj.txt

@echo on