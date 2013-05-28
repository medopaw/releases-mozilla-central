/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SPGetEntryEvent.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

SPGetEntryEvent::SPGetEntryEvent(
    Caller* aCaller,
    const nsAString& aRelpath,
    bool aCreate,
    bool aExclusive,
    bool aIsFile) :
    GetEntryEvent(aRelpath, aCreate, aExclusive, aIsFile),
    mCaller(aCaller)
{
  SDCARD_LOG("construct SPGetEntryEvent");
}

SPGetEntryEvent::~SPGetEntryEvent()
{
  SDCARD_LOG("destruct SPGetEntryEvent");
}

void SPGetEntryEvent::OnError()
{
  SDCARD_LOG("in SPGetEntryEvent.OnError()!");

  mCaller->CallErrorCallback(mErrorName);
}

void SPGetEntryEvent::OnSuccess()
{
  SDCARD_LOG("in SPGetEntryEvent.OnSuccess()!");

  mCaller->CallEntryCallback(mResultPath);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
