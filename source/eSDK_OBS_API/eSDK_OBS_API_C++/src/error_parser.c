/*********************************************************************************
* Copyright 2019 Huawei Technologies Co.,Ltd.
* Licensed under the Apache License, Version 2.0 (the "License"); you may not use
* this file except in compliance with the License.  You may obtain a copy of the
* License at
* 
* http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable law or agreed to in writing, software distributed
* under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
* CONDITIONS OF ANY KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations under the License.
**********************************************************************************
*/
#include <string.h>
#include "error_parser.h"

#ifdef WIN32
# pragma warning (disable:4127)
#endif
static obs_status errorXmlCallback(const char *elementPath, const char *data,
                                 int dataLen, void *callback_data)
{
    if (!data) {
        return OBS_STATUS_OK;
    }

    error_parser *errorParser = (error_parser *) callback_data;

    int fit;

    COMMLOG(OBS_LOGWARN, "%s errorXml : %s : %.*s", __FUNCTION__,elementPath, dataLen, data);

    if (!strcmp(elementPath, "Error")) {
    }
    else if (!strcmp(elementPath, "Error/Code")) {
        string_buffer_append(errorParser->code, data, dataLen, fit);
    }
    else if (!strcmp(elementPath, "Error/Message")) {
        string_buffer_append(errorParser->message, data, dataLen, fit);
        errorParser->obsErrorDetails.message = errorParser->message;
    }
    else if (!strcmp(elementPath, "Error/Resource")) {
        string_buffer_append(errorParser->resource, data, dataLen, fit);
        errorParser->obsErrorDetails.resource = errorParser->resource;
    }
    else if (!strcmp(elementPath, "Error/FurtherDetails")) {
        string_buffer_append(errorParser->further_details, data, dataLen, fit);
        errorParser->obsErrorDetails.further_details = 
            errorParser->further_details;
    }
    else {
        if (strncmp(elementPath, "Error/", sizeof("Error/") - 1)) {
            return OBS_STATUS_OK;
        }
        const char *elementName = &(elementPath[sizeof("Error/") - 1]);
        if (errorParser->obsErrorDetails.extra_details_count && 
            !strcmp(elementName, 
            errorParser->obsErrorDetails.extra_details[errorParser->obsErrorDetails.extra_details_count - 1].name))
        {
            string_multibuffer_append(errorParser->extraDetailsNamesValues,
                                      data, dataLen, fit);
            if (!fit) {
                errorParser->obsErrorDetails.extra_details_count--;
            }
            return OBS_STATUS_OK;
        }
        if (errorParser->obsErrorDetails.extra_details_count ==
            sizeof(errorParser->extra_details)) {
            return OBS_STATUS_OK;
        }
        char *name = string_multibuffer_current
            (errorParser->extraDetailsNamesValues);
        int nameLen = strlen(elementName);
        string_multibuffer_add(errorParser->extraDetailsNamesValues,
                               elementName, nameLen, fit);
        if (!fit) {
            return OBS_STATUS_OK;
        }
        char *value = string_multibuffer_current
            (errorParser->extraDetailsNamesValues);
        string_multibuffer_add(errorParser->extraDetailsNamesValues,
                               data, dataLen, fit);
        if (!fit) {
            return OBS_STATUS_OK;
        }
        obs_name_value *nv = 
            &(errorParser->extra_details
              [errorParser->obsErrorDetails.extra_details_count++]);
        nv->name = name;
        nv->value = value;
    }

    return OBS_STATUS_OK;
}


void error_parser_initialize(error_parser *errorParser)
{
    errorParser->obsErrorDetails.message = 0;
    errorParser->obsErrorDetails.resource = 0;
    errorParser->obsErrorDetails.further_details = 0;
    errorParser->obsErrorDetails.extra_details_count = 0;
    errorParser->obsErrorDetails.extra_details = errorParser->extra_details;
    errorParser->errorXmlParserInitialized = 0;
    string_buffer_initialize(errorParser->code);
    string_buffer_initialize(errorParser->message);
    string_buffer_initialize(errorParser->resource);
    string_buffer_initialize(errorParser->further_details);
    string_multibuffer_initialize(errorParser->extraDetailsNamesValues);
}


obs_status error_parser_add(error_parser *errorParser, const char *buffer,
                          int buffer_size)
{/*lint !e101 */
    if (!errorParser->errorXmlParserInitialized) {
        simplexml_initialize(&(errorParser->errorXmlParser), &errorXmlCallback,
                             errorParser);/*lint !e119 */
        errorParser->errorXmlParserInitialized = 1;
    }

    return simplexml_add(&(errorParser->errorXmlParser), buffer, buffer_size);
}


void error_parser_convert_status(error_parser *errorParser, obs_status *status)
{
    if (!errorParser->codeLen) {
        return;
    }

#define HANDLE_CODE(name)                                       \
    do {                                                        \
        if (!strcmp(errorParser->code, #name)) {                \
            *status = OBS_STATUS_##name;                      \
            goto code_set;                                      \
        }                                                       \
    } while (0)
    
    HANDLE_CODE(AccessDenied);
    HANDLE_CODE(AccountProblem);
    HANDLE_CODE(AmbiguousGrantByEmailAddress);
    HANDLE_CODE(BadDigest);
    HANDLE_CODE(BucketAlreadyExists);
    HANDLE_CODE(BucketAlreadyOwnedByYou);
    HANDLE_CODE(BucketNotEmpty);
    HANDLE_CODE(CredentialsNotSupported);
    HANDLE_CODE(CrossLocationLoggingProhibited);
    HANDLE_CODE(EntityTooSmall);
    HANDLE_CODE(EntityTooLarge);
    HANDLE_CODE(ExpiredToken);
    HANDLE_CODE(IllegalVersioningConfigurationException); 
    HANDLE_CODE(IncompleteBody);
    HANDLE_CODE(IncorrectNumberOfFilesInPostRequest);
    HANDLE_CODE(InlineDataTooLarge);
    HANDLE_CODE(InternalError);
    HANDLE_CODE(InvalidAccessKeyId);
    HANDLE_CODE(InvalidAddressingHeader);
    HANDLE_CODE(InvalidArgument);
    HANDLE_CODE(InvalidBucketName);
    HANDLE_CODE(InvalidBucketState); 
    HANDLE_CODE(InvalidDigest);
    HANDLE_CODE(InvalidLocationConstraint);
    HANDLE_CODE(InvalidObjectState); 
    HANDLE_CODE(InvalidPart); 
    HANDLE_CODE(InvalidPartOrder);
    HANDLE_CODE(InvalidPayer);
    HANDLE_CODE(InvalidPolicyDocument);
    HANDLE_CODE(InvalidRange);
    HANDLE_CODE(InvalidRedirectLocation);
    HANDLE_CODE(InvalidRequest);
    HANDLE_CODE(InvalidSecurity);
    HANDLE_CODE(InvalidSOAPRequest);
    HANDLE_CODE(InvalidStorageClass);
    HANDLE_CODE(InvalidTargetBucketForLogging);
    HANDLE_CODE(InvalidToken);
    HANDLE_CODE(InvalidURI);
    HANDLE_CODE(MalformedACLError);
    HANDLE_CODE(MalformedPolicy);
    HANDLE_CODE(MalformedPOSTRequest);
    HANDLE_CODE(MalformedXML);
    HANDLE_CODE(MaxMessageLengthExceeded);
    HANDLE_CODE(MaxPostPreDataLengthExceededError);
    HANDLE_CODE(MetadataTooLarge);
    HANDLE_CODE(MethodNotAllowed);
    HANDLE_CODE(MissingAttachment);
    HANDLE_CODE(MissingContentLength);
    HANDLE_CODE(MissingRequestBodyError);
    HANDLE_CODE(MissingSecurityElement);
    HANDLE_CODE(MissingSecurityHeader);
    HANDLE_CODE(NoLoggingStatusForKey);
    HANDLE_CODE(NoSuchBucket);
    HANDLE_CODE(NoSuchKey);
    HANDLE_CODE(NoSuchLifecycleConfiguration);
    HANDLE_CODE(NoSuchUpload);
    HANDLE_CODE(NoSuchVersion);
    HANDLE_CODE(NotImplemented);
    HANDLE_CODE(NotSignedUp);
    HANDLE_CODE(NotSuchBucketPolicy);
    HANDLE_CODE(OperationAborted);
    HANDLE_CODE(PermanentRedirect);
    HANDLE_CODE(PreconditionFailed);
    HANDLE_CODE(Redirect);
    HANDLE_CODE(RestoreAlreadyInProgress);
    HANDLE_CODE(RequestIsNotMultiPartContent);
    HANDLE_CODE(RequestTimeout);
    HANDLE_CODE(RequestTimeTooSkewed);
    HANDLE_CODE(RequestTorrentOfBucketError);
    HANDLE_CODE(SignatureDoesNotMatch);
    HANDLE_CODE(ServiceUnavailable);
    HANDLE_CODE(SlowDown);
    HANDLE_CODE(TemporaryRedirect);
    HANDLE_CODE(TokenRefreshRequired);
    HANDLE_CODE(TooManyBuckets);
    HANDLE_CODE(UnexpectedContent);
    HANDLE_CODE(UnresolvableGrantByEmailAddress);
    HANDLE_CODE(UserKeyMustBeSpecified);
    HANDLE_CODE(InsufficientStorageSpace);
    HANDLE_CODE(NoSuchWebsiteConfiguration);
    HANDLE_CODE(NoSuchBucketPolicy);
    HANDLE_CODE(NoSuchCORSConfiguration);
    HANDLE_CODE(InArrearOrInsufficientBalance);
    HANDLE_CODE(NoSuchTagSet);
    *status = OBS_STATUS_ErrorUnknown;

 code_set:

    return;
}

void error_parser_deinitialize(error_parser *errorParser)
{
    if (errorParser->errorXmlParserInitialized) {
        simplexml_deinitialize(&(errorParser->errorXmlParser));
    }
}