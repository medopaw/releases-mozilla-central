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

class CopyAndMoveToRunnable : public nsRunnable
{
public:
  CopyAndMoveToRunnable(DirectoryEntry* aParent, const nsAString* aNewName,
      EntryCallback* aSuccessCallback,
      ErrorCallback* aErrorCallback,
      Entry* aEntry, bool aIsCopy);

  virtual ~CopyAndMoveToRunnable();

  /*
   * Start the runnable thread.
   * First it will call WorkerThreadRun to perform worker thread operations.
   * After that it calls MainThreadRun to perform main thread operations.
   */
  void Start();

  // Overrides nsIRunnable
  NS_IMETHOD Run() MOZ_OVERRIDE;

protected:
  virtual void WorkerThreadRun();
  virtual void MainThreadRun();

  already_AddRefed<nsIDOMDOMError> GetDOMError() const;

private:
  // not thread safe. don't use it in worker thread.
  nsRefPtr<Entry> mEntry;

  nsCOMPtr<nsIFile> mFile;
  nsCOMPtr<nsIFile> mNewParenFile;
  nsCOMPtr<nsIFile> mNewFile;
  nsString mNewName;
  nsresult mErrorCode;
  nsString mErrorName;

private:
  bool IsDirectoryEmpty(nsIFile* dir);

  // It will only be used on main thread, so doesn't need a lock.
  static nsCOMPtr<nsIThread> sWorkerThread;

  // not thread safe
  nsRefPtr<EntryCallback> mSuccessCallback;
  // not thread safe
  nsRefPtr<ErrorCallback> mErrorCallback;
  bool mIsCopy;
};

} /* namespace sdcard */
} /* namespace dom */
} /* namespace mozilla */
