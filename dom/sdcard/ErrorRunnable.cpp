/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ErrorRunnable.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

ErrorRunnable::ErrorRunnable(
    ErrorCallback* aErrorCallback,
    const nsresult& aErrorCode) :
    CombinedRunnable(aErrorCallback, nullptr)
{
  SDCARD_LOG("init ErrorRunnable");
  SetErrorCode(aErrorCode);
}

ErrorRunnable::ErrorRunnable(
    ErrorCallback* aErrorCallback,
    const nsAString& aErrorName) :
    CombinedRunnable(aErrorCallback, nullptr)
{
  SDCARD_LOG("init ErrorRunnable");
  SetErrorName(aErrorName);
}

ErrorRunnable::~ErrorRunnable()
{
}

void ErrorRunnable::Start()
{
  SDCARD_LOG("in ErrorRunnable.Start()");
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

  // only the main thread part of Run() will be used
  NS_DispatchToMainThread(this);
}

void ErrorRunnable::WorkerThreadRun()
{
  MOZ_ASSERT(false, "We should never be in ErrorRunnable.WorkerThreadRun()!");
}

void ErrorRunnable::OnSuccess()
{
  MOZ_ASSERT(false, "We should never be in ErrorRunnable.OnSuccess()!");
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
