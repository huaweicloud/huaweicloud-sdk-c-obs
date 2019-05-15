OBS对象存储系列开放的C SDK接口，提供V2鉴权、V4鉴权、桶操作、对象操作接口等能力。

V1.5.60
1、修复putobject接口在上传过程中断开网卡导致调用程序无响应的bug。

V1.5.70
1、修复多段上传合并时，概率发生的core dump的bug。
2、Windows版本的SDK中将GetObject接口名称修改为getObject，DeleteObject接口名称修改为deleteObject，Linux版本SDK保持不变。
3、修改证书校验功能，在调用接口时，如不传入证书则不进行证书校验，传入证书时则进行证书校验。
4、升级openssl库。
5、取消通信时对hostname的验证功能。
6、支持中文key
7、设置和查询桶的生命周期，支持多条rule规则
8、设置和查询桶的Cros配置，支持多条rule规则
9、日志库增加可以设置文件权限功能
10、复制对象接口变更，增加nIsCopy参数（0表示Replace，>0表示Copy）
11、修复多线程调用中，libcurl出现崩溃的情况

V2.1.00
1、修复日志不断初始化中，未关闭/proc/self/maps的文件句柄问题。
2、listVersions增加响应参数。
3、升级openssl、libcurl、pcre、libxml等开源库。

V2.1.10
1、修复CodeDEX检查出的warning与error
2、进行了安全函数整改
3、openssl升级到openssl-1.0.2k版本

V2.1.11
1、优化linux UTF8处理方法，由setlocale更改为iconv。
2、增加温冷特性。
3、升级curl、zlib、libiconv等开源库。
4、解决OBS eSDK死锁的问题。

V2.1.12
1、增加桶标签功能、V4临时鉴权功能、大文件自动分割功能
2、解决中文转码导致内存占用增大的问题

V2.1.13
1、增加虚拟主机方式访问功能、V2临时鉴权功能、对象级别温冷功能
2、解决对象名包含特殊字符'~'，'@'，' ','*'时V4鉴权失败的问题

V2.1.14
1、增加断点续传上传文件功能、服务端加密功能
2、支持windows x64
3、升级开源库，openssl从openssl-1.0.2k升级到openssl-1.0.2m, libxml2从libxml2-2.9.4升级到libxml2-2.9.5

V2.1.15
1、增加断点续传并发下载功能、设置和查询桶的生命周期，支持trasition、获取对象内容，支持图片转码功能
2、升级开源库，libxml2从libxml2-2.9.5升级到libxml2-2.9.7
3、User-Agent头域修改为obs-sdk-c/x.x.x

