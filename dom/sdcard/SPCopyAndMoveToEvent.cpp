/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SPCopyAndMoveToEvent.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

SPCopyAndMoveToEvent::SPCopyAndMoveToEvent(
    Caller* aCaller,
    const nsAString& aRelpath,
    const nsAString& aParentPath,
    const nsAString& aNewName,
    bool aIsCopy) :
    CopyAndMoveToEvent(aRelpath, aParentPath, aNewName, aIsCopy),
    mCaller(aCaller)
{
  SDCARD_LOG("construct SPCopyAndMoveToEvent");
}

SPCopyAndMoveToEvent::~SPCopyAndMoveToEvent()
{
  SDCARD_LOG("destruct SPCopyAndMoveToEvent");
}

void SPCopyAndMoveToEvent::OnError()
{
  SDCARD_LOG("in SPCopyAndMoveToEvent.OnError()!");

  mCaller->CallErrorCallback(mErrorName);
}

void SPCopyAndMoveToEvent::OnSuccess()
{
  SDCARD_LOG("in SPCopyAndMoveToEvent.OnSuccess()!");

  mCaller->CallEntryCallback(mResultPath);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
