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
      name = DOM_ERROR_NO_MODIFICATION_ALLOWED;
      break;
    case NS_ERROR_FILE_TARGET_DOES_NOT_EXIST:
    case NS_ERROR_NOT_AVAILABLE:
      name = DOM_ERROR_NOT_FOUND;
      break;
    case NS_ERROR_FILE_ALREADY_EXISTS:
    case NS_ERROR_FILE_DIR_NOT_EMPTY:
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


GetMetadataRunnable::GetMetadataRunnable(MetadataCallback* aSuccessCallback, ErrorCallback* aErrorCallback, Entry* aEntry) : FileSystemRunnable(aErrorCallback, aEntry), mSuccessCallback(aSuccessCallback)
{
  SDCARD_LOG("init GetMetadataRunnable");
}

NS_IMETHODIMP GetMetadataRunnable::Run()
{
  SDCARD_LOG("in GetMetadataRunnable.Run()!");
  SDCARD_LOG("on main thread: %d", NS_IsMainThread());
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  int64_t size;
  nsCOMPtr<nsIRunnable> r;
  if (mEntry->mIsDirectory) {
    size = 0; // size is always 0 for directory
  } else {
    nsresult rv = mEntry->mFile->GetFileSize(&size);
    if (NS_FAILED(rv)) {
      r = new ErrorRunnable(mErrorCallback.get(), rv);
    }
  }
  mEntry->mMetadata->mSize = uint64_t(size);

  ErrorResult rv;
  r = new ResultRunnable<MetadataCallback, Metadata>(mSuccessCallback.get(), mEntry->mMetadata.get());
  NS_DispatchToMainThread(r);

  return NS_OK;
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
