/*
 * GetParentRunnable.h
 *
 *  Created on: Apr 8, 2013
 *      Author: yuan
 */

#pragma once

#include "FileSystemRunnable.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class GetParentRunnable : public FileSystemRunnable
{
public:
  GetParentRunnable(EntryCallback* aSuccessCallback,
      ErrorCallback* aErrorCallback,
      Entry* aEntry);

  virtual ~GetParentRunnable();

  // Overrides nsRunnable
  NS_IMETHOD Run() MOZ_OVERRIDE;

private:
  nsRefPtr<EntryCallback> mSuccessCallback;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
