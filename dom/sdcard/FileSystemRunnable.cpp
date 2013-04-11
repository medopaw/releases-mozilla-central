/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FileSystemRunnable.h"
#include "Metadata.h"

namespace mozilla {
namespace dom {
namespace sdcard {

ErrorRunnable::ErrorRunnable(ErrorCallback* aErrorCallback, const nsAString& aName) : mErrorCallback(aErrorCallback)
{
  SDCARD_LOG("init ErrorRunnable!");
  SDCARD_LOG("on main thread: %d", NS_IsMainThread());
  // MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");
  mError = DOMError::CreateWithName(aName);
}

ErrorRunnable::ErrorRunnable(ErrorCallback* aErrorCallback, const nsresult& aError) : mErrorCallback(aErrorCallback)
{
  SDCARD_LOG("init ErrorRunnable!");
  SDCARD_LOG("on main thread: %d", NS_IsMainThread());
  // MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");
  SDCARD_LOG("Error code: %d", aError);

  nsString name;
  switch (aError) {
    case NS_ERROR_FILE_INVALID_PATH:
    case NS_ERROR_FILE_UNRECOGNIZED_PATH:
      name = DOM_ERROR_ENCODING;
      break;
    case NS_ERROR_FILE_DESTINATION_NOT_DIR:
      name = DOM_ERROR_INVALID_MODIFICATION;
      break;
    case NS_ERROR_FILE_ACCESS_DENIED:
    case NS_ERROR_FILE_DIR_NOT_EMPTY:
      name = DOM_ERROR_NO_MODIFICATION_ALLOWED;
      break;
    case NS_ERROR_FILE_TARGET_DOES_NOT_EXIST:
    case NS_ERROR_NOT_AVAILABLE:
      name = DOM_ERROR_NOT_FOUND;
      break;
    case NS_ERROR_FILE_ALREADY_EXISTS:
      name = DOM_ERROR_PATH_EXISTS;
      break;
    case NS_ERROR_DOM_SECURITY_ERR:
    case NS_ERROR_OUT_OF_MEMORY:
      name = DOM_ERROR_SECURITY;
      break;
    case NS_ERROR_FILE_NOT_DIRECTORY:
      name = DOM_ERROR_TYPE_MISMATCH;
      break;
    case NS_ERROR_UNEXPECTED:
    default:
      name = DOM_ERROR_UNKNOWN;
      break;
  }
  mError = DOMError::CreateWithName(name);
}

ErrorRunnable::~ErrorRunnable()
{
}

NS_IMETHODIMP ErrorRunnable::Run()
{
  SDCARD_LOG("in ErrorRunnable.Run()");
  SDCARD_LOG("on main thread: %d", NS_IsMainThread());
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  if (mErrorCallback) {
    ErrorResult rv;
    mErrorCallback->Call(mError, rv);
  }
  return NS_OK;
}


FileSystemRunnable::FileSystemRunnable(ErrorCallback* aErrorCallback, Entry* aEntry) : mErrorCallback(aErrorCallback), mEntry(aEntry)
{
  SDCARD_LOG("init FileSystemRunnable!");
  SDCARD_LOG("on main thread: %d", NS_IsMainThread());
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
}

FileSystemRunnable::~FileSystemRunnable()
{
}


GetMetadataRunnable::GetMetadataRunnable(MetadataCallback* aSuccessCallback, ErrorCallback* aErrorCallback, Entry* aEntry) : FileSystemRunnable(aErrorCallback, aEntry), mSuccessCallback(aSuccessCallback)
{
  SDCARD_LOG("init GetMetadataRunnable");
}

GetMetadataRunnable::~GetMetadataRunnable()
{
}

NS_IMETHODIMP GetMetadataRunnable::Run()
{
  SDCARD_LOG("in GetMetadataRunnable.Run()!");
  SDCARD_LOG("on main thread: %d", NS_IsMainThread());
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  int64_t size;
  nsCOMPtr<nsIRunnable> r;
  nsresult rv = NS_OK;
  if (mEntry->IsDirectory()) {
    size = 0; // size is always 0 for directory
  } else {
    rv = mEntry->mFile->GetFileSize(&size);
    if (NS_FAILED(rv)) {
      r = new ErrorRunnable(mErrorCallback.get(), rv);
    }
  }
  if (NS_SUCCEEDED(rv)) {
    mEntry->mMetadata->mSize = uint64_t(size);
    r = new ResultRunnable<MetadataCallback, Metadata>(mSuccessCallback.get(), mEntry->mMetadata.get());
  }

  NS_DispatchToMainThread(r);

  return rv;
}


RemoveRunnable::RemoveRunnable(VoidCallback* aSuccessCallback, ErrorCallback* aErrorCallback, Entry* aEntry) : FileSystemRunnable(aErrorCallback, aEntry), mSuccessCallback(aSuccessCallback)
{
  SDCARD_LOG("init RemoveRunnable");
}

RemoveRunnable::~RemoveRunnable()
{
}

NS_IMETHODIMP RemoveRunnable::Run()
{
  SDCARD_LOG("in RemoveRunnable.Run()!");
  SDCARD_LOG("on main thread: %d", NS_IsMainThread());
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  nsCOMPtr<nsIRunnable> r;
  nsresult rv = NS_OK;
  if (mEntry->IsRoot()) {
    r = new ErrorRunnable(mErrorCallback.get(), DOM_ERROR_NO_MODIFICATION_ALLOWED);
  } else {
    rv = mEntry->mFile->Remove(false);
    if (NS_FAILED(rv)) {
      r = new ErrorRunnable(mErrorCallback.get(), rv);
    } else {
      r = new ResultRunnable<VoidCallback, void>(mSuccessCallback.get());
    }
  }

  NS_DispatchToMainThread(r);

  return rv;
}


GetFileRunnable::GetFileRunnable(const nsAString& aPath, const FileSystemFlags& aOptions, EntryCallback* aSuccessCallback, ErrorCallback* aErrorCallback, Entry* aEntry) : FileSystemRunnable(aErrorCallback, aEntry), mPath(aPath), /*mOptions(aOptions), */mSuccessCallback(aSuccessCallback)
{
  SDCARD_LOG("init GetFileRunnable");
 //  mOptions = aOptions;
}

GetFileRunnable::~GetFileRunnable()
{
}

NS_IMETHODIMP GetFileRunnable::Run()
{
  SDCARD_LOG("in RemoveRecursivelyRunnable.Run()!");
  SDCARD_LOG("on main thread: %d", NS_IsMainThread());
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");
  MOZ_ASSERT(!mEntry->mIsDirectory, "Only call on DirectoryEntry!");

  /*
  nsCOMPtr<nsIRunnable> r;
  nsresult rv = NS_OK;
  if (mEntry->IsRoot()) {
    r = new ErrorRunnable(mErrorCallback.get(), DOM_ERROR_NO_MODIFICATION_ALLOWED);
  } else {
    rv = mEntry->mFile->Remove(true);
    if (NS_FAILED(rv)) {
      r = new ErrorRunnable(mErrorCallback.get(), rv);
    } else {
      r = new ResultRunnable<VoidCallback, void>(mSuccessCallback.get());
    }
  }

  NS_DispatchToMainThread(r);

  return rv;
  */
  return NS_OK;
}


RemoveRecursivelyRunnable::RemoveRecursivelyRunnable(VoidCallback* aSuccessCallback, ErrorCallback* aErrorCallback, Entry* aEntry) : FileSystemRunnable(aErrorCallback, aEntry), mSuccessCallback(aSuccessCallback)
{
  SDCARD_LOG("init RemoveRecursivelyRunnable");
}

RemoveRecursivelyRunnable::~RemoveRecursivelyRunnable()
{
}

NS_IMETHODIMP RemoveRecursivelyRunnable::Run()
{
  SDCARD_LOG("in RemoveRecursivelyRunnable.Run()!");
  SDCARD_LOG("on main thread: %d", NS_IsMainThread());
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");
  MOZ_ASSERT(!mEntry->mIsDirectory, "Only call on DirectoryEntry!");

  nsCOMPtr<nsIRunnable> r;
  nsresult rv = NS_OK;
  if (mEntry->IsRoot()) {
    r = new ErrorRunnable(mErrorCallback.get(), DOM_ERROR_NO_MODIFICATION_ALLOWED);
  } else {
    rv = mEntry->mFile->Remove(true);
    if (NS_FAILED(rv)) {
      r = new ErrorRunnable(mErrorCallback.get(), rv);
    } else {
      r = new ResultRunnable<VoidCallback, void>(mSuccessCallback.get());
    }
  }

  NS_DispatchToMainThread(r);

  return rv;
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
