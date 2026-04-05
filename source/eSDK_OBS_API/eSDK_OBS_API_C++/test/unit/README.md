# OBS C SDK Unit Test Guide

## 1. 目录定位

`test/unit` 目录用于承载 OBS C SDK 中 SSL/TLS 相关逻辑的单元测试，当前重点覆盖：

- `setup_mtls`
- `setup_CA`
- `setup_CheckCA`
- `ssl_password_callback`

这套测试的目标不是替代 `test/e2e`，而是尽量在**不依赖真实 OBS 服务端、不发起真实网络请求**的前提下，验证新增 SSL/国密/双向认证逻辑的分支行为、错误码返回和 curl/OpenSSL 配置是否符合预期。

## 2. 设计思路

当前单测不是“测桩函数”，而是“测真实实现”：

- `src/real_impl.c` 通过宏重定向，把真实的 `src/request.c` 和 `src/ssl_password_callback.c` 编译进测试目标。
- `src/test_mocks.c` 提供 curl/OpenSSL/log/sslctx 相关 mock，用来拦截并记录关键调用。
- `include/pcre.h` 是测试侧的兼容头，用于解决本地没有安装 PCRE 开发头文件时的编译问题；本轮测试路径本身并不依赖 PCRE 功能。

因此，这套 unit test 更接近“真实代码 + 可控外部依赖”的模式，适合验证配置分支、错误码和 option 设置行为。

## 3. 目录结构说明

```text
test/unit/
├── CMakeLists.txt
├── README.md
├── main.c
├── include/
│   └── pcre.h
├── src/
│   ├── real_impl.c
│   ├── test_framework.c
│   ├── test_framework.h
│   ├── test_mocks.c
│   ├── test_mocks.h
│   └── test_subjects.h
└── tests/
    ├── test_password_callback.c
    ├── test_setup_ca.c
    └── test_setup_mtls.c
```

各文件职责如下：

- `CMakeLists.txt`  
  单测工程入口，负责依赖查找、编译参数、链接参数和 `ctest` 注册。

- `main.c`  
  单测主程序入口，支持 `--verbose` 和 `--filter=...`。

- `include/pcre.h`  
  测试兼容头，避免本地缺少 PCRE 头文件时阻塞 `request.c` 的真实编译。

- `src/real_impl.c`  
  通过宏重定向把真实被测源码拉进测试目标，是“真实单测”的核心入口。

- `src/test_framework.h` / `src/test_framework.c`  
  轻量测试框架，提供测试注册、断言、失败跳转和结果汇总能力。

- `src/test_mocks.h` / `src/test_mocks.c`  
  提供 mock 和调用记录能力，主要覆盖：
  - `curl_easy_setopt`
  - `curl_easy_strerror`
  - `SSL_CTX_set_default_passwd_cb`
  - `SSL_CTX_set_default_passwd_cb_userdata`
  - `OPENSSL_cleanse`
  - `sslctx_function`

- `src/test_subjects.h`  
  测试侧私有声明，用来引用 `setup_mtls/setup_CA/setup_CheckCA` 等内部实现。

- `tests/test_setup_mtls.c`  
  覆盖标准 TLS 和 GM 模式下的 mTLS 配置逻辑。

- `tests/test_setup_ca.c`  
  覆盖 CA 证书配置、证书信息注入、关闭校验和短路返回逻辑。

- `tests/test_password_callback.c`  
  覆盖密码延迟获取、失败返回、缓冲区清理和 lazy-loading 标记逻辑。

## 4. 依赖要求

构建这套单测需要以下基础依赖：

- CMake 3.16 或更高
- `pkg-config`
- OpenSSL 开发包
- libcurl 开发包

当前 `CMakeLists.txt` 通过 `pkg-config` 查找 `openssl` 和 `libcurl`，因此本地环境需要能正确执行：

```bash
pkg-config --cflags --libs openssl libcurl
```

## 5. 编译方式

推荐在仓库根目录执行：

```bash
cmake -S source/eSDK_OBS_API/eSDK_OBS_API_C++/test/unit -B /tmp/obs_ssl_unit_build
cmake --build /tmp/obs_ssl_unit_build -j4
```

说明：

- `-S` 指向当前 `unit` 目录。
- `-B` 可以放到任意本地构建目录，这里使用 `/tmp/obs_ssl_unit_build` 作为示例。
- 构建参数中已默认启用：
  - `-DUNIT_TEST`
  - `-ffunction-sections`
  - `-fdata-sections`
- 链接时会自动根据平台选择：
  - macOS: `-Wl,-dead_strip`
  - Linux: `-Wl,--gc-sections`

这样做的目的是在拉进真实 `request.c` 的同时，尽量减小无关代码段对单测链接的影响。

## 6. 运行方式

### 6.1 运行全部测试

```bash
/tmp/obs_ssl_unit_build/obs_ssl_unit_tests
```

### 6.2 显示详细输出

```bash
/tmp/obs_ssl_unit_build/obs_ssl_unit_tests --verbose
```

### 6.3 只运行某一类测试

```bash
/tmp/obs_ssl_unit_build/obs_ssl_unit_tests --filter=SetupCA
/tmp/obs_ssl_unit_build/obs_ssl_unit_tests --filter=SetupMtls
/tmp/obs_ssl_unit_build/obs_ssl_unit_tests --filter=PasswordCallback
```

### 6.4 通过 ctest 运行

```bash
ctest --test-dir /tmp/obs_ssl_unit_build --output-on-failure
```

## 7. 当前覆盖范围

### 7.1 `test_setup_mtls.c`

覆盖以下场景：

- `client_auth_switch = CLOSE` 时跳过客户端证书配置
- 标准双向认证缺少签名证书/私钥时返回精确错误码
- 标准双向认证成功时配置 `CURLOPT_SSLCERT` 和 `CURLOPT_SSLKEY`
- 配置密码回调时挂接 `CURLOPT_SSL_CTX_FUNCTION` / `CURLOPT_SSL_CTX_DATA`
- 标准 TLS 下 `ssl_cipher_list` / `ssl_version` 配置失败时返回精确状态
- GM 模式下的双证书配置、GM cipher/version 错误
- 当前 libcurl 不支持 NTLS 时返回 `OBS_STATUS_GM_TongsuoNotSupported`

### 7.2 `test_setup_ca.c`

覆盖以下场景：

- 没有 CA 信息时关闭 `SSL_VERIFYPEER` / `SSL_VERIFYHOST`
- 配置 `server_cert_path` 时设置 `CURLOPT_CAINFO`
- 配置 `certificate_info` 时设置 `CURLOPT_SSL_CTX_DATA` 和 `CURLOPT_SSL_CTX_FUNCTION`
- CA 设置失败时短路返回，不继续进入 mTLS 配置

### 7.3 `test_password_callback.c`

覆盖以下场景：

- `userdata = NULL`
- 未配置 `password_callback`
- 成功获取密码并设置 OpenSSL 回调上下文
- 回调失败时返回 `CURLE_SSL_CERTPROBLEM`
- 调用 `OPENSSL_cleanse` 清理敏感缓冲区
- `is_password_lazy_loading_enabled` 开关行为

## 8. 当前环境下的执行结果

在当前开发环境下，这套单测已经可以正常编译和运行。

一个典型结果是：

- `18 passed`
- `0 failed`
- `4 skipped`

其中 `skipped` 的测试来自 GM 细分场景。原因是当前本机 libcurl 未暴露 `CURL_SSLVERSION_NTLSv1_1`，因此这些用例会按预期跳过，只保留 “GM/Tongsuo 不支持” 分支验证。

## 9. 已知限制

- 这是一套 unit test，不进行真实网络访问，也不验证和 OBS 服务端的实际握手。
- 若要覆盖完整 GM 双证书配置分支，本地 libcurl 需要具备 NTLS/Tongsuo 能力。
- 当前编译过程中可能看到来自 `request.c` / `request.h` 的已有 warning，这些 warning 不影响本单测构建和执行。
- `test_subjects.h` 暴露的是内部实现声明，仅供测试使用，不代表这些函数是对外公开 API。

## 10. 如何扩展新的单测

如果后续继续扩展 SSL/国密相关逻辑，建议按下面的方式维护：

1. 先判断新增逻辑属于哪一层：
   - mTLS 逻辑放到 `test_setup_mtls.c`
   - CA/校验逻辑放到 `test_setup_ca.c`
   - 密码/回调逻辑放到 `test_password_callback.c`

2. 如果新增逻辑依赖新的 curl/OpenSSL 接口：
   - 先在 `test_mocks.h/.c` 中补 mock 和调用记录能力
   - 再在用例里增加对应断言

3. 如果真实源码新增了新的外部头依赖，而测试路径并不真正使用：
   - 可以参考 `include/pcre.h` 的做法，在 `include/` 下补测试兼容头

4. 如果新增的是全新的测试主题：
   - 在 `tests/` 下新建一个独立测试文件
   - 在 `CMakeLists.txt` 中把它加入 `obs_ssl_unit_tests` 目标

## 11. 什么时候用 unit，什么时候用 e2e

建议这样分工：

- `unit`  
  用来验证函数级分支、错误码、option 配置、回调挂接和边界条件。

- `e2e`  
  用来验证证书、域名、服务端握手、真实 OBS 访问链路和环境配置。

如果一个问题能在不联网的情况下复现并判断对错，优先补到 `unit`。  
如果问题依赖真实证书、真实域名或真实服务端行为，再放到 `e2e`。
