/*
 * MoveToRunnable.cpp
 *
 *  Created on: Apr 10, 2013
 *      Author: yuan
 */

#include "CopyAndMoveToRunnable.h"
#include "DirectoryEntry.h"
#include "FileEntry.h"
#include "Path.h"

namespace mozilla {
namespace dom {
namespace sdcard {

CopyAndMoveToRunnable::CopyAndMoveToRunnable(DirectoryEntry* aParent,
    const nsAString* aNewName,
    EntryCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback,
    Entry* aEntry, bool aIsCopy) :
    FileSystemRunnable(aErrorCallback, aEntry),
        mSuccessCallback(aSuccessCallback),
        mNewParent(aParent),
        mIsCopy(aIsCopy)

{
  if (aNewName) {
    mNewName = *aNewName;
  } else {
    // If the no new name is specified, set it to the entry's current name.
    mEntry->GetName(mNewName);
  }
}

CopyAndMoveToRunnable::~CopyAndMoveToRunnable()
{
}

NS_IMETHODIMP CopyAndMoveToRunnable::Run()
{
  SDCARD_LOG("in CopyAndMoveToRunnable.Run()!");
  SDCARD_LOG("on main thread:%d", NS_IsMainThread());
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  nsCOMPtr<nsIRunnable> mainThreadRunnable;
  nsresult rv = NS_OK;

  if (mEntry->IsRoot() || !(mEntry->IsFile() || mEntry->IsDirectory())) {
    // Cannot copy/move the root directory and special files
    if (mErrorCallback) {
      nsString errorName = DOM_ERROR_INVALID_MODIFICATION;
      mainThreadRunnable = new ErrorRunnable(mErrorCallback, errorName);
      NS_DispatchToMainThread(mainThreadRunnable);
    }
    return NS_OK;
  }

      // Create destination nsIFile object
  nsCOMPtr<nsIFile> newFile;
  mNewParent->GetFileInternal()->Clone(getter_AddRefs(newFile));
  rv = newFile->Append(mNewName);
  if (NS_FAILED(rv) ) {
    if (mErrorCallback) {
      mainThreadRunnable = new ErrorRunnable(mErrorCallback, rv);
      NS_DispatchToMainThread(mainThreadRunnable);
    }
    return NS_OK;
  }

  // Check if destination exists
  bool newFileExits = false;
  rv = newFile->Exists(&newFileExits);
  NS_ENSURE_SUCCESS(rv, rv);

  // Whether the destination is a file
  bool isNewFile = false;
  // Whether the destination is a directory
  bool isNewDirectory = false;
  if (newFileExits) {
    rv = newFile->IsFile(&isNewFile);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = newFile->IsDirectory(&isNewDirectory);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!(isNewFile || isNewDirectory)) {
      // Cannot overwrite a special file
      if (mErrorCallback) {
        nsString errorName = DOM_ERROR_INVALID_MODIFICATION;
        mainThreadRunnable = new ErrorRunnable(mErrorCallback, errorName);
        NS_DispatchToMainThread(mainThreadRunnable);
      }
      return NS_OK;
    }
  }

  nsString path;
  rv = mEntry->GetFileInternal()->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);
  nsString newPath;
  rv = newFile->GetPath(newPath);
  NS_ENSURE_SUCCESS(rv, rv);

  // The destination is the same with the source
  if (path == newPath) {
    // Cannot copy/move an entry into its parent if a name different from its
    // current one isn't provided
    if (mErrorCallback) {
      nsString errorName = DOM_ERROR_INVALID_MODIFICATION;
      mainThreadRunnable = new ErrorRunnable(mErrorCallback, errorName);
      NS_DispatchToMainThread(mainThreadRunnable);
    }
    return NS_OK;
  }

  if (isNewDirectory && !IsDirectoryEmpty(newFile)) {
    // Cannot copy/move to a path occupied by a directory which is not empty.
    if (mErrorCallback) {
      nsString errorName = DOM_ERROR_INVALID_MODIFICATION;
      mainThreadRunnable = new ErrorRunnable(mErrorCallback, errorName);
      NS_DispatchToMainThread(mainThreadRunnable);
    }
    return NS_OK;
  }

  if (mEntry->IsFile()) {
    // Move a file

    if (isNewDirectory) {
      // Cannot copy/move a file to a path occupied by a directory
      if (mErrorCallback) {
        nsString errorName = DOM_ERROR_INVALID_MODIFICATION;
        mainThreadRunnable = new ErrorRunnable(mErrorCallback, errorName);
        NS_DispatchToMainThread(mainThreadRunnable);
      }
      return NS_OK;
    }
  } else {
    // Move a directory
            if (isNewFile) {
              // Cannot copy/move a directory to a path occupied by a file
              if (mErrorCallback) {
                nsString errorName = DOM_ERROR_INVALID_MODIFICATION;
                mainThreadRunnable = new ErrorRunnable(mErrorCallback, errorName);
                NS_DispatchToMainThread(mainThreadRunnable);
              }
              return NS_OK;
            }

            if (Path::IsParentOf(path, newPath)) {
              // Cannot copy/move a directory inside itself or to child at any depth
              if (mErrorCallback) {
                nsString errorName = DOM_ERROR_INVALID_MODIFICATION;
                mainThreadRunnable = new ErrorRunnable(mErrorCallback, errorName);
              }
              return NS_OK;
            }
          }

        // copy/Move the entry
  mEntry->GetFileInternal()->Clone(getter_AddRefs(newFile));
  if (mIsCopy) {
    rv = newFile->CopyTo(mNewParent->GetFileInternal(), mNewName);
  } else {
    rv = newFile->MoveTo(mNewParent->GetFileInternal(), mNewName);
  }
  if (NS_FAILED(rv) ) {
    // Failed to copy/move
    if (mErrorCallback) {
      mainThreadRunnable = new ErrorRunnable(mErrorCallback, rv);
      NS_DispatchToMainThread(mainThreadRunnable);
    }
    return NS_OK;
  }

  // success callback
  nsRefPtr<Entry> newEntry;
  if (mEntry->IsFile()) {
    newEntry = dynamic_cast<Entry *>(new FileEntry(mEntry->GetFilesystem(),
        newFile));
  } else {
    newEntry = dynamic_cast<Entry *>(new DirectoryEntry(mEntry->GetFilesystem(),
        newFile));
  }
  if (mSuccessCallback) {
    mainThreadRunnable = new ResultRunnable<EntryCallback, Entry>(
        mSuccessCallback, newEntry);
    NS_DispatchToMainThread(mainThreadRunnable);
  }
  return NS_OK;
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

} /* namespace sdcard */
} /* namespace dom */
} /* namespace mozilla */
