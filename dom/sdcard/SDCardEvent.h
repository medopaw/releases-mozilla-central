/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "nsThreadUtils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class SDCardEvent : public nsRunnable
{
public:
  SDCardEvent(const nsAString& aRelpath);
  virtual ~SDCardEvent();

  /*
   * Start the runnable thread.
   * First it calls WorkerThreadRun() to perform worker thread operations.
   * After that it calls MainThreadRun() to perform main thread operations.
   */
  void Start();

  // overrides nsIRunnable
  NS_IMETHOD Run() MOZ_OVERRIDE;

protected:
  virtual void WorkerThreadRun() = 0;
  virtual void OnError() = 0;
  virtual void OnSuccess() = 0;

  void SetErrorCode(const nsresult& aErrorCode);
  void SetErrorName(const nsAString& aErrorName);

  nsString mRelpath;
  // not thread safe, only access it form worker thread
  nsCOMPtr<nsIFile> mFile;

  nsString mErrorName;

private:
  void MainThreadRun();

  nsresult mErrorCode;

  // It will only be used on main thread, so doesn't need a lock.
  nsCOMPtr<nsIThread> mWorkerThread;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
