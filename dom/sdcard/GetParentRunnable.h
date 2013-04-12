/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "CombinedRunnable.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class GetParentRunnable : public CombinedRunnable
{
public:
  GetParentRunnable(EntryCallback* aSuccessCallback,
      ErrorCallback* aErrorCallback,
      Entry* aEntry);

  virtual ~GetParentRunnable();

protected:
  virtual void WorkerThreadRun() MOZ_OVERRIDE;
  virtual void OnSuccess() MOZ_OVERRIDE;

private:
  nsCOMPtr<nsIFile> mFile;
  nsCOMPtr<nsIFile> mParentFile;

  // not thread safe
  nsRefPtr<EntryCallback> mSuccessCallback;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
