#include <string.h>

#include "test_framework.h"
#include "test_mocks.h"
#include "test_subjects.h"

// 注意：实际的密码回调逻辑在request.c中的configure_password_callback函数中
// 由于该函数是静态的，我们通过setup_client_certificate来间接测试密码回调功能

static int g_password_callback_calls = 0;

static int password_callback_success(void *context, char *password_buffer, size_t buffer_size)
{
    (void)context;
    ++g_password_callback_calls;
    strncpy(password_buffer, "s3cr3t", buffer_size - 1);
    password_buffer[buffer_size - 1] = '\0';
    return 0;
}

static int password_callback_failure(void *context, char *password_buffer, size_t buffer_size)
{
    (void)context;
    (void)password_buffer;
    (void)buffer_size;
    ++g_password_callback_calls;
    return -1;
}

static obs_http_request_option make_options(void)
{
    obs_http_request_option options;
    memset(&options, 0, sizeof(options));
    options.ssl_verifyhost = OBS_SSL_VERIFYHOST_HOSTNAME;
    return options;
}

TEST(PasswordCallback, SuccessWithCertificates)
{
    obs_http_request_option options = make_options();
    obs_status status;

    mock_reset();
    g_password_callback_calls = 0;
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    options.client_sign_cert_path = "/tmp/client.crt";
    options.client_sign_key_path = "/tmp/client.key";
    options.password_callback = password_callback_success;

    status = setup_client_certificate((CURL *)0x1234, &options);

    // 密码回调应该被调用一次
    TEST_ASSERT_EQ_INT(1, g_password_callback_calls);
    // 配置应该成功
    TEST_ASSERT_EQ_INT(OBS_STATUS_OK, status);
}

TEST(PasswordCallback, FailureWithCertificates)
{
    obs_http_request_option options = make_options();
    obs_status status;

    mock_reset();
    g_password_callback_calls = 0;
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    options.client_sign_cert_path = "/tmp/client.crt";
    options.client_sign_key_path = "/tmp/client.key";
    options.password_callback = password_callback_failure;

    status = setup_client_certificate((CURL *)0x1234, &options);

    // 密码回调应该被调用一次
    TEST_ASSERT_EQ_INT(1, g_password_callback_calls);
    // 配置应该失败
    TEST_ASSERT_EQ_INT(OBS_STATUS_SSL_PasswordCallbackError, status);
}

TEST(PasswordCallbackGM, SuccessWithGMCertificates)
{
#ifdef CURL_SSLVERSION_NTLSv1_1
    obs_http_request_option options = make_options();
    obs_status status;

    mock_reset();
    g_password_callback_calls = 0;
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    options.gm_mode_switch = OBS_GM_MODE_OPEN;
    options.client_sign_cert_path = "/tmp/sign.crt";
    options.client_sign_key_path = "/tmp/sign.key";
    options.client_enc_cert_path = "/tmp/enc.crt";
    options.client_enc_key_path = "/tmp/enc.key";
    options.password_callback = password_callback_success;

    status = setup_client_certificate((CURL *)0x1234, &options);

    // 密码回调应该被调用一次
    TEST_ASSERT_EQ_INT(1, g_password_callback_calls);
    // 配置应该成功
    TEST_ASSERT_EQ_INT(OBS_STATUS_OK, status);
#else
    TEST_SKIP("current libcurl does not expose NTLS GM support");
#endif
}

TEST(PasswordCallbackGM, FailureWithGMCertificates)
{
#ifdef CURL_SSLVERSION_NTLSv1_1
    obs_http_request_option options = make_options();
    obs_status status;

    mock_reset();
    g_password_callback_calls = 0;
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    options.gm_mode_switch = OBS_GM_MODE_OPEN;
    options.client_sign_cert_path = "/tmp/sign.crt";
    options.client_sign_key_path = "/tmp/sign.key";
    options.client_enc_cert_path = "/tmp/enc.crt";
    options.client_enc_key_path = "/tmp/enc.key";
    options.password_callback = password_callback_failure;

    status = setup_client_certificate((CURL *)0x1234, &options);

    // 密码回调应该被调用一次
    TEST_ASSERT_EQ_INT(1, g_password_callback_calls);
    // 配置应该失败
    TEST_ASSERT_EQ_INT(OBS_STATUS_SSL_PasswordCallbackError, status);
#else
    TEST_SKIP("current libcurl does not expose NTLS GM support");
#endif
}