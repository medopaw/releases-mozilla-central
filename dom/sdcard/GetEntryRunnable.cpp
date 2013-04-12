/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "GetEntryRunnable.h"
#include "DirectoryEntry.h"
#include "FileEntry.h"
#include "Path.h"

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
    const unsigned long aType) :
      CombinedRunnable(aErrorCallback, aEntry),
      mPath(aPath),
      mCreate(aCreate),
      mExclusive(aExclusive),
      mSuccessCallback(aSuccessCallback),
      mType(aType)

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

  bool exists = Exists(mResultFile);
  if (!mCreate && !exists) {
    SetErrorName(DOM_ERROR_NOT_FOUND);
    return;
  } else if (mCreate && mExclusive && exists) {
    SetErrorName(DOM_ERROR_PATH_EXISTS);
    return;
  } else if (!mCreate && exists) {
      bool isDirectory, isFile;
      mResultFile->IsDirectory(&isDirectory);
      mResultFile->IsFile(&isFile);
      if (!(mType == nsIFile::NORMAL_FILE_TYPE || mType == nsIFile::DIRECTORY_TYPE)
          || (mType == nsIFile::NORMAL_FILE_TYPE && isDirectory)
          || (mType == nsIFile::DIRECTORY_TYPE && isFile)) {
        SetErrorName(DOM_ERROR_TYPE_MISMATCH);
        return;
      }
  }

  if (mCreate && !exists) {
    // create
    SDCARD_LOG("Create %s", NS_ConvertUTF16toUTF8(mPath).get());
    rv = mResultFile->Create(mType, 0600);
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
  nsRefPtr<Entry> resultEntry = Entry::FromFile(GetEntry()->GetFilesystem(), mResultFile.get());
  mSuccessCallback->Call(*resultEntry, rv);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
