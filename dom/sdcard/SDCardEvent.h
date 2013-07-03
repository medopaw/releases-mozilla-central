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

class Worker;

class SDCardEvent : public nsRunnable
{
public:
  SDCardEvent(Worker* aWorker);
  virtual ~SDCardEvent();

  /*
   * Start the runnable thread.
   * First it calls WorkerThreadRun() to perform worker thread operations.
   * After that it calls MainThreadRun() to perform main thread operations.
   */
  void Start();

  // Overrides nsIRunnable.
  NS_IMETHOD Run() MOZ_OVERRIDE;

  void Cancel();

protected:
  bool mCanceled;

  nsRefPtr<Worker> mWorker;
  virtual void OnError() = 0;
  virtual void OnSuccess() = 0;

  virtual void HandleResult();

private:
  // It will only be used on main thread, so doesn't need a lock.
  nsCOMPtr<nsIThread> mWorkerThread;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
