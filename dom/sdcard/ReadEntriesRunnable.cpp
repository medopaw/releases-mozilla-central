/*
 * ReadEntriesRunnable.cpp
 *
 *  Created on: Apr 10, 2013
 *      Author: yuan
 */

#include "ReadEntriesRunnable.h"
#include "nsISimpleEnumerator.h"
#include "FileEntry.h"
#include "DirectoryEntry.h"

namespace mozilla {
namespace dom {
namespace sdcard {

ReadEntriesRunnable::ReadEntriesRunnable(EntriesCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback,
    Entry* aEntry) :
    FileSystemRunnable(aErrorCallback, aEntry),
        mSuccessCallback(aSuccessCallback)
{
}

ReadEntriesRunnable::~ReadEntriesRunnable()
{
}


NS_IMETHODIMP ReadEntriesRunnable::Run()
{
  SDCARD_LOG("in ReadEntriesRunnable.Run()!");
  SDCARD_LOG("on main thread:%d", NS_IsMainThread());
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  nsCOMPtr<nsIRunnable> mainThreadRunnable;
  nsresult rv = NS_OK;

  nsCOMPtr<nsISimpleEnumerator> childEnumerator;
  rv = mEntry->GetFileInternal()->GetDirectoryEntries(
      getter_AddRefs(childEnumerator));
  if (NS_SUCCEEDED(rv) )
  {
    Sequence<OwningNonNull<Entry> >* entries = new Sequence<
        OwningNonNull<Entry> >();
    bool hasElements;
    while (NS_SUCCEEDED(childEnumerator->HasMoreElements(&hasElements))
        && hasElements) {
      nsCOMPtr<nsISupports> child;
      rv = childEnumerator->GetNext(getter_AddRefs(child));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIFile> childFile = do_QueryInterface(child);
      NS_ENSURE_TRUE(childFile, NS_NOINTERFACE);

      nsRefPtr<Entry> entry;

      bool isDir;
      rv = childFile->IsDirectory(&isDir);
      NS_ENSURE_SUCCESS(rv, rv);
      if (isDir) {
        entry = dynamic_cast<Entry *>(new DirectoryEntry(
            mEntry->GetFilesystem(), childFile));
      }

      bool isFile;
      rv = childFile->IsFile(&isFile);
      NS_ENSURE_SUCCESS(rv, rv);
      if (isFile) {
        entry = dynamic_cast<Entry *>(new FileEntry(mEntry->GetFilesystem(),
            childFile));
      }

      if (entry) {
        *entries->AppendElement() = entry.forget();
      }
    }
    // success callback
    mainThreadRunnable = new ResultRunnable<EntriesCallback,
        Sequence<OwningNonNull<Entry> >, true>(
        mSuccessCallback, entries);
  } else {
    // error callback
    if (mErrorCallback) {
      mainThreadRunnable = new ErrorRunnable(mErrorCallback, rv);
    }
  }

  if (mainThreadRunnable) {
    NS_DispatchToMainThread(mainThreadRunnable);
  }
  return NS_OK;
}

} /* namespace sdcard */
} /* namespace dom */
} /* namespace mozilla */
