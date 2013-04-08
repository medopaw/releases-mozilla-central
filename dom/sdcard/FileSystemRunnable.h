/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "nsThreadUtils.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

template <class S, class T>
class ResultRunnable : public nsRunnable
{
  public:
    ResultRunnable(S& aSuccessCallback, T* aResult) : mSuccessCallback(aSuccessCallback), mResult(aResult)
    {
      SDCARD_LOG("init ResultRunnable!");
      SDCARD_LOG("on main thread: %d", NS_IsMainThread());
      MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!"); // This should be running on the worker thread
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
    S mSuccessCallback;
    T* mResult;
};

class FileSystemRunnable : public nsRunnable
{
  public:
    FileSystemRunnable(const Optional< OwningNonNull<ErrorCallback> >& aErrorCallback, Entry* aEntry) : mErrorCallback(aErrorCallback), mEntry(aEntry)
    {
      SDCARD_LOG("init FileSystemRunnable!");
      SDCARD_LOG("on main thread: %d", NS_IsMainThread());
      MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
    }

  protected:
    const Optional< OwningNonNull<ErrorCallback> >& mErrorCallback;
    Entry* mEntry;
};

class GetMetadataRunnable : public FileSystemRunnable
{
  public:
    GetMetadataRunnable(MetadataCallback& aSuccessCallback, const Optional< OwningNonNull<ErrorCallback> >& aErrorCallback, Entry* aEntry) : FileSystemRunnable(aErrorCallback, aEntry), mSuccessCallback(aSuccessCallback)
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
      nsCOMPtr<nsIRunnable> r = new ResultRunnable<MetadataCallback, Metadata>(mSuccessCallback, mEntry->mMetadata);
      NS_DispatchToMainThread(r);

      return NS_OK;
    }

  private:
    MetadataCallback mSuccessCallback;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
