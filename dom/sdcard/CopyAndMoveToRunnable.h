/*
 * MoveToRunnable.h
 *
 *  Created on: Apr 10, 2013
 *      Author: yuan
 */

#pragma once

#include "CombinedRunnable.h"
#include "mozilla/dom/FileSystemBinding.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class CopyAndMoveToRunnable : public CombinedRunnable
{
public:
  CopyAndMoveToRunnable(DirectoryEntry* aParent, const nsAString* aNewName,
      EntryCallback* aSuccessCallback,
      ErrorCallback* aErrorCallback,
      Entry* aEntry, bool aIsCopy);

  virtual ~CopyAndMoveToRunnable();

protected:
  virtual void WorkerThreadRun() MOZ_OVERRIDE;
  virtual void MainThreadRun() MOZ_OVERRIDE;

private:
  bool IsDirectoryEmpty(nsIFile* dir);

  nsCOMPtr<nsIFile> mFile;
  nsCOMPtr<nsIFile> mNewParentDir;
  nsCOMPtr<nsIFile> mResultFile;
  nsString mNewName;

  // not thread safe
  nsRefPtr<EntryCallback> mSuccessCallback;
  // not thread safe
  nsRefPtr<ErrorCallback> mErrorCallback;
  bool mIsCopy;
};

} /* namespace sdcard */
} /* namespace dom */
} /* namespace mozilla */
