/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "GetEntryRunnable.h"
#include "DirectoryEntry.h"
#include "FileEntry.h"
#include "FileUtils.h"
#include "Path.h"
#include "Error.h"

namespace mozilla {
namespace dom {
namespace sdcard {

GetEntryRunnable::GetEntryRunnable(
    const nsAString& aPath,
    bool aCreate,
    bool aExclusive,
    EntryCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback,
    Entry* aEntry,
    bool aIsFile) :
      CombinedRunnable(aErrorCallback, aEntry),
      mPath(aPath),
      mCreate(aCreate),
      mExclusive(aExclusive),
      mSuccessCallback(aSuccessCallback),
      mIsFile(aIsFile)

{
  SDCARD_LOG("init GetEntryRunnable");
}

GetEntryRunnable::~GetEntryRunnable()
{
}

bool GetEntryRunnable::Exists(nsIFile* aFile)
{
  bool exists;
  aFile->Exists(&exists);
  return exists;
}

void GetEntryRunnable::WorkerThreadRun()
{
  SDCARD_LOG("in GetEntryRunnable.WorkerThreadRun()!");
  SDCARD_LOG("realPath=%s", NS_ConvertUTF16toUTF8(mPath).get());
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  nsresult rv = NS_NewLocalFile(mPath, false, getter_AddRefs(mResultFile));
  if (NS_FAILED(rv)) {
    SetErrorCode(rv);
    return;
  }

  unsigned long type = FileUtils::GetType(mIsFile);
  bool exists = Exists(mResultFile);
  if (!mCreate && !exists) {
    SetErrorName(Error::DOM_ERROR_NOT_FOUND);
    return;
  } else if (mCreate && mExclusive && exists) {
    SetErrorName(Error::DOM_ERROR_PATH_EXISTS);
    return;
  } else if (!mCreate && exists) {
      bool isFile = false;
      bool isDirectory = false;
      rv = mResultFile->IsFile(&isFile);
      if (NS_FAILED(rv)) {
        SetErrorCode(rv);
        return;
      }
      rv = mResultFile->IsDirectory(&isDirectory);
      if (NS_FAILED(rv)) {
        SetErrorCode(rv);
        return;
      }
      if (!(isFile || isDirectory)
          || (type == nsIFile::NORMAL_FILE_TYPE && isDirectory)
          || (type == nsIFile::DIRECTORY_TYPE && isFile)) {
        SetErrorName(Error::DOM_ERROR_TYPE_MISMATCH);
        return;
      }
  }

  if (mCreate && !exists) {
    // create
    SDCARD_LOG("Create %s", NS_ConvertUTF16toUTF8(mPath).get());
    rv = mResultFile->Create(type, 0600);
    if (NS_FAILED(rv)) {
      SetErrorCode(rv);
      return;
    }
  }
}

void GetEntryRunnable::OnSuccess()
{
  SDCARD_LOG("in GetEntryRunnable.OnSuccess()!");
  MOZ_ASSERT(mSuccessCallback, "Must pass successCallback!");

  ErrorResult rv;
  nsRefPtr<Entry> resultEntry = Entry::CreateFromFile(GetEntry()->GetFilesystem(), mResultFile.get());
  mSuccessCallback->Call(*resultEntry, rv);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
