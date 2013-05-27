/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "SDCardEvent.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class CopyAndMoveToEvent : public SDCardEvent
{
public:
  CopyAndMoveToEvent(
      const nsAString& aRelpath,
      const nsAString& aParentPath,
      const nsAString& aNewName,
      bool aIsCopy);

  virtual ~CopyAndMoveToEvent();

protected:
  virtual void WorkerThreadRun() MOZ_OVERRIDE;
  nsString mResultPath;

private:
  nsString mParentPath;
  nsString mNewName;
  bool mIsCopy;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
