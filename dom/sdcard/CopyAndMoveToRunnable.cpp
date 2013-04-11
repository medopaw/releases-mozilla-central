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

nsCOMPtr<nsIThread> CopyAndMoveToRunnable::sWorkerThread;

CopyAndMoveToRunnable::CopyAndMoveToRunnable(DirectoryEntry* aParent,
    const nsAString* aNewName,
    EntryCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback,
    Entry* aEntry, bool aIsCopy) :
    mEntry(aEntry),
        mErrorCode(NS_OK),
        mSuccessCallback(aSuccessCallback),
        mErrorCallback(aErrorCallback),
        mIsCopy(aIsCopy)

{
  if (aNewName) {
    mNewName = *aNewName;
  } else {
    // If the no new name is specified, set it to the entry's current name.
    aEntry->GetName(mNewName);
  }
  mFile = aEntry->GetFileInternal();
  mNewParenFile = aParent->GetFileInternal();
}

CopyAndMoveToRunnable::~CopyAndMoveToRunnable()
{
}

void CopyAndMoveToRunnable::Start()
{
  // Run on worker thread.
  if (!sWorkerThread) {
    nsresult rv = NS_NewThread(getter_AddRefs(sWorkerThread));
    if (NS_FAILED(rv) ) {
      sWorkerThread = nullptr;
      return;
    }
  }
  sWorkerThread->Dispatch(this, NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP CopyAndMoveToRunnable::Run()
{
  SDCARD_LOG("in CopyAndMoveToRunnable.Run()!");
  SDCARD_LOG("on main thread:%d", NS_IsMainThread());
  if (!NS_IsMainThread()) {
    WorkerThreadRun();
    NS_DispatchToMainThread(this);
  } else {
    MainThreadRun();
  }

  return NS_OK;
}

void CopyAndMoveToRunnable::WorkerThreadRun()
{
  nsresult rv = NS_OK;

  nsString path;
  rv = mFile->GetPath(path);
  if (Path::IsBase(path)) {
    // Cannot copy/move the root directory
    mErrorName = DOM_ERROR_INVALID_MODIFICATION;
    return;
  }

  bool isFile = false;
  bool isDirectory = false;
  mFile->IsFile(&isFile);
  mFile->IsDirectory(&isDirectory);

  if (!(isFile || isDirectory)) {
    // Cannot copy/move a special file
    mErrorName = DOM_ERROR_INVALID_MODIFICATION;
    return;
  }

    // Create destination nsIFile object
  mNewParenFile->Clone(getter_AddRefs(mNewFile));
  rv = mNewFile->Append(mNewName);
  if (NS_FAILED(rv)) {
    mErrorCode = rv;
    return;
  }

  // Check if destination exists
  bool newFileExits = false;
  mNewFile->Exists(&newFileExits);

  // Whether the destination is a file
  bool isNewFile = false;
  // Whether the destination is a directory
  bool isNewDirectory = false;
  if (newFileExits) {
    mNewFile->IsFile(&isNewFile);
    mNewFile->IsDirectory(&isNewDirectory);
    if (!(isNewFile || isNewDirectory)) {
      // Cannot overwrite a special file
      mErrorName = DOM_ERROR_INVALID_MODIFICATION;
      return;
    }
  }

  nsString newPath;
  mNewFile->GetPath(newPath);

  // The destination is the same with the source
  if (path == newPath) {
    // Cannot copy/move an entry into its parent if a name different from its
    // current one isn't provided
    mNewName = DOM_ERROR_INVALID_MODIFICATION;
    return;
  }

  if (isNewDirectory && !IsDirectoryEmpty(mNewFile)) {
    // Cannot copy/move to a path occupied by a directory which is not empty.
    mErrorName = DOM_ERROR_INVALID_MODIFICATION;
    return;
  }

  if ((isFile && isNewDirectory) || (isDirectory && isNewFile)) {
    // Cannot copy/move a file to a path occupied by a directory, or
    // copy/move a directory to a path occupied by a file
    mErrorName = DOM_ERROR_INVALID_MODIFICATION;
    return;
  }

  if (Path::IsParentOf(path, newPath)) {
    // Cannot copy/move a directory inside itself or to child at any depth
    mErrorName = DOM_ERROR_INVALID_MODIFICATION;
    return;
  }

    // copy/Move the entry
  mFile->Clone(getter_AddRefs(mNewFile));

  if (mIsCopy) {
    rv = mNewFile->CopyTo(mNewParenFile, mNewName);
  } else {
    rv = mNewFile->MoveTo(mNewParenFile, mNewName);
  }
  if (NS_FAILED(rv) ) {
    // Failed to copy/move
    mErrorCode = rv;
    return;
  }
}

void CopyAndMoveToRunnable::MainThreadRun()
{
  nsRefPtr<nsIDOMDOMError> error = GetDOMError();
  if (error) {
    // error callback
    if (mErrorCallback) {
      ErrorResult rv;
      mErrorCallback->Call(error, rv);
    }
  } else {
    // success callback
    if (mSuccessCallback) {
      ErrorResult rv;
      nsRefPtr<Entry> newEntry = Entry::FromFile(mEntry->GetFilesystem(),
          mNewFile.get());
      mSuccessCallback->Call(*newEntry, rv);
    }
  }
}

already_AddRefed<nsIDOMDOMError> CopyAndMoveToRunnable::GetDOMError() const
{
  already_AddRefed<nsIDOMDOMError> domError = nullptr;
  nsString name;
  if (!mErrorName.IsEmpty()) {
    name = mErrorName;
  } else if (mErrorCode != NS_OK) {
    switch (mErrorCode) {
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
  }
  if (!name.IsEmpty()) {
    domError = DOMError::CreateWithName(mErrorName);
  }
  return domError;
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
