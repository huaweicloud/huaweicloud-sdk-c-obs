# SDK认证功能使用指南

## 1. 功能概述

本SDK支持四种认证方式的组合配置，满足不同场景的安全需求：

| 认证模式        | 服务端认证       | 客户端认证            |                   | 说明                         |
|---------|-------------------|---------------|------|---------|
| 对应参数 | certificate_info | client_auth_switch | gm_mode_switch |  |
| 标准TLS单向认证 | 国际CA证书路径 | OBS_CLIENT_AUTH_CLOSE | OBS_GM_MODE_CLOSE | 验证服务器证书，默认安全配置 |
| 标准TLS双向认证 | 国际CA证书路径 | OBS_CLIENT_AUTH_OPEN | OBS_GM_MODE_CLOSE | 双向证书验证，安全性更高 |
| 国密单向认证 | 国密CA证书路径 | OBS_CLIENT_AUTH_CLOSE | OBS_GM_MODE_OPEN | 国密算法验证服务器证书 |
| 国密双向认证 | 国密CA证书路径 | OBS_CLIENT_AUTH_OPEN | OBS_GM_MODE_OPEN | 国密算法+双向证书验证 |

**核心参数说明：**

- `certificate_info`：服务端证书认证路径配置
- `client_auth_switch`：客户端证书认证开关（OBS_CLIENT_AUTH_CLOSE=关闭，OBS_CLIENT_AUTH_OPEN=开启）
- `gm_mode_switch`：国密模式开关（OBS_GM_MODE_CLOSE=标准TLS，OBS_GM_MODE_OPEN=国密模式）

## 2. 快速配置

### 2.1 标准TLS单向认证（默认配置）

```c
obs_options options;
init_obs_options(&options);

// 服务端认证
options.bucket_options.certificate_info = "/path/to/ca.crt";
```

### 2.2 标准TLS双向认证

```c
init_obs_options(&options);

// 服务端认证
options.bucket_options.certificate_info = "/path/to/ca.crt";

// 客户端认证
options.request_options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
options.request_options.client_sign_cert_path = "/path/to/client.crt";
options.request_options.client_sign_key_path = "/path/to/client.key";

// 密码回调（可选）
options.request_options.password_callback = your_password_callback;
options.request_options.password_callback_context = your_context;
```

### 2.3 国密单向认证

```c
init_obs_options(&options);

// 服务端认证
options.bucket_options.certificate_info = "/path/to/gm_ca.crt";

// 开启国密模式
options.request_options.gm_mode_switch = OBS_GM_MODE_OPEN;
// 国密模式自动使用NTLSv1.1，无需手动设置ssl_version
```

### 2.4 国密双向认证

```c
init_obs_options(&options);

// 服务端认证
options.bucket_options.certificate_info = "/path/to/gm_ca.crt";

// 客户端认证
options.request_options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
options.request_options.client_sign_cert_path = "/path/to/gm_client.crt";
options.request_options.client_sign_key_path = "/path/to/gm_client.key";

// 开启国密模式
options.request_options.gm_mode_switch = OBS_GM_MODE_OPEN;
options.request_options.client_enc_cert_path = "/path/to/gm_client_enc.crt";  // 国密加密证书
options.request_options.client_enc_key_path = "/path/to/gm_client_enc.key";  // 国密加密私钥
// 密码回调（可选）
options.request_options.password_callback = your_password_callback;
options.request_options.password_callback_context = your_context;
```

### 2.5 综合配置示例

以下是一个包含所有高级功能的完整配置示例：

```c
// 1. 初始化obs_options结构
obs_options options;
init_obs_options(&options);

// 2. 配置基础认证参数
options.bucket_options.host_name = "obs.cn-north-4.myhuaweicloud.com";
options.bucket_options.bucket_name = "your-bucket-name";
options.bucket_options.access_key = "your-access-key";
options.bucket_options.secret_access_key = "your-secret-key";
options.bucket_options.token = "your-security-token";  // 临时认证token

// 3. 配置SSL/TLS参数
options.bucket_options.certificate_info = "/path/to/ca.crt";
options.request_options.ssl_version = 0;  // 让CURL自动选择

// 4. 配置客户端双向认证
options.request_options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
options.request_options.client_sign_cert_path = "/path/to/client.crt";
options.request_options.client_sign_key_path = "/path/to/client.key";

// 5. 配置密码回调
options.request_options.password_callback = my_password_callback;
options.request_options.password_callback_context = NULL;

// 6. 调用API
obs_status ret_status = OBS_STATUS_BUTT;
create_bucket(&option, NULL, NULL, NULL, &ret_status);
if (ret_status != OBS_STATUS_OK) {
    printf("API调用失败: %s\n", obs_get_status_name(ret_status));
    // 错误处理逻辑
}
```

### 2.6 国密模式完整配置

```c
// 国密模式完整配置示例
obs_options options;
init_obs_options(&options);

// 基础配置
options.bucket_options.host_name = "obs.cn-north-4.myhuaweicloud.com";
options.bucket_options.bucket_name = "your-bucket-name";
options.bucket_options.access_key = "your-access-key";
options.bucket_options.secret_access_key = "your-secret-key";

// 国密CA证书
options.bucket_options.certificate_info = "/path/to/gm_ca.crt";

// 开启国密模式
options.request_options.gm_mode_switch = OBS_GM_MODE_OPEN;

// 国密双向认证配置
options.request_options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
options.request_options.client_sign_cert_path = "/path/to/gm_client_sign.crt";
options.request_options.client_sign_key_path = "/path/to/gm_client_sign.key";
options.request_options.client_enc_cert_path = "/path/to/gm_client_enc.crt";   // 国密加密证书
options.request_options.client_enc_key_path = "/path/to/gm_client_enc.key";     // 国密加密私钥

// 注意：国密模式下ssl_version会自动设置为NTLSv1.1，无需手动设置
```

## 3. 配置参数说明

### 3.1 认证模式参数

| 参数名 | 类型 | 可选值 | 说明 |
|-------|------|-------|------|
| certificate_info | string | 自定义CA证书路径 | 服务端认证证书路径（bucket_options中） |
| server_cert_path | string | 自定义CA证书路径 | 服务端认证证书路径（request_options中，certificate_info优先级更高） |
| client_auth_switch | obs_client_auth_switch | OBS_CLIENT_AUTH_CLOSE=关闭, OBS_CLIENT_AUTH_OPEN=开启 | 是否启用客户端证书认证 |
| gm_mode_switch | obs_gm_mode_switch | OBS_GM_MODE_CLOSE=标准TLS, OBS_GM_MODE_OPEN=国密模式 | 选择使用国际标准TLS或国密算法 |

**obs_bucket_context 其他重要字段：**

| 参数名 | 类型 | 可选值 | 说明 |
|-------|------|-------|------|
| useCname | bool | true/false | 是否启用自定义域名访问 |
| uri_style | obs_uri_style | OBS_URI_STYLE_PATH=路径风格, OBS_URI_STYLE_VIRTUALHOST=虚拟主机风格 | URI访问风格 |
| protocol | obs_protocol | OBS_PROTOCOL_HTTP=HTTP, OBS_PROTOCOL_HTTPS=HTTPS | 通信协议选择 |
| storage_class | obs_storage_class | OBS_STORAGE_CLASS_STANDARD=标准, OBS_STORAGE_CLASS_IA=低频, OBS_STORAGE_CLASS_GLACIER=归档 | 存储类型 |
| bucket_type | obs_bucket_type | OBS_BUCKET_TYPE_OBJECT=对象桶, OBS_BUCKET_TYPE_POSIX=POSIX桶 | 桶类型 |

**说明：**
- `certificate_info` 和 `server_cert_path` 都用于服务端认证，但位于不同的结构体中
- 当同时配置两者时，`certificate_info` 优先级更高
- 建议优先使用 `certificate_info`，`server_cert_path` 作为备用方案
- `useCname` 开启后，需要配置正确的CNAME解析记录

### 3.2 客户端认证参数

| 参数名 | 类型 | 说明 |
|-------|------|------|
| client_sign_cert_path | string | 客户端签名证书路径（PEM格式） |
| client_sign_key_path | string | 客户端签名私钥路径 |
| password_callback | obs_password_cb_t | 私钥密码回调函数指针 |
| password_callback_context | void* | 回调函数上下文 |

### 3.3 国密模式额外参数

| 参数名 | 类型 | 说明 |
|-------|------|------|
| client_enc_cert_path | string | 客户端加密证书路径（国密模式必需） |
| client_enc_key_path | string | 客户端加密私钥路径（国密模式必需） |

### 3.4 TLS版本参数

| 参数名 | 类型 | 说明 |
|-------|------|------|
| ssl_version | int | SSL版本（CURL SSL版本常量） |

**说明：**
- 标准TLS模式：支持各种TLS版本，默认为0（由CURL自动选择）
- 国密模式：自动使用NTLSv1.1，无需手动设置ssl_version

### 3.5 SSL调试参数

| 参数名 | 类型 | 说明 |
|-------|------|------|
| ssl_cipher_list | string | SSL/TLS密码套件配置（自定义加密算法） |
| curl_log_verbose | bool | CURL日志详细输出开关（true=开启详细日志，false=关闭） |

**说明：**
- `ssl_cipher_list`：用于指定SSL/TLS连接使用的密码套件，格式为逗号分隔的字符串（如："ECDHE-RSA-AES128-GCM-SHA256,ECDHE-ECDSA-AES128-GCM-SHA256"）
- `curl_log_verbose`：开启后会输出详细的CURL调试信息，有助于排查SSL连接问题，但在生产环境中建议关闭

## 4. 常见问题

### Q1: 双向认证需要哪些文件？
标准TLS模式需要客户端签名证书（.crt或.pem）和客户端签名私钥（.key），均需为PEM格式。
国密模式除了签名证书和私钥外，还需要加密证书和加密私钥。

### Q2: 国密模式为何自动使用NTLSv1.1？
国密算法套件（如ECDHE-SM2-WITH-SM4-SM3）需要NTLSv1.1支持，因此SDK自动设置为NTLSv1.1。

### Q3: 如何验证配置是否正确？
调用API前，确保：
- 证书文件路径存在且可读
- 私钥文件路径存在且可读
- 密码回调函数正确设置（如有）
- 国密模式下加密证书和私钥都已配置

### Q4: 生产环境建议配置？
- 优先使用双向认证
- 国密模式确保使用Tongsuo编译的libcurl
- 确保证书文件路径正确且文件可读
- 定期轮换证书和密钥

### Q5: 密码回调函数如何使用？
当客户端私钥有密码保护时，需要设置password_callback：

**函数签名：**
```c
typedef int (*obs_password_cb_t)(void* context, char* buf, int buf_len);
```

**参数说明：**
- `context`：用户自定义上下文数据，通过`password_callback_context`传入
- `buf`：密码缓冲区，用于存储密码
- `buf_len`：缓冲区最大长度
- **返回值**：0表示成功，非0表示失败

**使用示例：**
```c
int my_password_callback(void* context, char* buf, int buf_len) {
    const char* my_password = "your_password";
    strncpy(buf, my_password, buf_len - 1);
    buf[buf_len - 1] = '\0';
    return 0;  // 成功返回0，失败返回非0
}

options.request_options.password_callback = my_password_callback;
options.request_options.password_callback_context = NULL;
```

**注意事项：**
- 密码回调函数会在SSL握手时被调用
- SDK会安全地处理密码内存，避免明文泄露
- 如果回调函数返回非0值，SSL连接将失败并返回`OBS_STATUS_SSL_PasswordCallbackError`

### Q6: 不同环境如何选择日志级别？

**环境建议：**

- **生产环境**：使用`Error`级别（LogLevel_Run=3）
- **测试环境**：使用`Warn`级别（LogLevel_Run=2）
- **开发环境**：临时使用`Info`级别（LogLevel_Run=1）

## 5. SSL配置注意事项

### 5.1 证书验证失败处理
- 当提供CA证书但验证失败时，API调用会返回错误
- 客户端证书设置失败时，会回退到单向认证并记录警告
- 建议在生产环境开启完整的SSL验证

### 5.2 SSL配置优先级
当同时配置certificate_info和server_cert_path时：
- 优先使用certificate_info进行CA证书验证
- server_cert_path作为备用方案
- 如果都没有提供，根据ssl_verify_peer/ssl_verify_host设置决定是否验证

### 5.3 国密模式特殊要求
- 国密模式必须使用Tongsuo编译的libcurl
- 国密模式自动使用NTLSv1.1，手动设置ssl_version可能无效
- 国密双向认证需要提供签名证书和加密证书两套

## 6. 编译说明

### 6.1 标准TLS模式编译

```bash
cd source/eSDK_OBS_API/eSDK_OBS_API_C++
cmake ..
make -j
```

### 6.2 国密模式编译（Tongsuo）

国密模式需要Tongsuo（中国密码）库支持，编译步骤如下：

#### 步骤1：编译Tongsuo

```bash
git clone https://github.com/Tongsuo-Project/Tongsuo
cd Tongsuo
./config --prefix=/opt/tongsuo enable-ntls
make -j && make install
```

**关键配置：**

- `--prefix=/opt/tongsuo`：安装路径
- `enable-ntls`：启用国密NTLS协议支持

#### 步骤2：编译curl（国密libcurl）

```bash
git clone https://github.com/Tongsuo-Project/curl.git
cd curl
git apply tongsuo.patch              # 应用铜锁补丁
autoreconf -fi                       # 重新生成configure

# 关键：设置rpath确保运行时能找到Tongsuo库
LDFLAGS=-Wl,-rpath=/opt/tongsuo/lib64 \
./configure --enable-warnings --enable-werror \
            --with-openssl=/opt/tongsuo

make -j && make install
```

**关键要点：**
- 必须先构建Tongsuo，curl依赖Tongsuo提供国密功能
- `LDFLAGS=-Wl,-rpath=/opt/tongsuo/lib64`：避免运行时找不到libcrypto等库
- `--with-openssl=/opt/tongsuo`：指定使用Tongsuo而非系统OpenSSL

#### 步骤3：编译OBS SDK

关于编译请参考华为云官网文档，在编译时SDK不会依赖Tongsuo curl，只有运行时，并且开启国密开关后才会判断。

https://support.huaweicloud.com/sdk-c-devg-obs/obs_20_0004.html

## 7. 错误码说明

### 7.1 SSL相关错误码

| 错误码 | 说明 | 解决方案 |
|-------|------|----------|
| OBS_STATUS_SSL_MissingBothCertAndKey | 双向认证开启但客户端证书和私钥都未提供 | 检查`client_sign_cert_path`和`client_sign_key_path`是否正确配置 |
| OBS_STATUS_SSL_CertNotFound | 双向认证开启但客户端证书未提供 | 确认证书文件路径存在且可读，检查`client_sign_cert_path`配置 |
| OBS_STATUS_SSL_KeyNotFound | 双向认证开启但客户端私钥未提供 | 确认私钥文件路径存在且可读，检查`client_sign_key_path`配置 |
| OBS_STATUS_SSL_PasswordCallbackError | 密码回调函数失败 | 检查密码回调函数实现，确保返回值为0表示成功 |
| OBS_STATUS_SSL_PasswordConfigError | 密码配置错误 | 确认私钥密码正确，检查密码回调函数是否正确返回密码 |
| OBS_STATUS_SSL_CipherConfigError | SSL密码套件配置错误 | 检查`ssl_cipher_list`配置，确保密码套件格式正确 |
| OBS_STATUS_SSL_VersionConfigError | SSL版本配置错误 | 检查`ssl_version`配置，确保使用支持的SSL版本 |

### 7.2 国密相关错误码

| 错误码 | 说明 | 解决方案 |
|-------|------|----------|
| OBS_STATUS_GM_MissingDualCertPath | 国密模式缺少加密证书配置 | 国密模式需要同时配置签名证书和加密证书，检查`client_enc_cert_path`和`client_enc_key_path` |
| OBS_STATUS_GM_TongsuoNotSupported | 国密模式需要Tongsuo编译的libcurl | 确保使用Tongsuo编译的libcurl库，参考第6.2节编译说明 |
| OBS_STATUS_GM_UnsupportedSSLVersion | 国密模式只支持NTLSv1.1版本 | 国密模式自动使用NTLSv1.1，不要手动设置`ssl_version`为其他值 |
| OBS_STATUS_GM_CipherConfigError | 国密证书配置错误 | 检查国密证书和私钥文件格式，确保使用正确的国密证书 |
| OBS_STATUS_GM_VersionConfigError | 国密SSL版本配置错误 | 确保使用支持NTLSv1.1的libcurl版本 |
