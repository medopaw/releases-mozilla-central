/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "RemoveEvent.h"
#include "nsIFile.h"
#include "Path.h"
#include "Error.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

RemoveEvent::RemoveEvent(const nsAString& aRelpath, bool aRecursive) :
    SDCardEvent(aRelpath),
    mRecursive(aRecursive)
{
  SDCARD_LOG("construct RemoveEvent");
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
}

RemoveEvent::~RemoveEvent()
{
  SDCARD_LOG("destruct RemoveEvent");
}

void RemoveEvent::WorkerThreadRun()
{
  SDCARD_LOG("in RemoveEvent.WorkerThreadRun()!");
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  nsresult rv = NS_OK;
  if (Path::IsBase(mRelpath)) {
    // cannot remove root directory
    SetErrorName(Error::DOM_ERROR_NO_MODIFICATION_ALLOWED);
  } else {
    rv = mFile->Remove(mRecursive);
    if (NS_FAILED(rv)) {
      SetErrorCode(rv);
    }
  }
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
