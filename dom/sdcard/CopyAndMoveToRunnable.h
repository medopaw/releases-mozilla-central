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

class CopyAndMoveToRunnable : public CombinedRunnable
{
public:
  CopyAndMoveToRunnable(DirectoryEntry* aParent, const nsAString* aNewName, bool aIsCopy,
      EntryCallback* aSuccessCallback,
      ErrorCallback* aErrorCallback,
      Entry* aEntry);

  virtual ~CopyAndMoveToRunnable();

protected:
  virtual void WorkerThreadRun() MOZ_OVERRIDE;
  virtual void OnSuccess() MOZ_OVERRIDE;

private:
  bool IsDirectoryEmpty(nsIFile* dir);

  nsCOMPtr<nsIFile> mFile;
  nsCOMPtr<nsIFile> mNewParentDir;
  nsCOMPtr<nsIFile> mResultFile;
  nsString mNewName;

  // not thread safe
  nsRefPtr<EntryCallback> mSuccessCallback;
  bool mIsCopy;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
