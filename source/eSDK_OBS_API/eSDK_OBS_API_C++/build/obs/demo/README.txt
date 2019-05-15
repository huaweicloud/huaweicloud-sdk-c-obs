server.jks与client.crt为测试用证书。

请将server.jks导入到服务器的ISM上，路径为https://ip:28443/app/html/main.html#/rack	（ip为服务器的ip地址）。

请保证client.crt的名字不变，如果需要使用自制的证书来验证，请将新证书命名成client.crt，然后替换。

请在set.bat脚本中修改环境变量值，在当前目录下执行set.bat脚本会设置临时环境变量，

set.bat脚本同时将sln目录下编译产生的的exe文件及运行所需的库文件拷贝到bin目录下。



