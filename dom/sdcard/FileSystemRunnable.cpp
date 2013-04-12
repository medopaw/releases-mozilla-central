/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FileSystemRunnable.h"
#include "DirectoryEntry.h"
#include "FileEntry.h"
#include "Metadata.h"
#include "Path.h"

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
  nsString errorCode;
  switch (aError) {
    case NS_ERROR_FILE_INVALID_PATH:
      errorCode.AssignLiteral("NS_ERROR_FILE_INVALID_PATH");
      break;
    case NS_ERROR_FILE_UNRECOGNIZED_PATH:
      errorCode.AssignLiteral("NS_ERROR_FILE_UNRECOGNIZED_PATH");
      break;
    case NS_ERROR_FILE_DESTINATION_NOT_DIR:
      errorCode.AssignLiteral("NS_ERROR_FILE_DESTINATION_NOT_DIR");
      break;
    case NS_ERROR_FILE_ACCESS_DENIED:
      errorCode.AssignLiteral("NS_ERROR_FILE_ACCESS_DENIED");
      break;
    case NS_ERROR_FILE_DIR_NOT_EMPTY:
      errorCode.AssignLiteral("NS_ERROR_FILE_DIR_NOT_EMPTY");
      break;
    case NS_ERROR_FILE_TARGET_DOES_NOT_EXIST:
      errorCode.AssignLiteral("NS_ERROR_FILE_TARGET_DOES_NOT_EXIST");
      break;
    case NS_ERROR_NOT_AVAILABLE:
      errorCode.AssignLiteral("NS_ERROR_NOT_AVAILABLE");
      break;
    case NS_ERROR_FILE_ALREADY_EXISTS:
      errorCode.AssignLiteral("NS_ERROR_FILE_ALREADY_EXISTS");
      break;
    case NS_ERROR_DOM_SECURITY_ERR:
      errorCode.AssignLiteral("NS_ERROR_DOM_SECURITY_ERR");
    case NS_ERROR_OUT_OF_MEMORY:
      errorCode.AssignLiteral("NS_ERROR_OUT_OF_MEMORY");
      break;
    case NS_ERROR_FILE_NOT_DIRECTORY:
      errorCode.AssignLiteral("NS_ERROR_FILE_NOT_DIRECTORY");
      break;
    case NS_ERROR_UNEXPECTED:
      errorCode.AssignLiteral("NS_ERROR_UNEXPECTED");
    default:
      errorCode.AssignLiteral("Unknow Error Code");
      SDCARD_LOG("Error Code: %u", aError);
      break;
  }
  SDCARD_LOG("Error code: %s", NS_ConvertUTF16toUTF8(errorCode).get());

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


GetMetadataRunnable::GetMetadataRunnable(MetadataCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback, Entry* aEntry) :
    CombinedRunnable(aErrorCallback, aEntry),
    mSuccessCallback(aSuccessCallback),
    mTime(0),
    mFileSize(0)
{
  SDCARD_LOG("init GetMetadataRunnable");
  mFile = aEntry->GetFileInternal();
}

GetMetadataRunnable::~GetMetadataRunnable()
{
}

void GetMetadataRunnable::WorkerThreadRun()
{
  SDCARD_LOG("in GetMetadataRunnable.WorkerThreadRun()!");
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  nsresult rv = NS_OK;
  bool isDirectory = false;
  mFile->IsDirectory(&isDirectory);
  if (isDirectory) {
    mFileSize = 0; // size is always 0 for directory
  } else {
    int64_t size = 0;
    rv = mFile->GetFileSize(&size);
    mFileSize = static_cast<uint64_t>(size);
    if (NS_FAILED(rv)) {
      SetErrorCode(rv);
      return;
    }
  }
  rv = mFile->GetLastModifiedTime(&mTime);
  if (NS_FAILED(rv)) {
    SetErrorCode(rv);
    return;
  }
}

void GetMetadataRunnable::OnSuccess()
{
  SDCARD_LOG("in GetMetadataRunnable.MainThreadRun()!");
  MOZ_ASSERT(mSuccessCallback, "Must pass successCallback!");

  ErrorResult rv;
  nsRefPtr<Metadata> metadata = new Metadata();
  metadata->SetSize(mFileSize);
  metadata->SetModificationTime(mTime);
  mSuccessCallback->Call(*metadata, rv);
}

RemoveRunnable::RemoveRunnable(
    VoidCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback,
    Entry* aEntry,
    bool aRecursive) :
    CombinedRunnable(aErrorCallback, aEntry),
    mSuccessCallback(aSuccessCallback),
    mRecursive(aRecursive)
{
  SDCARD_LOG("init RemoveRunnable");
  mFile = aEntry->GetFileInternal();
}

RemoveRunnable::~RemoveRunnable()
{
}

void RemoveRunnable::WorkerThreadRun()
{
  SDCARD_LOG("in RemoveRunnable.WorkerThreadRun()!");
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  nsresult rv = NS_OK;
  nsString path;
  mFile->GetPath(path);
  if (Path::IsBase(path)) {
    // cannot remove root directory
    SetErrorName(DOM_ERROR_NO_MODIFICATION_ALLOWED);
    return;
  } else {
    rv = mFile->Remove(mRecursive);
    if (NS_FAILED(rv)) {
      SetErrorCode(rv);
    }
  }
}

void RemoveRunnable::OnSuccess()
{
  SDCARD_LOG("in RemoveRunnable.MainThreadRun()!");
  MOZ_ASSERT(mSuccessCallback, "Must pass successCallback!");

  ErrorResult rv;
  mSuccessCallback->Call(rv);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
