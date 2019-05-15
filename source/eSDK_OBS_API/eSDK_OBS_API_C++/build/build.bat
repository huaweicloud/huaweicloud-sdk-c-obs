@echo off

set CUR_DIR=%~dp0

if {x64} == {%~3} (
call %CUR_DIR%build_x64.bat %~1 %~2
) else if {win32} == {%~3} (
call %CUR_DIR%build_win32.bat %~1 %~2
) else if {all} == {%~3} (
::echo "call win32 then call x64"
call %CUR_DIR%build_win32.bat %~1 %~2
call %CUR_DIR%build_x64.bat %~1 %~2
) else (
echo "the third param is not valid, only 'win32','x64' or 'all' is available"
)


@echo on
