/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "nsThreadUtils.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "mozilla/dom/DOMError.h"
#include "Utils.h"
#include "Entry.h"

#define DOM_ERROR_ENCODING                    NS_LITERAL_STRING("EncodingError")
#define DOM_ERROR_INVALID_MODIFICATION        NS_LITERAL_STRING("InvalidModificationError")
#define DOM_ERROR_INVALID_STATE               NS_LITERAL_STRING("InvalidStateError")
#define DOM_ERROR_NOT_FOUND                   NS_LITERAL_STRING("NotFoundError")
#define DOM_ERROR_NOT_READABLE                NS_LITERAL_STRING("NotReadableError")
#define DOM_ERROR_NO_MODIFICATION_ALLOWED     NS_LITERAL_STRING("NoModificationAllowedError")
#define DOM_ERROR_PATH_EXISTS                 NS_LITERAL_STRING("PathExistsError")
#define DOM_ERROR_QUOTA_EXCEEDED              NS_LITERAL_STRING("QuotaExceededError")
#define DOM_ERROR_SECURITY                    NS_LITERAL_STRING("SecurityError")
#define DOM_ERROR_TYPE_MISMATCH               NS_LITERAL_STRING("TypeMismatchError")
#define DOM_ERROR_UNKNOWN                     NS_LITERAL_STRING("Unknown")

namespace mozilla {
namespace dom {
namespace sdcard {

template<class T, class U>
class ResultRunnable : public nsRunnable
{
  public:
    ResultRunnable(T* aSuccessCallback, U* aResult) : mSuccessCallback(aSuccessCallback), mResult(aResult)
    {
      SDCARD_LOG("init ResultRunnable!");
      SDCARD_LOG("on main thread: %d", NS_IsMainThread());
      MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");
    }

    NS_IMETHOD Run()
    {
      SDCARD_LOG("in ResultRunnable.Run()!");
      SDCARD_LOG("on main thread: %d", NS_IsMainThread());
      MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

      if (mSuccessCallback) {
        ErrorResult rv;
        mSuccessCallback->Call(*mResult, rv);
      }

      return NS_OK;
    }

  private:
    nsRefPtr<T> mSuccessCallback;
    nsRefPtr<U> mResult;
};

// for VoidCallback only
template<class T>
class ResultRunnable<T, void> : public nsRunnable
{
  public:
    ResultRunnable(T* aSuccessCallback) : mSuccessCallback(aSuccessCallback)
    {
      SDCARD_LOG("init ResultRunnable!");
      SDCARD_LOG("on main thread: %d", NS_IsMainThread());
      MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");
    }

    NS_IMETHOD Run()
    {
      SDCARD_LOG("in ResultRunnable.Run()!");
      SDCARD_LOG("on main thread: %d", NS_IsMainThread());
      MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

      if (mSuccessCallback) {
        ErrorResult rv;
        mSuccessCallback->Call(rv);
      }

      return NS_OK;
    }

  private:
    nsRefPtr<T> mSuccessCallback;
};

class ErrorRunnable : public nsRunnable
{
  public:
    ErrorRunnable(ErrorCallback* aErrorCallback, const nsAString& aName);
    ErrorRunnable(ErrorCallback* aErrorCallback, const nsresult& aError);

    NS_IMETHOD Run();

  private:
    nsCOMPtr<nsIDOMDOMError> mError;
    nsRefPtr<ErrorCallback> mErrorCallback;
};

class FileSystemRunnable : public nsRunnable
{
  public:
    FileSystemRunnable(ErrorCallback* aErrorCallback, Entry* aEntry);

  protected:
    nsRefPtr<ErrorCallback> mErrorCallback;
    nsRefPtr<Entry> mEntry;
};

class GetMetadataRunnable : public FileSystemRunnable
{
  public:
    GetMetadataRunnable(MetadataCallback* aSuccessCallback, ErrorCallback* aErrorCallback, Entry* aEntry);

    NS_IMETHOD Run();

  private:
    nsRefPtr<MetadataCallback> mSuccessCallback;
};

class RemoveRunnable : public FileSystemRunnable
{
  public:
    RemoveRunnable(VoidCallback* aSuccessCallback, ErrorCallback* aErrorCallback, Entry* aEntry);

    NS_IMETHOD Run();

  private:
    nsRefPtr<VoidCallback> mSuccessCallback;
};

class RemoveRecursivelyRunnable : public FileSystemRunnable
{
  public:
    RemoveRecursivelyRunnable(VoidCallback* aSuccessCallback, ErrorCallback* aErrorCallback, Entry* aEntry);

    NS_IMETHOD Run();

  private:
    nsRefPtr<VoidCallback> mSuccessCallback;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
