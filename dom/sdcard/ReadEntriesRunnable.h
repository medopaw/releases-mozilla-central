/*
 * ReadEntriesRunnable.h
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

class ReadEntriesRunnable : public CombinedRunnable
{
public:
  ReadEntriesRunnable(EntriesCallback* aSuccessCallback,
      ErrorCallback* aErrorCallback,
      Entry* aEntry);

  virtual ~ReadEntriesRunnable();

protected:
  virtual void WorkerThreadRun() MOZ_OVERRIDE;
  virtual void MainThreadRun() MOZ_OVERRIDE;

private:
  nsCOMPtr<nsIFile> mFile;
  nsTArray<nsCOMPtr<nsIFile> > mChildren;

  // not thread safe
  nsRefPtr<EntriesCallback> mSuccessCallback;
  // not thread safe
  nsRefPtr<ErrorCallback> mErrorCallback;
};

} /* namespace sdcard */
} /* namespace dom */
} /* namespace mozilla */
