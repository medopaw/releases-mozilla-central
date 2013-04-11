/*
 * GetParentRunnable.cpp
 *
 *  Created on: Apr 8, 2013
 *      Author: yuan
 */

#include "GetParentRunnable.h"
#include "DirectoryEntry.h"

namespace mozilla {
namespace dom {
namespace sdcard {

GetParentRunnable::GetParentRunnable(EntryCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback,
    Entry* aEntry) :
    FileSystemRunnable(aErrorCallback, aEntry),
        mSuccessCallback(aSuccessCallback)
{
}

GetParentRunnable::~GetParentRunnable()
{
}

NS_IMETHODIMP GetParentRunnable::Run()
{
  SDCARD_LOG("in GetParentRunnable.Run()!");
  SDCARD_LOG("on main thread:%d", NS_IsMainThread());
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  nsCOMPtr<nsIRunnable> mainThreadRunnable;
  nsresult rv = NS_OK;
  Entry* parent = nullptr;

  if (mEntry->IsRoot()) {
    // The parent folder of the root is itself.
    parent = mEntry;
  } else {
    nsCOMPtr<nsIFile> parentFile;
    rv = mEntry->GetFileInternal()->GetParent(getter_AddRefs(parentFile));
    if (NS_SUCCEEDED(rv) ) {
      parent = new DirectoryEntry(mEntry->GetFilesystem(), parentFile);
    }
  }

  if (parent) {
    // success callback
    nsCOMPtr<nsIRunnable> r = new ResultRunnable<EntryCallback, Entry>(
        mSuccessCallback, parent);
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


} // namespace sdcard
} // namespace dom
} // namespace mozilla
