/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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

  nsCOMPtr<nsIThread> mWorkerThread;
};

} /* namespace sdcard */
} /* namespace dom */
} /* namespace mozilla */
