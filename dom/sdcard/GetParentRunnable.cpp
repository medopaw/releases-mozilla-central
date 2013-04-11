/*
 * GetParentRunnable.cpp
 *
 *  Created on: Apr 8, 2013
 *      Author: yuan
 */

#include "GetParentRunnable.h"
#include "Entry.h"
#include "Path.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

GetParentRunnable::GetParentRunnable(EntryCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback,
    Entry* aEntry) :
    CombinedRunnable(aEntry),
    mSuccessCallback(aSuccessCallback),
    mErrorCallback(aErrorCallback)
{
  mFile = aEntry->GetFileInternal();
}

GetParentRunnable::~GetParentRunnable()
{
}

void GetParentRunnable::WorkerThreadRun()
{
  SDCARD_LOG("in GetParentRunnable.WorkerThreadRun()!");
  nsString path;
  mFile->GetPath(path);
  if (Path::IsBase(path)) {
    // The parent folder of the root is itself.
    mParentFile = mFile;
  } else {
    nsresult rv = mFile->GetParent(getter_AddRefs(mParentFile));
    if (NS_FAILED(rv) ) {
      // Failed to copy/move
      SetErrorCode(rv);
    }
  }
}

void GetParentRunnable::MainThreadRun()
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
    ErrorResult rv;
    nsRefPtr<Entry> newEntry = Entry::FromFile(GetEntry()->GetFilesystem(),
        mParentFile.get());
    mSuccessCallback->Call(*newEntry, rv);
  }
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
