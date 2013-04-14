/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Error.h"
#include "mozilla/dom/DOMError.h"
// #include "mozilla/dom/FileSystemBinding.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

const nsString Error::DOM_ERROR_ENCODING                =   NS_LITERAL_STRING("EncodingError");
const nsString Error::DOM_ERROR_INVALID_MODIFICATION    =   NS_LITERAL_STRING("InvalidModificationError");
const nsString Error::DOM_ERROR_INVALID_STATE           =   NS_LITERAL_STRING("InvalidStateError");
const nsString Error::DOM_ERROR_NOT_FOUND               =   NS_LITERAL_STRING("NotFoundError");
const nsString Error::DOM_ERROR_NOT_READABLE            =   NS_LITERAL_STRING("NotReadableError");
const nsString Error::DOM_ERROR_NO_MODIFICATION_ALLOWED =   NS_LITERAL_STRING("NoModificationAllowedError");
const nsString Error::DOM_ERROR_PATH_EXISTS             =   NS_LITERAL_STRING("PathExistsError");
const nsString Error::DOM_ERROR_QUOTA_EXCEEDED          =   NS_LITERAL_STRING("QuotaExceededError");
const nsString Error::DOM_ERROR_SECURITY                =   NS_LITERAL_STRING("SecurityError");
const nsString Error::DOM_ERROR_TYPE_MISMATCH           =   NS_LITERAL_STRING("TypeMismatchError");
const nsString Error::DOM_ERROR_UNKNOWN                 =   NS_LITERAL_STRING("Unknown");

void Error::HandleError(ErrorCallback* aErrorCallback, const nsAString& aErrorName)
{
  SDCARD_LOG("in ErrorHandler::HandleError()");
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

  SDCARD_LOG("Create DOMError with %s", NS_ConvertUTF16toUTF8(aErrorName).get());
  if (aErrorCallback) { // errorCallback is always optional
    ErrorResult rv;
    aErrorCallback->Call(DOMError::CreateWithName(aErrorName).get(), rv);
  }
}

void Error::HandleError(ErrorCallback* aErrorCallback, const nsresult& aErrorCode)
{
  SDCARD_LOG("in ErrorHandler::HandleError()");
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

  if (aErrorCallback) { // errorCallback is always optional
    nsString errorString;
    switch (aErrorCode) {
      case NS_ERROR_FILE_INVALID_PATH:
        errorString.AssignLiteral("NS_ERROR_FILE_INVALID_PATH");
        break;
      case NS_ERROR_FILE_UNRECOGNIZED_PATH:
        errorString.AssignLiteral("NS_ERROR_FILE_UNRECOGNIZED_PATH");
        break;
      case NS_ERROR_FILE_DESTINATION_NOT_DIR:
        errorString.AssignLiteral("NS_ERROR_FILE_DESTINATION_NOT_DIR");
        break;
      case NS_ERROR_FILE_ACCESS_DENIED:
        errorString.AssignLiteral("NS_ERROR_FILE_ACCESS_DENIED");
        break;
      case NS_ERROR_FILE_DIR_NOT_EMPTY:
        errorString.AssignLiteral("NS_ERROR_FILE_DIR_NOT_EMPTY");
        break;
      case NS_ERROR_FILE_TARGET_DOES_NOT_EXIST:
        errorString.AssignLiteral("NS_ERROR_FILE_TARGET_DOES_NOT_EXIST");
        break;
      case NS_ERROR_NOT_AVAILABLE:
        errorString.AssignLiteral("NS_ERROR_NOT_AVAILABLE");
        break;
      case NS_ERROR_FILE_ALREADY_EXISTS:
        errorString.AssignLiteral("NS_ERROR_FILE_ALREADY_EXISTS");
        break;
      case NS_ERROR_DOM_SECURITY_ERR:
        errorString.AssignLiteral("NS_ERROR_DOM_SECURITY_ERR");
      case NS_ERROR_OUT_OF_MEMORY:
        errorString.AssignLiteral("NS_ERROR_OUT_OF_MEMORY");
        break;
      case NS_ERROR_FILE_NOT_DIRECTORY:
        errorString.AssignLiteral("NS_ERROR_FILE_NOT_DIRECTORY");
        break;
      case NS_ERROR_UNEXPECTED:
        errorString.AssignLiteral("NS_ERROR_UNEXPECTED");
      default:
        break;
    }
    if (errorString.IsEmpty()) {
      SDCARD_LOG("Create DOMError from nsresult %ul", aErrorCode);
    } else {
      SDCARD_LOG("Create DOMError from nsresult %s", NS_ConvertUTF16toUTF8(errorString).get());
    }

    ErrorResult rv;
    aErrorCallback->Call(DOMError::CreateForNSResult(aErrorCode).get(), rv);
  }
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
