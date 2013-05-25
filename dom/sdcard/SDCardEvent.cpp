/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SDCardEvent.h"
#include "Error.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

SDCardEvent::SDCardEvent(const nsAString& aRelpath) :
    mRelpath(aRelpath),
    mErrorCode(NS_OK),
    mWorkerThread(nullptr)
{
  SDCARD_LOG("construct SDCardEvent");
}

SDCardEvent::~SDCardEvent()
{
  SDCARD_LOG("destruct SDCardEvent");
}

void SDCardEvent::Start()
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

  // run worker thread
  if (!mWorkerThread) {
    nsresult rv = NS_NewThread(getter_AddRefs(mWorkerThread));
    if (NS_FAILED(rv)) {
      mWorkerThread = nullptr;
      // call error callback
      SetErrorCode(rv);
      MainThreadRun();
      return;
    }
  }
  mWorkerThread->Dispatch(this, NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP SDCardEvent::Run()
{
  if (!NS_IsMainThread()) {
    SDCARD_LOG("SDCardEvent.Run() on worker thread.");
    // resolve mFile using mRelPath first
    nsresult rv = NS_NewLocalFile(mRelpath, false, getter_AddRefs(mFile));
    if (NS_FAILED(rv)) {
      SDCARD_LOG("Error occurs when create mFile from mRelpath.");
      SetErrorCode(rv);
    } else {
      // run worker thread tasks: file operations
      WorkerThreadRun();
    }
    // dispatch itself to main thread
    NS_DispatchToMainThread(this);
  } else {
    SDCARD_LOG("SDCardEvent.Run() on main thread.");
    // shutdown mWorkerThread
    if (mWorkerThread) {
      mWorkerThread->Shutdown();
    }
    mWorkerThread = nullptr;
    // run main thread tasks: call callbacks
    MainThreadRun();
  }

  return NS_OK;
}

void SDCardEvent::MainThreadRun()
{
  SDCARD_LOG("in SDCardEvent.MainThreadRun()!");
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

  if (!mErrorName.IsEmpty()) {
    // Error::HandleError(mErrorCallback, mErrorName);
  } else if (mErrorCode != NS_OK) {
    // Error::HandleError(mErrorCallback, mErrorCode);
  } else {
    OnSuccess();
  }
}


void SDCardEvent::SetErrorCode(const nsresult& aErrorCode)
{
  mErrorCode = aErrorCode;
}

void SDCardEvent::SetErrorName(const nsAString& aErrorName)
{
  mErrorName = aErrorName;
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
