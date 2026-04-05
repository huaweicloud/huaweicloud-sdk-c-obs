#include <string.h>

#include "test_framework.h"
#include "test_mocks.h"
#include "test_subjects.h"

static CURL *dummy_curl = (CURL *)0x2345;

static obs_http_request_option make_options(void)
{
    obs_http_request_option options;
    memset(&options, 0, sizeof(options));
    options.ssl_verifyhost = OBS_SSL_VERIFYHOST_HOSTNAME;
    return options;
}

TEST(SetupCA, NoCaDisablesVerificationThenReturnsMtlsError)
{
    obs_http_request_option options = make_options();
    obs_status status;

    mock_reset();
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;

    status = setup_CheckCA(dummy_curl, &options, NULL);

    TEST_ASSERT_EQ_INT(OBS_STATUS_SSL_MissingBothCertAndKey, status);
    TEST_ASSERT_EQ_UINTPTR(0, mock_curl_call_value(CURLOPT_SSL_VERIFYPEER, 0));
    TEST_ASSERT_EQ_UINTPTR(0, mock_curl_call_value(CURLOPT_SSL_VERIFYHOST, 0));
}

TEST(SetupCA, ServerCertPathConfiguresVerifyAndCainfo)
{
    obs_http_request_option options = make_options();
    obs_status status;

    mock_reset();
    options.server_cert_path = "/tmp/ca.pem";

    status = setup_CheckCA(dummy_curl, &options, NULL);

    TEST_ASSERT_EQ_INT(OBS_STATUS_OK, status);
    TEST_ASSERT_EQ_UINTPTR(1, mock_curl_call_value(CURLOPT_SSL_VERIFYPEER, 0));
    TEST_ASSERT_EQ_UINTPTR(OBS_SSL_VERIFYHOST_HOSTNAME, mock_curl_call_value(CURLOPT_SSL_VERIFYHOST, 0));
    TEST_ASSERT_EQ_PTR(options.server_cert_path, mock_curl_call_value(CURLOPT_CAINFO, 0));
}

TEST(SetupCA, CertificateInfoConfiguresSslCtxHook)
{
    obs_http_request_option options = make_options();
    static char certificate_info[] = "PEM_DATA";
    obs_status status;

    mock_reset();
    options.certificate_info = certificate_info;

    status = setup_CheckCA(dummy_curl, &options, NULL);

    TEST_ASSERT_EQ_INT(OBS_STATUS_OK, status);
    TEST_ASSERT_EQ_PTR(certificate_info, mock_curl_call_value(CURLOPT_SSL_CTX_DATA, 0));
    TEST_ASSERT_EQ_PTR(sslctx_function, mock_curl_call_value(CURLOPT_SSL_CTX_FUNCTION, 0));
}

TEST(SetupCA, CaFailureShortCircuitsBeforeMtls)
{
    obs_http_request_option options = make_options();
    obs_status status;

    mock_reset();
    options.server_cert_path = "/tmp/ca.pem";
    options.client_auth_switch = OBS_CLIENT_AUTH_OPEN;
    mock_set_curl_result(CURLOPT_CAINFO, CURLE_BAD_FUNCTION_ARGUMENT);

    status = setup_CheckCA(dummy_curl, &options, NULL);

    TEST_ASSERT_EQ_INT(OBS_STATUS_FailedToIInitializeRequest, status);
    TEST_ASSERT_EQ_SIZE(0, mock_curl_call_count(CURLOPT_SSLCERT));
    TEST_ASSERT_EQ_SIZE(0, mock_curl_call_count(CURLOPT_SSLKEY));
}
