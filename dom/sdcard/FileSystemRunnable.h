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
#include "Metadata.h"

namespace mozilla {
namespace dom {
namespace sdcard {

template <class T, class U>
class ResultRunnable : public nsRunnable
{
  public:
    ResultRunnable(T& aSuccessCallback, U* aResult) : mSuccessCallback(aSuccessCallback), mResult(aResult)
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
      ErrorResult rv;
      mSuccessCallback.Call(*mResult, rv);

      return NS_OK;
    }

  private:
    T& mSuccessCallback;
    U* mResult;
};

class ErrorRunnable : public nsRunnable
{
  public:
    ErrorRunnable(ErrorCallback* aErrorCallback/*, const nsAString& name*/) : mErrorCallback(aErrorCallback)
    {
      SDCARD_LOG("init ErrorRunnable!");
      SDCARD_LOG("on main thread: %d", NS_IsMainThread());
      // MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");
    }

    NS_IMETHOD Run()
    {
      SDCARD_LOG("in ErrorRunnable.Run()");
      SDCARD_LOG("on main thread: %d", NS_IsMainThread());
      MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
      if (mErrorCallback != nullptr) {
        nsCOMPtr<nsIDOMDOMError> mError = DOMError::CreateWithName(NS_LITERAL_STRING("Example Error"));
        ErrorResult rv;
        mErrorCallback->Call(mError, rv);
      }
      return NS_OK;
    }

  private:
    nsCOMPtr<ErrorCallback> mErrorCallback;
};

class FileSystemRunnable : public nsRunnable
{
  public:
    FileSystemRunnable(ErrorCallback* aErrorCallback, Entry* aEntry) : mErrorCallback(aErrorCallback), mEntry(aEntry)
    {
      SDCARD_LOG("init FileSystemRunnable!");
      SDCARD_LOG("on main thread: %d", NS_IsMainThread());
      MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
    }

  protected:
    nsRefPtr<ErrorCallback> mErrorCallback;
    nsRefPtr<Entry> mEntry;
};

class GetMetadataRunnable : public FileSystemRunnable
{
  public:
    GetMetadataRunnable(MetadataCallback& aSuccessCallback, ErrorCallback* aErrorCallback, Entry* aEntry) : FileSystemRunnable(aErrorCallback, aEntry), mSuccessCallback(aSuccessCallback)
    {
    }

    NS_IMETHOD Run()
    {
      SDCARD_LOG("in GetMetadataRunnable.Run()!");
      SDCARD_LOG("on main thread:%d", NS_IsMainThread());
      MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");
      int64_t size;
      if (mEntry->mIsDirectory) {
        size = 0; // size is always 0 for directory
      } else {
        nsresult rv = mEntry->mFile->GetFileSize(&size);
        if (NS_FAILED(rv)) {
        // errorcallback
        }
      }
      mEntry->mMetadata->mSize = uint64_t(size);

      // successcallback
      ErrorResult rv;
//      mSuccessCallback.Call(*(mEntry->mMetadata), rv);
      nsCOMPtr<nsIRunnable> r = new ResultRunnable<MetadataCallback, Metadata>(mSuccessCallback, mEntry->mMetadata.get());
      NS_DispatchToMainThread(r);

      return NS_OK;
    }

  private:
    MetadataCallback& mSuccessCallback;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
