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

class GetEntryRunnable : public CombinedRunnable
{
public:
  GetEntryRunnable(const nsAString& aPath,
  bool aCreate, bool aExclusive,
  const unsigned long aType,
  EntryCallback* aSuccessCallback,
  ErrorCallback* aErrorCallback,
  Entry* aEntry);

  virtual ~GetEntryRunnable();

protected:
  virtual void WorkerThreadRun() MOZ_OVERRIDE;
  virtual void MainThreadRun() MOZ_OVERRIDE;

private:
  bool Exists(nsIFile* aFile);

  nsCOMPtr<nsIFile> mResultFile;

  nsString mPath;
  bool mCreate;
  bool mExclusive;
  const unsigned long mType;

  // not thread safe
  nsRefPtr<EntryCallback> mSuccessCallback;
  // not thread safe
  nsRefPtr<ErrorCallback> mErrorCallback;
};

} /* namespace sdcard */
} /* namespace dom */
} /* namespace mozilla */
