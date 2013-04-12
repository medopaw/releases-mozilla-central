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
    CombinedRunnable(aEntry),
    mFileSize(0),
    mTime(0),
    mSuccessCallback(aSuccessCallback),
    mErrorCallback(aErrorCallback)
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

void GetMetadataRunnable::MainThreadRun()
{
  SDCARD_LOG("in GetMetadataRunnable.MainThreadRun()!");
  nsRefPtr<nsIDOMDOMError> error = GetDOMError();
  if (error) {
    // error callback
    if (mErrorCallback) {
      ErrorResult rv;
      mErrorCallback->Call(error, rv);
    }
  } else {
    // success callback
    ErrorResult rv;
    nsRefPtr<Metadata> metadata = new Metadata();
    metadata->SetSize(mFileSize);
    metadata->SetModificationTime(mTime);
    mSuccessCallback->Call(*metadata, rv);
  }
}

RemoveRunnable::RemoveRunnable(VoidCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback, Entry* aEntry) :
    CombinedRunnable(aEntry),
    mSuccessCallback(aSuccessCallback),
    mErrorCallback(aErrorCallback)
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
  nsresult rv = NS_OK;
  nsString path;
  mFile->GetPath(path);
  if (Path::IsBase(path)) {
    // Cannot remove root directory
    SetErrorName(DOM_ERROR_NO_MODIFICATION_ALLOWED);
    return;
  } else {
    rv = mFile->Remove(false);
    if (NS_FAILED(rv)) {
      SetErrorCode(rv);
    }
  }
}

void RemoveRunnable::MainThreadRun()
{
  SDCARD_LOG("in RemoveRunnable.MainThreadRun()!");
  nsRefPtr<nsIDOMDOMError> error = GetDOMError();
  if (error) {
    // error callback
    if (mErrorCallback) {
      ErrorResult rv;
      mErrorCallback->Call(error, rv);
    }
  } else {
    // success callback
    ErrorResult rv;
    mSuccessCallback->Call(rv);
  }
}

GetEntryRunnable::GetEntryRunnable(const nsAString& aPath, bool aCreate, bool aExclusive, const unsigned long aType, EntryCallback* aSuccessCallback, ErrorCallback* aErrorCallback) : FileSystemRunnable(aErrorCallback, nullptr), mPath(aPath), mCreate(aCreate), mExclusive(aExclusive), mType(aType), mSuccessCallback(aSuccessCallback)
{
  SDCARD_LOG("init GetEntryRunnable");
}

GetEntryRunnable::~GetEntryRunnable()
{
}

NS_IMETHODIMP GetEntryRunnable::Run()
{
  SDCARD_LOG("in GetEntryRunnable.Run()!");
  SDCARD_LOG("on main thread: %d", NS_IsMainThread());
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");
  MOZ_ASSERT(mEntry->IsDirectory() || mEntry->IsFile(), "Must be either directory or file!");
  SDCARD_LOG("realPath=%s", NS_ConvertUTF16toUTF8(mPath).get());

  nsCOMPtr<nsIRunnable> r;
  nsCOMPtr<nsIFile> file;
  nsresult rv = NS_NewLocalFile(mPath, false, getter_AddRefs(file));
  if (NS_SUCCEEDED(rv)) {
    bool exists;
    file->Exists(&exists);
    // deal with fails
    nsString errorName;
    if (!mCreate && !exists) {
      errorName = DOM_ERROR_NOT_FOUND;
    } else if (mCreate && mExclusive && exists) {
      errorName = DOM_ERROR_PATH_EXISTS;
    } else if (!mCreate && exists) {
      bool isDirectory, isFile;
      file->IsDirectory(&isDirectory);
      file->IsFile(&isFile);
      if ((mType == nsIFile::NORMAL_FILE_TYPE && isDirectory) || (mType == nsIFile::DIRECTORY_TYPE && isFile)) {
        errorName = DOM_ERROR_TYPE_MISMATCH;
      }
    }
    if (!errorName.IsEmpty()) {
      r = new ErrorRunnable(mErrorCallback, errorName);
    } else {
      if (mCreate && !exists) {
        // create
        rv = file->Create(mType, 0600);
      } 
      // retrieve
      if (NS_SUCCEEDED(rv)) {
        // create correspondng Entry
        nsRefPtr<Entry> pEntry;
        switch (mType) {
          case nsIFile::NORMAL_FILE_TYPE:
            pEntry = new FileEntry(mEntry->Filesystem(), file);
            break;
          case nsIFile::DIRECTORY_TYPE:
            pEntry = new DirectoryEntry(mEntry->Filesystem(), file);
            break;
          default:
            pEntry = nullptr;
            break;
        }
        // create EntryCallback
        if (pEntry) {
          r = new ResultRunnable<EntryCallback, Entry>(mSuccessCallback.get(), pEntry.get());
        } else {
          r = new ErrorRunnable(mErrorCallback, DOM_ERROR_TYPE_MISMATCH);
        }
      }
    }
  }
  if (NS_FAILED(rv)) {
    r = new ErrorRunnable(mErrorCallback.get(), rv);
  }
  NS_DispatchToMainThread(r);

  return rv;
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
  MOZ_ASSERT(!mEntry->IsDirectory(), "Only call on DirectoryEntry!");

  nsCOMPtr<nsIRunnable> r;
  nsresult rv = NS_OK;
  if (mEntry->IsRoot()) {
    r = new ErrorRunnable(mErrorCallback.get(), DOM_ERROR_NO_MODIFICATION_ALLOWED);
  } else {
    rv = mEntry->GetFileInternal()->Remove(true);
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
