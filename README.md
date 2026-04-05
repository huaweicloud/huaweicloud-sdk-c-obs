# huaweicloud-sdk-c-obs

## Release Notes

### Version 3.24.12

**New Features:**
1. Supports the recycle bin feature of parallel file systems.
2. Supports obtaining the bucket type in bucket metadata.

**Resolved Issues:**
1. Optimized some logic for resumable upload and download.
2. Resolved the issue where some variables are not correctly initialized.

---

### Version 3.24.3

**New Features:**
1. Supports access using a custom domain name.
2. Supports setting, obtaining, and deleting access labels.

**Resolved Issues:**
1. Resolved the issue where the placeholder formats of some print functions are incorrect.
2. Replaced unsafe functions with safe functions.
3. Resolved the issue where an error is reported when the header field cannot be obtained.
4. Resolved the issue where the bucket policy length is too short.
5. Resolved memory leakage.
6. Resolved the issue where the length of the log path in the memory is inconsistent with the actual length.

---

### Version 3.23.9

**New Features:**
1. The log file name in Windows supports wide characters.
2. Supports setting an object expiration time during resumable upload.
3. Resolved the issue where the status code returned for resumable upload is incorrect.
4. Resolved the issue where the SSE-C setting does not take effect during resumable upload.

---

### Version 3.23.3

**New Features:**
1. Wide characters are supported in Windows.
2. Components such as OpenSSL, cURL, and PCRE are upgraded.
3. The log4cpp is upgraded to fix some compilation alarms.

---

### Version 3.22.5

**New Features:**
1. Supports obtaining the progress of uploading objects using put_object.
2. The retry feature is supported in some cases, such as network timeout and server errors.

**Resolved Issues:**
1. Resolved the issue where MD5 verification fails when the encryption key contains \0 during object upload.

---

### Version 3.22.3

**New Features:**
1. Resumable upload supports getting the upload progress.

---

### Version 3.21.8

**Resolved Issues:**
1. Resolved the issue where some information about the directory and object names is lost when using the listing interface in Windows.

---

### Version 3.20.7

**Resolved Issues:**
1. Resolved the issue where the signature is incorrect when both the source object and destination object contain full-width characters during the copy_object operation.
2. Resolved the issue where an exception occurs when the file downloaded using download_file fails to be opened.

---

### Version 3.19.9.3

**New Features:**
1. Added the create_bucket_with_params interface to support the creation of 3AZ buckets.

---

### Version 3.19.9

**New Features:**
1. Added the code example of limiting download rates.
2. Added APIs for creating and querying POSIX buckets.
3. Optimized log printing.

**Resolved Issues:**
1. [Feature] Resolved the issue where the resumable upload fails.
2. [Feature] Resolved the issue where the upload status is not correctly returned during resumable upload.

---

### Version 3.19.7

**New Features:**
1. Added the set_object_metadata interface to provide the feature of setting object metadata.
2. Supports the setting of BBR acceleration.

**Resolved Issues:**
1. [Feature] Resolved the issue where requests fail because HTTP 2.0 is enabled on the server.
2. [Feature] Resolved the issue where the request is abnormal due to retry after an error is returned.

**Third-party Dependency Updates:**
- cURL is upgraded to 7.64.1
- libssh2 is upgraded to 1.9.0
- libxml2 is upgraded to 2.9.9
- OpenSSL is upgraded to 1.0.2r

---

## About

This is the Huawei Cloud C SDK for OBS (Object Storage Service).

For more information, please refer to the [official documentation](https://support.huaweicloud.com/intl/en-us/obs/index.html).