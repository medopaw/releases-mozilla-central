/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "GetEntryEvent.h"
#include "FileUtils.h"
#include "Path.h"
#include "Error.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

GetEntryEvent::GetEntryEvent(
    const nsAString& aRelpath,
    bool aCreate,
    bool aExclusive,
    bool aIsFile) :
      SDCardEvent(aRelpath),
      mCreate(aCreate),
      mExclusive(aExclusive),
      mIsFile(aIsFile)

{
  SDCARD_LOG("construct GetEntryEvent");
}

GetEntryEvent::~GetEntryEvent()
{
  SDCARD_LOG("destruct GetEntryEvent");
}

void GetEntryEvent::WorkerThreadRun()
{
  SDCARD_LOG("in GetEntryEvent.WorkerThreadRun()!");
  SDCARD_LOG("realPath=%s", NS_ConvertUTF16toUTF8(mRelpath).get());
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  bool exists;
  nsresult rv = mFile->Exists(&exists);
  if (NS_FAILED(rv)) {
    SDCARD_LOG("Error occurs when checking if mResultFile exists.");
    SetErrorCode(rv);
    return;
  }
  if (!mCreate && !exists) {
    SDCARD_LOG("If create is not true and the path doesn't exist, getFile/getDirectory must fail.");
    SetErrorName(Error::DOM_ERROR_NOT_FOUND);
    return;
  } else if (mCreate && mExclusive && exists) {
    SDCARD_LOG("If create and exclusive are both true, and the path already exists, getFile/getDirectory must fail.");
    SetErrorName(Error::DOM_ERROR_PATH_EXISTS);
    return;
  } else if (!mCreate && exists) {
      bool isFile = false;
      bool isDirectory = false;
      rv = mFile->IsFile(&isFile);
      if (NS_FAILED(rv)) {
        SDCARD_LOG("Error occurs when getting isFile.");
        SetErrorCode(rv);
        return;
      }
      rv = mFile->IsDirectory(&isDirectory);
      if (NS_FAILED(rv)) {
        SDCARD_LOG("Error occurs when getting isDirectory.");
        SetErrorCode(rv);
        return;
      }
      if (!(isFile || isDirectory)
          || (mIsFile && isDirectory)
          || (!mIsFile && isFile)) {
        SDCARD_LOG("If create is not true and the path exists, but is a directory/file, getFile/getDirectory must fail.");
        SetErrorName(Error::DOM_ERROR_TYPE_MISMATCH);
        return;
      }
  }

  if (mCreate && !exists) {
    // create
    SDCARD_LOG("Create %s", NS_ConvertUTF16toUTF8(mRelpath).get());
    // Only owner can access created item, and directory needs +x.
    uint32_t permission = mIsFile ? 0600 : 0700;
    // Note that any path segments that do not already exist will be created automatically, which I think is implied by w3c draft.
    rv = mFile->Create(FileUtils::GetType(mIsFile), permission);
    if (NS_FAILED(rv)) {
      SDCARD_LOG("Error occurs during creation.");
      SetErrorCode(rv);
      return;
    }
  }

  mFile->GetPath(mResultPath);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
