/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "RemoveEvent.h"
#include "Caller.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class SPRemoveEvent : public RemoveEvent
{
public:
  SPRemoveEvent(Caller* aCaller, const nsAString& aRelpath, bool aRecursive = false);
  ~SPRemoveEvent();

private:
  virtual void OnError() MOZ_OVERRIDE;
  virtual void OnSuccess() MOZ_OVERRIDE;

  nsRefPtr<Caller> mCaller;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
