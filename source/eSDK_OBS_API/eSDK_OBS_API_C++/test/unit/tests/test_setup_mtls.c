#include <string.h>

#include "test_framework.h"
#include "test_mocks.h"
#include "test_subjects.h"

static CURL *dummy_curl = (CURL *)0x1234;

static obs_http_request_option make_options(void)
{
    obs_http_request_option options;
    memset(&options, 0, sizeof(options));
    options.ssl_verifyhost = OBS_SSL_VERIFYHOST_HOSTNAME;
    return options;
}

static int password_callback_success(void *context, char *password_buffer, size_t buffer_size)
{
    (void)context;
    if (buffer_size == 0) {
        return -1;
    }
    strncpy(password_buffer, "secret", buffer_size - 1);
    password_buffer[buffer_size - 1] = '\0';
    return 0;
}

TEST(SetupMtlsStandard, ClientAuthCloseSkipsClientCertConfig)
{
    obs_http_request_option options = make_options();
    obs_status status;

    mock_reset();
    options.client_auth_switch = OBS_CLIENT_AUTH_CLOSE;
    options.ssl_cipher_list = "ECDHE-RSA-AES256-GCM-SHA384";

    status = setup_client_certificate(dummy_curl, &options);

    TEST_ASSERT_EQ_INT(OBS_STATUS_OK, status);
    TEST_ASSERT_EQ_SIZE(0, mock_curl_call_count(CURLOPT_SSLCERT));
    TEST_ASSERT_EQ_SIZE(0, mock_curl_call_count(CURLOPT_SSLKEY));
    TEST_ASSERT_EQ_SIZE(1, mock_curl_call_count(CURLOPT_SSL_CIPHER_LIST));
}

TEST(SetupMtlsStandard, MissingBothCertAndKey)
{
    obs_http_request_option options = make_options();

    mock_reset();
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;

    TEST_ASSERT_EQ_INT(OBS_STATUS_SSL_MissingBothCertAndKey, setup_client_certificate(dummy_curl, &options));
}

TEST(SetupMtlsStandard, MissingSignCert)
{
    obs_http_request_option options = make_options();

    mock_reset();
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    options.client_sign_key_path = "/tmp/client.key";

    TEST_ASSERT_EQ_INT(OBS_STATUS_SSL_CertNotFound, setup_client_certificate(dummy_curl, &options));
}

TEST(SetupMtlsStandard, MissingSignKey)
{
    obs_http_request_option options = make_options();

    mock_reset();
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    options.client_sign_cert_path = "/tmp/client.crt";

    TEST_ASSERT_EQ_INT(OBS_STATUS_SSL_KeyNotFound, setup_client_certificate(dummy_curl, &options));
}

TEST(SetupMtlsStandard, SuccessConfiguresSignCertAndKey)
{
    obs_http_request_option options = make_options();
    obs_status status;

    mock_reset();
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    options.client_sign_cert_path = "/tmp/client.crt";
    options.client_sign_key_path = "/tmp/client.key";

    status = setup_client_certificate(dummy_curl, &options);

    TEST_ASSERT_EQ_INT(OBS_STATUS_OK, status);
    TEST_ASSERT_EQ_SIZE(1, mock_curl_call_count(CURLOPT_SSLCERT));
    TEST_ASSERT_EQ_SIZE(1, mock_curl_call_count(CURLOPT_SSLKEY));
    TEST_ASSERT_EQ_PTR(options.client_sign_cert_path, mock_curl_call_value(CURLOPT_SSLCERT, 0));
    TEST_ASSERT_EQ_PTR(options.client_sign_key_path, mock_curl_call_value(CURLOPT_SSLKEY, 0));
}

TEST(SetupMtlsStandard, PasswordCallbackConfiguresSslCtxHooks)
{
    obs_http_request_option options = make_options();
    obs_status status;

    mock_reset();
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    options.client_sign_cert_path = "/tmp/client.crt";
    options.client_sign_key_path = "/tmp/client.key";
    options.password_callback = password_callback_success;

    status = setup_client_certificate(dummy_curl, &options);

    TEST_ASSERT_EQ_INT(OBS_STATUS_OK, status);
    TEST_ASSERT_EQ_SIZE(1, mock_curl_call_count(CURLOPT_SSL_CTX_FUNCTION));
    TEST_ASSERT_EQ_SIZE(1, mock_curl_call_count(CURLOPT_SSL_CTX_DATA));
    TEST_ASSERT_EQ_PTR(ssl_password_callback, mock_curl_call_value(CURLOPT_SSL_CTX_FUNCTION, 0));
    TEST_ASSERT_EQ_PTR(&options, mock_curl_call_value(CURLOPT_SSL_CTX_DATA, 0));
}

TEST(SetupMtlsStandard, CipherConfigFailureReturnsExactStatus)
{
    obs_http_request_option options = make_options();

    mock_reset();
    options.ssl_cipher_list = "TLS_AES_256_GCM_SHA384";
    mock_set_curl_result(CURLOPT_SSL_CIPHER_LIST, CURLE_BAD_FUNCTION_ARGUMENT);

    TEST_ASSERT_EQ_INT(OBS_STATUS_SSL_CipherConfigError, setup_client_certificate(dummy_curl, &options));
}

TEST(SetupMtlsStandard, VersionConfigFailureReturnsExactStatus)
{
    obs_http_request_option options = make_options();

    mock_reset();
    options.ssl_version = CURL_SSLVERSION_TLSv1_2;
    mock_set_curl_result(CURLOPT_SSLVERSION, CURLE_BAD_FUNCTION_ARGUMENT);

    TEST_ASSERT_EQ_INT(OBS_STATUS_SSL_VersionConfigError, setup_client_certificate(dummy_curl, &options));
}

TEST(SetupMtlsGM, MissingDualCertPath)
{
#ifdef CURL_SSLVERSION_NTLSv1_1
    obs_http_request_option options = make_options();

    mock_reset();
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    options.gm_mode_switch = OBS_GM_MODE_OPEN;
    options.client_sign_cert_path = "/tmp/sign.crt";
    options.client_sign_key_path = "/tmp/sign.key";

    TEST_ASSERT_EQ_INT(OBS_STATUS_GM_MissingDualCertPath, setup_client_certificate(dummy_curl, &options));
#else
    TEST_SKIP("current libcurl does not expose NTLS GM support");
#endif
}

TEST(SetupMtlsGM, SuccessConfiguresDualCertAndDefaultVersion)
{
#ifdef CURL_SSLVERSION_NTLSv1_1
    obs_http_request_option options = make_options();
    obs_status status;

    mock_reset();
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    options.gm_mode_switch = OBS_GM_MODE_OPEN;
    options.client_sign_cert_path = "/tmp/sign.crt";
    options.client_sign_key_path = "/tmp/sign.key";
    options.client_enc_cert_path = "/tmp/enc.crt";
    options.client_enc_key_path = "/tmp/enc.key";

    status = setup_client_certificate(dummy_curl, &options);

    TEST_ASSERT_EQ_INT(OBS_STATUS_OK, status);
    TEST_ASSERT_EQ_SIZE(1, mock_curl_call_count(CURLOPT_SSLSIGNCERT));
    TEST_ASSERT_EQ_SIZE(1, mock_curl_call_count(CURLOPT_SSLSIGNKEY));
    TEST_ASSERT_EQ_SIZE(1, mock_curl_call_count(CURLOPT_SSLENCCERT));
    TEST_ASSERT_EQ_SIZE(1, mock_curl_call_count(CURLOPT_SSLENCKEY));
    TEST_ASSERT_EQ_UINTPTR(CURL_SSLVERSION_NTLSv1_1, mock_curl_call_value(CURLOPT_SSLVERSION, 0));
#else
    TEST_SKIP("current libcurl does not expose NTLS GM support");
#endif
}

TEST(SetupMtlsGM, UnsupportedSslVersionReturnsExactStatus)
{
#ifdef CURL_SSLVERSION_NTLSv1_1
    obs_http_request_option options = make_options();

    mock_reset();
    options.gm_mode_switch = OBS_GM_MODE_OPEN;
    options.ssl_version = CURL_SSLVERSION_TLSv1_2;

    TEST_ASSERT_EQ_INT(OBS_STATUS_GM_UnsupportedSSLVersion, setup_client_certificate(dummy_curl, &options));
#else
    TEST_SKIP("current libcurl does not expose NTLS GM support");
#endif
}

TEST(SetupMtlsGM, CipherConfigFailureReturnsExactStatus)
{
#ifdef CURL_SSLVERSION_NTLSv1_1
    obs_http_request_option options = make_options();

    mock_reset();
    options.gm_mode_switch = OBS_GM_MODE_OPEN;
    options.ssl_cipher_list = "ECDHE-SM2-WITH-SM4-SM3";
    mock_set_curl_result(CURLOPT_SSL_CIPHER_LIST, CURLE_BAD_FUNCTION_ARGUMENT);

    TEST_ASSERT_EQ_INT(OBS_STATUS_GM_CipherConfigError, setup_client_certificate(dummy_curl, &options));
#else
    TEST_SKIP("current libcurl does not expose NTLS GM support");
#endif
}

TEST(SetupMtlsGM, TongsuoNotSupportedWithoutNtls)
{
#ifndef CURL_SSLVERSION_NTLSv1_1
    obs_http_request_option options = make_options();

    mock_reset();
    options.gm_mode_switch = OBS_GM_MODE_OPEN;

    TEST_ASSERT_EQ_INT(OBS_STATUS_GM_TongsuoNotSupported, setup_client_certificate(dummy_curl, &options));
#else
    TEST_SKIP("current libcurl exposes NTLS GM support");
#endif
}

TEST(SetupClientCertificateGM, MissingEncryptionCert)
{
#ifdef CURL_SSLVERSION_NTLSv1_1
    obs_http_request_option options = make_options();

    mock_reset();
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    options.gm_mode_switch = OBS_GM_MODE_OPEN;
    options.client_sign_cert_path = "/tmp/sign.crt";
    options.client_sign_key_path = "/tmp/sign.key";

    TEST_ASSERT_EQ_INT(OBS_STATUS_GM_MissingDualCertPath, setup_client_certificate(dummy_curl, &options));
#else
    TEST_SKIP("current libcurl does not expose NTLS GM support");
#endif
}

TEST(SetupClientCertificateGM, MissingEncryptionKey)
{
#ifdef CURL_SSLVERSION_NTLSv1_1
    obs_http_request_option options = make_options();

    mock_reset();
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    options.gm_mode_switch = OBS_GM_MODE_OPEN;
    options.client_sign_cert_path = "/tmp/sign.crt";
    options.client_sign_key_path = "/tmp/sign.key";
    options.client_enc_cert_path = "/tmp/enc.crt";

    TEST_ASSERT_EQ_INT(OBS_STATUS_GM_MissingDualCertPath, setup_client_certificate(dummy_curl, &options));
#else
    TEST_SKIP("current libcurl does not expose NTLS GM support");
#endif
}

TEST(SetupClientCertificateGM, DualCertConfigSuccess)
{
#ifdef CURL_SSLVERSION_NTLSv1_1
    obs_http_request_option options = make_options();
    obs_status status;

    mock_reset();
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    options.gm_mode_switch = OBS_GM_MODE_OPEN;
    options.client_sign_cert_path = "/tmp/sign.crt";
    options.client_sign_key_path = "/tmp/sign.key";
    options.client_enc_cert_path = "/tmp/enc.crt";
    options.client_enc_key_path = "/tmp/enc.key";

    status = setup_client_certificate(dummy_curl, &options);

    TEST_ASSERT_EQ_INT(OBS_STATUS_OK, status);
    TEST_ASSERT_EQ_SIZE(1, mock_curl_call_count(CURLOPT_SSLSIGNCERT));
    TEST_ASSERT_EQ_SIZE(1, mock_curl_call_count(CURLOPT_SSLSIGNKEY));
    TEST_ASSERT_EQ_SIZE(1, mock_curl_call_count(CURLOPT_SSLENCCERT));
    TEST_ASSERT_EQ_SIZE(1, mock_curl_call_count(CURLOPT_SSLENCKEY));
    TEST_ASSERT_EQ_PTR(options.client_sign_cert_path, mock_curl_call_value(CURLOPT_SSLSIGNCERT, 0));
    TEST_ASSERT_EQ_PTR(options.client_sign_key_path, mock_curl_call_value(CURLOPT_SSLSIGNKEY, 0));
    TEST_ASSERT_EQ_PTR(options.client_enc_cert_path, mock_curl_call_value(CURLOPT_SSLENCCERT, 0));
    TEST_ASSERT_EQ_PTR(options.client_enc_key_path, mock_curl_call_value(CURLOPT_SSLENCKEY, 0));
#else
    TEST_SKIP("current libcurl does not expose NTLS GM support");
#endif
}

TEST(SetupClientCertificateGM, CipherConfigFailureReturnsExactStatus)
{
#ifdef CURL_SSLVERSION_NTLSv1_1
    obs_http_request_option options = make_options();

    mock_reset();
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    options.gm_mode_switch = OBS_GM_MODE_OPEN;
    options.client_sign_cert_path = "/tmp/sign.crt";
    options.client_sign_key_path = "/tmp/sign.key";
    options.client_enc_cert_path = "/tmp/enc.crt";
    options.client_enc_key_path = "/tmp/enc.key";
    mock_set_curl_result(CURLOPT_SSLSIGNCERT, CURLE_BAD_FUNCTION_ARGUMENT);

    TEST_ASSERT_EQ_INT(OBS_STATUS_GM_CipherConfigError, setup_client_certificate(dummy_curl, &options));
#else
    TEST_SKIP("current libcurl does not expose NTLS GM support");
#endif
}
