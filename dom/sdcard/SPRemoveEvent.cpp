/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SPRemoveEvent.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

SPRemoveEvent::SPRemoveEvent(Caller* aCaller, const nsAString& aRelpath, bool aRecursive) :
    RemoveEvent(aRelpath, aRecursive),
    mCaller(aCaller)
{
  SDCARD_LOG("construct SPRemoveEvent");
}

SPRemoveEvent::~SPRemoveEvent()
{
  SDCARD_LOG("destruct SPRemoveEvent");
}

void SPRemoveEvent::OnError()
{
  SDCARD_LOG("in SPRemoveEvent.OnError()!");

  mCaller->CallErrorCallback(mErrorName);
}

void SPRemoveEvent::OnSuccess()
{
  SDCARD_LOG("in SPRemoveEvent.OnSuccess()!");

  mCaller->CallVoidCallback();
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
