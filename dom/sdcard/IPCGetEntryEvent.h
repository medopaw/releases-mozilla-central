/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "IPCEvent.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class IPCGetEntryEvent : public IPCEvent
{
public:
  IPCGetEntryEvent(
      const nsAString& aRelpath,
      bool aCreate,
      bool aExclusive,
      bool aIsFile,
      SDCardRequestParent* aParent);
  virtual ~IPCGetEntryEvent();

private:
  virtual void OnSuccess() MOZ_OVERRIDE;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
