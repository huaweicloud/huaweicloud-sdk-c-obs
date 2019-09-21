1. 第三方依赖：
    openssl(1.0.2r) zlib(1.2.11.0) libiconv(1.15) pcre(8.39) libssh2(1.9.0) libcurl(7.64.1) libxml2(2.9.9)  

2. 编译前准备：
    拷贝对应的第三方库源码至esdk_obs_c\third_party_groupware\eSDK_Storage_Plugins目录下，编译对应的第三方库
2.1 Windows环境编译
        预先编译第三方库，将对应的动态库文件拷贝至esdk_obs_c\source\eSDK_OBS_API\eSDK_OBS_API_C++\bin对应目录下，将对应的静态库文件拷贝至esdk_obs_c\source\eSDK_OBS_API\eSDK_OBS_API_C++\lib对应目录下
