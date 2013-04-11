/*
 * CombinedRunnable.h
 *
 *  Created on: Apr 11, 2013
 *      Author: yuan
 */

#pragma once

#include "nsThreadUtils.h"
#include "mozilla/dom/DOMError.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class Entry;

extern const nsString DOM_ERROR_ENCODING;
extern const nsString DOM_ERROR_INVALID_MODIFICATION;
extern const nsString DOM_ERROR_INVALID_STATE;
extern const nsString DOM_ERROR_NOT_FOUND;
extern const nsString DOM_ERROR_NOT_READABLE;
extern const nsString DOM_ERROR_NO_MODIFICATION_ALLOWED;
extern const nsString DOM_ERROR_PATH_EXISTS;
extern const nsString DOM_ERROR_QUOTA_EXCEEDED;
extern const nsString DOM_ERROR_SECURITY;
extern const nsString DOM_ERROR_TYPE_MISMATCH;
extern const nsString DOM_ERROR_UNKNOWN;

class CombinedRunnable : public nsRunnable
{
public:
  CombinedRunnable(Entry* entry);
  virtual ~CombinedRunnable();

  /*
   * Start the runnable thread.
   * First it will call WorkerThreadRun to perform worker thread operations.
   * After that it calls MainThreadRun to perform main thread operations.
   */
  void Start();

  // Overrides nsIRunnable
  NS_IMETHOD Run() MOZ_OVERRIDE;

protected:
  virtual void WorkerThreadRun() = 0;
  virtual void MainThreadRun() = 0;

  already_AddRefed<nsIDOMDOMError> GetDOMError() const;

  void SetErrorCode(nsresult errorCode)
  {
    mErrorCode = errorCode;
  }

  void SetErrorName(const nsString& errorName)
  {
    mErrorName = errorName;
  }

  Entry* GetEntry() const;

private:
  // It will only be used on main thread, so doesn't need a lock.
  static nsCOMPtr<nsIThread> sWorkerThread;
  // Not thread safe. Can't be used it in worker thread.
  nsRefPtr<Entry> mEntry;
  nsresult mErrorCode;
  nsString mErrorName;
};

} /* namespace sdcard */
} /* namespace dom */
} /* namespace mozilla */
