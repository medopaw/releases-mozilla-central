/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "GetParentRunnable.h"
#include "Entry.h"
#include "Path.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

GetParentRunnable::GetParentRunnable(EntryCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback, Entry* aEntry) :
    CombinedRunnable(aErrorCallback, aEntry),
    mSuccessCallback(aSuccessCallback)
{
  SDCARD_LOG("construct GetParentRunnable!");
  mFile = aEntry->GetFileInternal();
}

GetParentRunnable::~GetParentRunnable()
{
  SDCARD_LOG("destruct GetParentRunnable!");
}

void
GetParentRunnable::WorkerThreadRun()
{
  SDCARD_LOG("in GetParentRunnable.WorkerThreadRun()!");
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  nsString path;
  mFile->GetPath(path);
  if (Path::IsBase(path)) {
    // The parent folder of the root is itself.
    mParentFile = mFile;
  } else {
    nsresult rv = mFile->GetParent(getter_AddRefs(mParentFile));
    if (NS_FAILED(rv) ) {
      // Failed to copy/move
      SetErrorCode(rv);
    }
  }
}

void
GetParentRunnable::OnSuccess()
{
  SDCARD_LOG("in GetParentRunnable.OnSuccess()!");
  MOZ_ASSERT(mSuccessCallback, "Must pass successCallback!");

  ErrorResult rv;
  nsRefPtr<Entry> resultEntry = Entry::CreateFromFile(
      GetEntry()->GetFilesystem(), mParentFile.get());
  mSuccessCallback->Call(*resultEntry, rv);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
