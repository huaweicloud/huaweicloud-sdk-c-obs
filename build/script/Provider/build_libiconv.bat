@echo off

rem 设置变量
set libiconv_version=libiconv-1.15
SET LIBICONV_PATH=%cd%\..\..\..\third_party_groupware\eSDK_Storage_Plugins\%libiconv_version%\source
SET SOLUTION="%LIBICONV_PATH%\windows\libiconv\libiconv.sln"
if {x64} == {%~1} (
SET SOLUTION_CONFIG="Release|X64"
SET OUTPUT_TAG=win64_x64_msvc
) else if {win32} == {%~1} (
SET SOLUTION_CONFIG="Release|Win32"
SET OUTPUT_TAG=win32_x86_msvc
) else (
echo "the first parameter should be x64 or win32"
echo "Usage: build_libiconv.bat x64 or build_libiconv.bat win32"
goto Exit
)
SET ACTION=Rebuild
::D:\esdk_obs_c\third_party_groupware\eSDK_Storage_Plugins\libxml2-2.9.4\win32\VC10\libxml2.sln
echo -----------start to compile iconv-----------

rem 编译系统工程
"C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv" %SOLUTION% /%ACTION% %SOLUTION_CONFIG%

xCOPY "%LIBICONV_PATH%\windows\libiconv\Release\lib\*.lib" "%LIBICONV_PATH%\..\..\build\%libiconv_version%\%OUTPUT_TAG%\lib\" /y
xCOPY "%LIBICONV_PATH%\windows\libiconv\*.h" "%LIBICONV_PATH%\..\..\build\%libiconv_version%\include\" /y

xCOPY "%LIBICONV_PATH%\windows\libiconv\Release\lib\*.lib" "%LIBICONV_PATH%\..\lib\%OUTPUT_TAG%\release\" /y
xCOPY "%LIBICONV_PATH%\windows\libiconv\*.h" "%LIBICONV_PATH%\..\include\" /y

xCOPY "%LIBICONV_PATH%\windows\libiconv\Release\lib\*.lib" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\lib\%OUTPUT_TAG%\release\" /y
xCOPY "%LIBICONV_PATH%\windows\libiconv\*.h" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\include\" /y

:Exit

