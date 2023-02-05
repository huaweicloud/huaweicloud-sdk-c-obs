@echo off

rem 设置变量
SET LIBXML_VERSION=libxml2-2.9.9
SET LIBXML2_PATH=%cd%\..\..\..\third_party_groupware\eSDK_Storage_Plugins\%LIBXML_VERSION%
SET SOLUTION="%LIBXML2_PATH%\win32\VC10\libxml2.sln"
if {x64} == {%~1} (
SET SOLUTION_CONFIG="Release|x64"
set OUTPUT_TAG=win64_x64_msvc
) else if {win32} == {%~1} (
SET SOLUTION_CONFIG="Release|Win32"
set OUTPUT_TAG=win32_x86_msvc
) else (
echo "the first parameter should be x64 or win32"
echo "Usage: build_libxml2.bat x64 or build_libxml2.bat win32"
goto exit
)



SET ACTION=Rebuild

echo -----------start to compile libxml2-----------

rem 编译系统工程
"C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv" %SOLUTION% /%ACTION% %SOLUTION_CONFIG%


::xCOPY "%LIBXML2_PATH%\sln\pcre_839\Release\*.dll" "%LIBXML2_PATH%\..\..\build\libiconv-1.14\bin\" /s /e /y
xCOPY "%LIBXML2_PATH%\win32\VC10\Release\lib\*.lib" "%LIBXML2_PATH%\..\build\%LIBXML_VERSION%\lib\%OUTPUT_TAG%\release\" /s /e /y
xCOPY "%LIBXML2_PATH%\include\*.h" "%LIBXML2_PATH%\..\build\%LIBXML_VERSION%\include\" /s /e /y

::xCOPY "%LIBXML2_PATH%\sln\pcre_839\Release\*.dll" "%LIBXML2_PATH%\..\bin\win32_x86_msvc\release\" /s /e /y
xCOPY "%LIBXML2_PATH%\win32\VC10\Release\lib\*.lib" "%LIBXML2_PATH%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\lib\%OUTPUT_TAG%\release\" /s /e /y
xCOPY "%LIBXML2_PATH%\include\*.h" "%LIBXML2_PATH%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\include\" /s /e /y

:Exit

