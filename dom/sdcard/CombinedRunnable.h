/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "nsThreadUtils.h"
#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/FileSystemBinding.h"

namespace mozilla {
namespace dom {
namespace sdcard {

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

class Entry;

class CombinedRunnable : public nsRunnable
{
public:
  CombinedRunnable(ErrorCallback* aErrorCallback, Entry* entry);
  virtual ~CombinedRunnable();

  /*
   * Start the runnable thread.
   * First it calls WorkerThreadRun to perform worker thread operations.
   * After that it calls MainThreadRun to perform main thread operations.
   */
  void Start();

  // overrides nsIRunnable
  NS_IMETHOD Run() MOZ_OVERRIDE;

protected:
  virtual void WorkerThreadRun() = 0;
  void MainThreadRun();
  virtual void OnSuccess() = 0;

  void SetErrorCode(nsresult errorCode);
  void SetErrorName(const nsString& errorName);

  Entry* GetEntry() const;

private:
  already_AddRefed<nsIDOMDOMError> GetDOMError() const;

  nsresult mErrorCode;
  nsString mErrorName;

  nsRefPtr<ErrorCallback> mErrorCallback;

  // Not thread safe. Can't be used on worker thread.
  nsRefPtr<Entry> mEntry;

  // It will only be used on main thread, so doesn't need a lock.
  nsCOMPtr<nsIThread> mWorkerThread;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
