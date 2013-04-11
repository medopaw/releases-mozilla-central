/*
 * ReadEntriesRunnable.cpp
 *
 *  Created on: Apr 10, 2013
 *      Author: yuan
 */

#include "ReadEntriesRunnable.h"
#include "nsISimpleEnumerator.h"
#include "Entry.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

ReadEntriesRunnable::ReadEntriesRunnable(EntriesCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback,
    Entry* aEntry) :
    CombinedRunnable(aEntry),
    mSuccessCallback(aSuccessCallback),
    mErrorCallback(aErrorCallback)
{
  mFile = aEntry->GetFileInternal();
}

ReadEntriesRunnable::~ReadEntriesRunnable()
{
}

void ReadEntriesRunnable::WorkerThreadRun()
{
  SDCARD_LOG("in ReadEntriesRunnable.WorkerThreadRun()!");
  nsresult rv = NS_OK;

  nsCOMPtr<nsISimpleEnumerator> childEnumerator;
  rv = mFile->GetDirectoryEntries(getter_AddRefs(childEnumerator));
  if (NS_FAILED(rv))
  {
    SetErrorCode(rv);
  }

  bool hasElements;
  while (NS_SUCCEEDED(childEnumerator->HasMoreElements(&hasElements))
      && hasElements) {
    nsCOMPtr<nsISupports> child;
    rv = childEnumerator->GetNext(getter_AddRefs(child));
    if (NS_FAILED(rv)) {
      SetErrorCode(rv);
      return;
    }

    nsCOMPtr<nsIFile> childFile = do_QueryInterface(child);

    nsRefPtr<Entry> entry;

    bool isDir;
    childFile->IsDirectory(&isDir);
    bool isFile;
    childFile->IsFile(&isFile);

    if (isDir || isFile) {
      mChildren.AppendElement(childFile);
    }
  }
}

void ReadEntriesRunnable::MainThreadRun()
{
  SDCARD_LOG("in GetParentRunnable.MainThreadRun()!");
  nsRefPtr<nsIDOMDOMError> error = GetDOMError();
  if (error) {
    // error callback
    if (mErrorCallback) {
      ErrorResult rv;
      mErrorCallback->Call(error, rv);
    }
  } else {
    // success callback
    Sequence<OwningNonNull<Entry> > entries;
    int n = mChildren.Length();
    for (int i = 0; i < n; i++) {
      nsRefPtr<Entry> entry = Entry::FromFile(GetEntry()->GetFilesystem(),
          mChildren[i].get());
      *entries.AppendElement() = entry.forget();
    }
    ErrorResult rv;
    mSuccessCallback->Call(entries, rv);
  }
}


} /* namespace sdcard */
} /* namespace dom */
} /* namespace mozilla */
