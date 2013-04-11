/*
 * MoveToRunnable.h
 *
 *  Created on: Apr 10, 2013
 *      Author: yuan
 */

#pragma once

#include "FileSystemRunnable.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class CopyAndMoveToRunnable : public FileSystemRunnable
{
public:
  CopyAndMoveToRunnable(DirectoryEntry* aParent, const nsAString* aNewName,
      EntryCallback* aSuccessCallback,
      ErrorCallback* aErrorCallback,
      Entry* aEntry, bool aIsCopy);

  virtual ~CopyAndMoveToRunnable();

  // Overrides nsRunnable
  NS_IMETHOD Run() MOZ_OVERRIDE;
private:
  bool IsDirectoryEmpty(nsIFile* dir);

  nsRefPtr<EntryCallback> mSuccessCallback;
  nsRefPtr<DirectoryEntry> mNewParent;
  nsString mNewName;
  bool mIsCopy;
};

} /* namespace sdcard */
} /* namespace dom */
} /* namespace mozilla */
