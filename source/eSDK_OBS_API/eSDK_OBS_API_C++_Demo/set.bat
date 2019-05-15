set S3_HOSTNAME=0.0.0.0
set S3_ACCESS_KEY_ID=xxxxxxxxxxxxxxxxxxxx
set S3_SECRET_ACCESS_KEY=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

set "G_DEMO_DIR=%~dp0"

mkdir bin
xcopy /c /Y %G_DEMO_DIR%sln\*.exe %G_DEMO_DIR%bin
xcopy /c /Y %G_DEMO_DIR%..\bin\*.* %G_DEMO_DIR%bin
xcopy /c /Y %G_DEMO_DIR%*.crt %G_DEMO_DIR%bin
xcopy /c /Y %G_DEMO_DIR%*.jks %G_DEMO_DIR%bin