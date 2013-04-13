/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CopyAndMoveToRunnable.h"
#include "DirectoryEntry.h"
#include "FileEntry.h"
#include "Path.h"

namespace mozilla {
namespace dom {
namespace sdcard {

CopyAndMoveToRunnable::CopyAndMoveToRunnable(
    DirectoryEntry* aParent,
    const nsAString* aNewName,
    EntryCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback,
    Entry* aEntry,
    bool aIsCopy) :
    CombinedRunnable(aErrorCallback, aEntry),
    mSuccessCallback(aSuccessCallback),
    mIsCopy(aIsCopy)
{
  SDCARD_LOG("init CopyAndMoveToRunnable");
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

  if (aNewName && !aNewName->IsEmpty()) {
    mNewName = *aNewName;
  } else { // if new name not specified, set it to the entry's current name
    aEntry->GetName(mNewName);
  }
  mFile = aEntry->GetFileInternal();
  mNewParentDir = aParent->GetFileInternal();
}

CopyAndMoveToRunnable::~CopyAndMoveToRunnable()
{
}

void CopyAndMoveToRunnable::WorkerThreadRun()
{
  SDCARD_LOG("in CopyAndMoveToRunnable.WorkerThreadRun()!");
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  nsresult rv = NS_OK;
  nsString path;
  rv = mFile->GetPath(path);
  if (Path::IsBase(path)) {
    // Cannot copy/move the root directory
    SetErrorName(DOM_ERROR_INVALID_MODIFICATION);
    return;
  }

  bool isFile = false;
  bool isDirectory = false;
  mFile->IsFile(&isFile);
  mFile->IsDirectory(&isDirectory);

  if (!(isFile || isDirectory)) {
    // Cannot copy/move a special file
    SetErrorName(DOM_ERROR_INVALID_MODIFICATION);
    return;
  }

    // Create destination nsIFile object
  mNewParentDir->Clone(getter_AddRefs(mResultFile));
  rv = mResultFile->Append(mNewName);
  if (NS_FAILED(rv)) {
    SetErrorCode(rv);
    return;
  }

  // Check if destination exists
  bool newFileExits = false;
  mResultFile->Exists(&newFileExits);

  // Whether the destination is a file
  bool isNewFile = false;
  // Whether the destination is a directory
  bool isNewDirectory = false;
  if (newFileExits) {
    mResultFile->IsFile(&isNewFile);
    mResultFile->IsDirectory(&isNewDirectory);
    if (!(isNewFile || isNewDirectory)) {
      // Cannot overwrite a special file
      SetErrorName(DOM_ERROR_INVALID_MODIFICATION);
      return;
    }
  }

  nsString newPath;
  mResultFile->GetPath(newPath);

  // The destination is the same with the source
  if (path == newPath) {
    // Cannot copy/move an entry into its parent if a name different from its
    // current one isn't provided
    SetErrorName(DOM_ERROR_INVALID_MODIFICATION);
    return;
  }

  if (isNewDirectory && !IsDirectoryEmpty(mResultFile)) {
    // Cannot copy/move to a path occupied by a directory which is not empty.
    SetErrorName(DOM_ERROR_INVALID_MODIFICATION);
    return;
  }

  if ((isFile && isNewDirectory) || (isDirectory && isNewFile)) {
    // Cannot copy/move a file to a path occupied by a directory, or
    // copy/move a directory to a path occupied by a file
    SetErrorName(DOM_ERROR_INVALID_MODIFICATION);
    return;
  }

  if (Path::IsParentOf(path, newPath)) {
    // Cannot copy/move a directory inside itself or to child at any depth
    SetErrorName(DOM_ERROR_INVALID_MODIFICATION);
    return;
  }

    // copy/Move the entry
  mFile->Clone(getter_AddRefs(mResultFile));

  if (mIsCopy) {
    rv = mResultFile->CopyTo(mNewParentDir, mNewName);
  } else {
    rv = mResultFile->MoveTo(mNewParentDir, mNewName);
  }
  if (NS_FAILED(rv) ) {
    // failed to copy/move
    SetErrorCode(rv);
    return;
  }
}

void CopyAndMoveToRunnable::OnSuccess()
{
  SDCARD_LOG("in CopyAndMoveToRunnable.OnSuccess()!");

  if (mSuccessCallback) { // successCallback is optional
    ErrorResult rv;
    nsRefPtr<Entry> resultEntry = Entry::FromFile(GetEntry()->GetFilesystem(), mResultFile.get());
    mSuccessCallback->Call(*resultEntry, rv);
  }
}

bool CopyAndMoveToRunnable::IsDirectoryEmpty(nsIFile* dir)
{
  nsCOMPtr<nsISimpleEnumerator> childEnumerator;
  nsresult rv = dir->GetDirectoryEntries(getter_AddRefs(childEnumerator));
  if (NS_SUCCEEDED(rv) )
  {
    bool hasElements;
    while (NS_SUCCEEDED(childEnumerator->HasMoreElements(&hasElements))
        && hasElements) {
      return false;
    }
  }
  return true;
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
