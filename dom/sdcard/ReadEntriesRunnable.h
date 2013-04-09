/*
 * ReadEntriesRunnable.h
 *
 *  Created on: Apr 10, 2013
 *      Author: yuan
 */

#pragma once

#include "FileSystemRunnable.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class ReadEntriesRunnable : public FileSystemRunnable
{
public:
  ReadEntriesRunnable(EntriesCallback* aSuccessCallback,
      ErrorCallback* aErrorCallback,
      Entry* aEntry);

  virtual ~ReadEntriesRunnable();

  // Overrides nsRunnable
  NS_IMETHOD Run() MOZ_OVERRIDE;
private:
  nsRefPtr<EntriesCallback> mSuccessCallback;
};

} /* namespace sdcard */
} /* namespace dom */
} /* namespace mozilla */
