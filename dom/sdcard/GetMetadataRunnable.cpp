/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "GetMetadataRunnable.h"
#include "Metadata.h"
#include "Entry.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

GetMetadataRunnable::GetMetadataRunnable(MetadataCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback, Entry* aEntry) :
    CombinedRunnable(aErrorCallback, aEntry),
    mSuccessCallback(aSuccessCallback),
    mTime(0),
    mFileSize(0)
{
  SDCARD_LOG("construct GetMetadataRunnable");
  mFile = aEntry->GetFileInternal();
}

GetMetadataRunnable::~GetMetadataRunnable()
{
  SDCARD_LOG("destruct GetMetadataRunnable");
}

void
GetMetadataRunnable::WorkerThreadRun()
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
    if (NS_FAILED(rv) ) {
      SetErrorCode(rv);
      return;
    }
  }
  rv = mFile->GetLastModifiedTime(&mTime);
  if (NS_FAILED(rv) ) {
    SetErrorCode(rv);
    return;
  }
}

void
GetMetadataRunnable::OnSuccess()
{
  SDCARD_LOG("in GetMetadataRunnable.MainThreadRun()!");
  MOZ_ASSERT(mSuccessCallback, "Must pass successCallback!");

  ErrorResult rv;
  nsRefPtr<Metadata> metadata = new Metadata();
  metadata->SetSize(mFileSize);
  metadata->SetModificationTime(mTime);
  mSuccessCallback->Call(*metadata, rv);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
