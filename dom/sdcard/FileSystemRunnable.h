/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "mozilla/dom/FileSystemBinding.h"
#include "CombinedRunnable.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class Metadata;

template <class T, class U, bool NEED_DELETION = false>
class ResultRunnable : public nsRunnable
{
  public:
    ResultRunnable(T* aSuccessCallback, U* aResult) : mSuccessCallback(aSuccessCallback), mResult(aResult)
    {
      SDCARD_LOG("init ResultRunnable!");
      SDCARD_LOG("on main thread: %d", NS_IsMainThread());
      MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");
    }

    virtual ~ResultRunnable() {}

    NS_IMETHOD Run()
    {
      SDCARD_LOG("in ResultRunnable.Run()!");
      SDCARD_LOG("on main thread: %d", NS_IsMainThread());
      MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

      if (mSuccessCallback) {
        ErrorResult rv;
        mSuccessCallback->Call(*mResult, rv);
      }
      if (NEED_DELETION && mResult) {
        delete mResult;
      }

      return NS_OK;
    }

  protected:
    nsRefPtr<T> mSuccessCallback;
    U* mResult;
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
    virtual ~ErrorRunnable();

    NS_IMETHOD Run();

  private:
    nsCOMPtr<nsIDOMDOMError> mError;
    nsRefPtr<ErrorCallback> mErrorCallback;
};

class FileSystemRunnable : public nsRunnable
{
  public:
    FileSystemRunnable(ErrorCallback* aErrorCallback, Entry* aEntry);
    virtual ~FileSystemRunnable();

  protected:
    nsRefPtr<ErrorCallback> mErrorCallback;
    nsRefPtr<Entry> mEntry;
};

class GetMetadataRunnable : public CombinedRunnable
{
public:
  GetMetadataRunnable(MetadataCallback* aSuccessCallback, ErrorCallback* aErrorCallback, Entry* aEntry);
  ~GetMetadataRunnable();

protected:
  virtual void WorkerThreadRun() MOZ_OVERRIDE;
  virtual void OnSuccess() MOZ_OVERRIDE;

private:
  nsCOMPtr<nsIFile> mFile;
  uint64_t mFileSize;
  PRTime mTime;

  // not thread safe
  nsRefPtr<MetadataCallback> mSuccessCallback;
};

class RemoveRunnable : public CombinedRunnable
{
public:
  RemoveRunnable(VoidCallback* aSuccessCallback, ErrorCallback* aErrorCallback, Entry* aEntry, bool aRecursive = false);
  ~RemoveRunnable();

protected:
  virtual void WorkerThreadRun() MOZ_OVERRIDE;
  virtual void OnSuccess() MOZ_OVERRIDE;

private:
  nsCOMPtr<nsIFile> mFile;
  bool mRecursive;

  // not thread safe
  nsRefPtr<VoidCallback> mSuccessCallback;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
