/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CombinedRunnable.h"
#include "Entry.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsString.h"
#include "Error.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

CombinedRunnable::CombinedRunnable(ErrorCallback* aErrorCallback,
    Entry* aEntry) :
    mErrorCallback(aErrorCallback),
    mEntry(aEntry),
    mErrorCode(NS_OK),
    mWorkerThread(nullptr)
{
  SDCARD_LOG("construct CombinedRunnable");
}

CombinedRunnable::~CombinedRunnable()
{
  SDCARD_LOG("destruct CombinedRunnable");
}

void
CombinedRunnable::Start()
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

  // run worker thread
  if (!mWorkerThread) {
    nsresult rv = NS_NewThread(getter_AddRefs(mWorkerThread));
    if (NS_FAILED(rv) ) {
      mWorkerThread = nullptr;
      // call error callback
      SetErrorCode(rv);
      MainThreadRun();
      return;
    }
  }
  mWorkerThread->Dispatch(this, NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP
CombinedRunnable::Run()
{
  if (!NS_IsMainThread()) {
    SDCARD_LOG("CombinedRunnable.Run() on worker thread.");
    // run worker thread tasks: file operations
    WorkerThreadRun();
    // dispatch itself to main thread
    NS_DispatchToMainThread(this);
  } else {
    SDCARD_LOG("CombinedRunnable.Run() on main thread.");
    // shutdown mWorkerThread
    if (mWorkerThread) {
      mWorkerThread->Shutdown();
    }
    mWorkerThread = nullptr;
    // run main thread tasks: call callbacks
    MainThreadRun();
    // ensure mEntry is released on main thread
    mEntry = nullptr;
  }

  return NS_OK;
}

void
CombinedRunnable::MainThreadRun()
{
  SDCARD_LOG("in CombinedRunnable.MainThreadRun()!");
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

  if (!mErrorName.IsEmpty()) {
    Error::HandleError(mErrorCallback, mErrorName);
  } else if (mErrorCode != NS_OK) {
    Error::HandleError(mErrorCallback, mErrorCode);
  } else {
    OnSuccess();
  }
}

void
CombinedRunnable::SetErrorCode(const nsresult& aErrorCode)
{
  mErrorCode = aErrorCode;
}

void
CombinedRunnable::SetErrorName(const nsAString& aErrorName)
{
  mErrorName = aErrorName;
}

Entry*
CombinedRunnable::GetEntry() const
{
  MOZ_ASSERT(NS_IsMainThread(), "only call on main thread!");
  return NS_IsMainThread() ? mEntry : nullptr;
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
