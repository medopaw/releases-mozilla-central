/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "CombinedRunnable.h"

namespace mozilla {
namespace dom {
namespace sdcard {

/*
 * This Runnable is meant to call error callback on main thread.
 * So no worker thread related thing would be provided.
 * And CombinedRunnable::Start() won't be called, because it starts worker thread.
 * We'll use our own overwritten Start() instead.
 */
class ErrorRunnable : public CombinedRunnable
{
public:
  ErrorRunnable(ErrorCallback* aErrorCallback, const nsresult& aErrorCode);
  ErrorRunnable(ErrorCallback* aErrorCallback, const nsAString& aErrorName);
  ~ErrorRunnable();

  // overwrite CombinedRunnable::Start()
  void Start();

protected:
  // these two override function won't be used
  virtual void WorkerThreadRun() MOZ_OVERRIDE;
  virtual void OnSuccess() MOZ_OVERRIDE;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
