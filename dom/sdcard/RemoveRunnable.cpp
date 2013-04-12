/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "RemoveRunnable.h"
#include "Entry.h"
#include "Path.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

RemoveRunnable::RemoveRunnable(
    VoidCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback,
    Entry* aEntry,
    bool aRecursive) :
    CombinedRunnable(aErrorCallback, aEntry),
    mSuccessCallback(aSuccessCallback),
    mRecursive(aRecursive)
{
  SDCARD_LOG("init RemoveRunnable");
  mFile = aEntry->GetFileInternal();
}

RemoveRunnable::~RemoveRunnable()
{
}

void RemoveRunnable::WorkerThreadRun()
{
  SDCARD_LOG("in RemoveRunnable.WorkerThreadRun()!");
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  nsresult rv = NS_OK;
  nsString path;
  mFile->GetPath(path);
  if (Path::IsBase(path)) {
    // cannot remove root directory
    SetErrorName(DOM_ERROR_NO_MODIFICATION_ALLOWED);
    return;
  } else {
    rv = mFile->Remove(mRecursive);
    if (NS_FAILED(rv)) {
      SetErrorCode(rv);
    }
  }
}

void RemoveRunnable::OnSuccess()
{
  SDCARD_LOG("in RemoveRunnable.MainThreadRun()!");
  MOZ_ASSERT(mSuccessCallback, "Must pass successCallback!");

  ErrorResult rv;
  mSuccessCallback->Call(rv);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
