/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CopyAndMoveToRunnable.h"
#include "DirectoryEntry.h"
#include "FileEntry.h"
#include "FileUtils.h"
#include "Path.h"
#include "Error.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

CopyAndMoveToRunnable::CopyAndMoveToRunnable(DirectoryEntry* aParent,
    const nsAString* aNewName,
    EntryCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback,
    Entry* aEntry,
    bool aIsCopy) :
    CombinedRunnable(aErrorCallback, aEntry),
    mSuccessCallback(aSuccessCallback),
    mIsCopy(aIsCopy)
{
  SDCARD_LOG("construct CopyAndMoveToRunnable");
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

  if (aNewName && !aNewName->IsEmpty()) {
    mNewName = *aNewName;
  } else {
    // If new name not specified, set it to the entry's current name.
    aEntry->GetName(mNewName);
  }
  mFile = aEntry->GetFileInternal();
  mNewParentDir = aParent->GetFileInternal();
}

CopyAndMoveToRunnable::~CopyAndMoveToRunnable()
{
  SDCARD_LOG("destruct CopyAndMoveToRunnable");
}

void
CopyAndMoveToRunnable::WorkerThreadRun()
{
  SDCARD_LOG("in CopyAndMoveToRunnable.WorkerThreadRun()!");
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  nsresult rv = NS_OK;
  nsString path;
  rv = mFile->GetPath(path);
  if (Path::IsBase(path)) {
    // Cannot copy/move the root directory
    SDCARD_LOG("Can't copy/move the root directory!");
    SetErrorName(Error::DOM_ERROR_INVALID_MODIFICATION);
    return;
  }

  bool isFile = false;
  bool isDirectory = false;
  rv = mFile->IsFile(&isFile);
  if (NS_FAILED(rv) ) {
    SDCARD_LOG("Error occurs when getting isFile.");
    SetErrorCode(rv);
    return;
  }
  rv = mFile->IsDirectory(&isDirectory);
  if (NS_FAILED(rv) ) {
    SDCARD_LOG("Error occurs when getting isDirectory.");
    SetErrorCode(rv);
    return;
  }

  if (!(isFile || isDirectory)) {
    // Cannot copy/move a special file
    SDCARD_LOG("mFile is neither a file nor directory.");
    SetErrorName(Error::DOM_ERROR_INVALID_MODIFICATION);
    return;
  }

  // assign mResultFile to target file first
  mNewParentDir->Clone(getter_AddRefs(mResultFile));
  rv = mResultFile->Append(mNewName);
  if (NS_FAILED(rv) ) {
    SDCARD_LOG("Error occurs when append new name to mResultFile.");
    SetErrorCode(rv);
    return;
  }

  // Check if destination exists
  bool newFileExits = false;
  rv = mResultFile->Exists(&newFileExits);
  if (NS_FAILED(rv) ) {
    SDCARD_LOG("Error occurs when checking if mResultFile exists.");
    SetErrorCode(rv);
    return;
  }

  // Whether the destination is a file
  bool isNewFile = false;
  // Whether the destination is a directory
  bool isNewDirectory = false;
  if (newFileExits) {
    mResultFile->IsFile(&isNewFile);
    mResultFile->IsDirectory(&isNewDirectory);
    if (!(isNewFile || isNewDirectory)) {
      // Cannot overwrite a special file.
      SDCARD_LOG("mResultFile is neither a file nor directory.");
      SetErrorName(Error::DOM_ERROR_INVALID_MODIFICATION);
      return;
    }
  }

  nsString newPath;
  mResultFile->GetPath(newPath);

  // The destination is the same with the source
  if (path == newPath) {
    // Cannot copy/move an entry into its parent if a name different from its
    // current one isn't provided
    SDCARD_LOG(
        "Cannot copy/move an entry into its parent if a name different from its current one isn't provided.");
    SetErrorName(Error::DOM_ERROR_INVALID_MODIFICATION);
    return;
  }

  if (isNewDirectory) {
    bool dirEmpty;
    rv = FileUtils::IsDirectoryEmpty(mResultFile, &dirEmpty);
    if (NS_FAILED(rv) ) {
      SDCARD_LOG("Error occurs when checking if directory is empty.");
      SetErrorCode(rv);
      return;
    }
    if (!dirEmpty) {
      // Cannot copy/move to a path occupied by a directory which is not empty.
      SDCARD_LOG(
          "Cannot copy/move to a path occupied by a directory which is not empty.");
      SetErrorName(Error::DOM_ERROR_INVALID_MODIFICATION);
      return;
    }
  }

  if ((isFile && isNewDirectory) || (isDirectory && isNewFile)) {
    // Cannot copy/move a file to a path occupied by a directory, or
    // copy/move a directory to a path occupied by a file
    SDCARD_LOG(
        "Cannot copy/move a file to a path occupied by a directory, or copy/move a directory to a path occupied by a file.");
    SetErrorName(Error::DOM_ERROR_INVALID_MODIFICATION);
    return;
  }

  if (Path::IsParentOf(path, newPath)) {
    // Cannot copy/move a directory inside itself or to child at any depth
    SDCARD_LOG(
        "Cannot copy/move a directory inside itself or to child at any depth.");
    SetErrorName(Error::DOM_ERROR_INVALID_MODIFICATION);
    return;
  }

  // Delete the existing entry as nsIFile.coptTo()/moveTo() cannot overwrite
  if (newFileExits) {
    rv = mResultFile->Remove(false);
    if (NS_FAILED(rv) ) {
      SDCARD_LOG("Fail to remove existing target before copy/move.");
      SetErrorCode(rv);
      return;
    }
  }

  // assigne mResultFile back to original file to copy/move
  mFile->Clone(getter_AddRefs(mResultFile));

  // Actually copy/move the entry.
  if (mIsCopy) {
    rv = mResultFile->CopyTo(mNewParentDir, mNewName);
  } else {
    rv = mResultFile->MoveTo(mNewParentDir, mNewName);
  }
  if (NS_FAILED(rv) ) {
    // fail to copy/move
    SDCARD_LOG("Fail to copy/move.");
    SetErrorCode(rv);
    return;
  }
}

void
CopyAndMoveToRunnable::OnSuccess()
{
  SDCARD_LOG("in CopyAndMoveToRunnable.OnSuccess()!");

  if (mSuccessCallback) { // successCallback is optional
    ErrorResult rv;
    nsRefPtr<Entry> resultEntry = Entry::CreateFromFile(
        GetEntry()->GetFilesystem(), mResultFile.get());
    mSuccessCallback->Call(*resultEntry, rv);
  }
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
