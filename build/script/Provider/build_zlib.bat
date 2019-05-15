@echo off

set zlib_version=zlib-1.2.11
SET ZLIB_PATH=%cd%\..\..\..\third_party_groupware\eSDK_Storage_Plugins\zlib\%zlib_version%
SET SOLUTION="%ZLIB_PATH%\contrib\vstudio\vc10\zlibvc.vcxproj"

if {x64} == {%~1} (
SET SOLUTION_CONFIG="Release|X64"
SET PLATFORM_TAG=x64
set OUTPUT_TAG=win64_x64_msvc
) else if {win32} == {%~1} (
SET SOLUTION_CONFIG="Release|Win32"
SET PLATFORM_TAG=x86
set OUTPUT_TAG=win32_x86_msvc
) else (
echo "the first parameter should be x64 or win32"
echo "Usage: build_zlib.bat x64 or build_zlib.bat win32"
goto Exit
)
SET ACTION=Rebuild

echo -----------start to compile zlib-----------

rem @"%VS100COMNTOOLS%\..\IDE\devenv" .\zlib-1.2.11\contrib\vstudio\vc10\zlibvc.vcxproj /Rebuild "Release|Win32"
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv" %SOLUTION% /%ACTION% %SOLUTION_CONFIG%


xcopy /Y "%ZLIB_PATH%\contrib\vstudio\vc10\%PLATFORM_TAG%\ZlibDllRelease\zlibwapi.dll" "%ZLIB_PATH%\..\..\build\%zlib_version%\%OUTPUT_TAG%\dll\"
xcopy /Y "%ZLIB_PATH%\contrib\vstudio\vc10\%PLATFORM_TAG%\ZlibDllRelease\zlibwapi.lib" "%ZLIB_PATH%\..\..\build\%zlib_version%\%OUTPUT_TAG%\lib\"

xcopy /Y "%ZLIB_PATH%\contrib\vstudio\vc10\%PLATFORM_TAG%\ZlibDllRelease\zlibwapi.dll" "%ZLIB_PATH%\%OUTPUT_TAG%\dll\"
xcopy /Y "%ZLIB_PATH%\contrib\vstudio\vc10\%PLATFORM_TAG%\ZlibDllRelease\zlibwapi.lib" "%ZLIB_PATH%\%OUTPUT_TAG%\lib\"

xcopy /Y %ZLIB_PATH%\contrib\minizip\crypt.h "%ZLIB_PATH%\..\..\build\%zlib_version%\include\"
xcopy /Y %ZLIB_PATH%\contrib\minizip\ioapi.c "%ZLIB_PATH%\..\..\build\%zlib_version%\include\"
xcopy /Y %ZLIB_PATH%\contrib\minizip\ioapi.h "%ZLIB_PATH%\..\..\build\%zlib_version%\include\"
xcopy /Y %ZLIB_PATH%\contrib\minizip\iowin32.h "%ZLIB_PATH%\..\..\build\%zlib_version%\include\"
xcopy /Y %ZLIB_PATH%\contrib\minizip\mztools.h "%ZLIB_PATH%\..\..\build\%zlib_version%\include\"
xcopy /Y %ZLIB_PATH%\contrib\minizip\unzip.h "%ZLIB_PATH%\..\..\build\%zlib_version%\include\"
xcopy /Y %ZLIB_PATH%\contrib\minizip\zip.h "%ZLIB_PATH%\..\..\build\%zlib_version%\include\"
xcopy /Y %ZLIB_PATH%\zconf.h "%ZLIB_PATH%\..\..\build\%zlib_version%\include\"
xcopy /Y %ZLIB_PATH%\zlib.h "%ZLIB_PATH%\..\..\build\%zlib_version%\include\"

::copy to \..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\ used by ssh2 and obs
xcopy /Y "%ZLIB_PATH%\contrib\vstudio\vc10\%PLATFORM_TAG%\ZlibDllRelease\zlibwapi.dll" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\bin\%OUTPUT_TAG%\release\"
xcopy /Y "%ZLIB_PATH%\contrib\vstudio\vc10\%PLATFORM_TAG%\ZlibDllRelease\zlibwapi.lib" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\lib\%OUTPUT_TAG%\release\"

xcopy /Y "%ZLIB_PATH%\..\..\build\%zlib_version%\include\*" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\include\"
 
:Exit
