/*
 * GetParentRunnable.h
 *
 *  Created on: Apr 8, 2013
 *      Author: yuan
 */

#pragma once

#include "CombinedRunnable.h"
#include "mozilla/dom/FileSystemBinding.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class GetParentRunnable : public CombinedRunnable
{
public:
  GetParentRunnable(EntryCallback* aSuccessCallback,
      ErrorCallback* aErrorCallback,
      Entry* aEntry);

  virtual ~GetParentRunnable();

protected:
  virtual void WorkerThreadRun() MOZ_OVERRIDE;
  virtual void MainThreadRun() MOZ_OVERRIDE;

private:
  nsCOMPtr<nsIFile> mFile;
  nsCOMPtr<nsIFile> mParentFile;

  // not thread safe
  nsRefPtr<EntryCallback> mSuccessCallback;
  // not thread safe
  nsRefPtr<ErrorCallback> mErrorCallback;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
