/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ReadEntriesRunnable.h"
#include "nsISimpleEnumerator.h"
#include "Entry.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

ReadEntriesRunnable::ReadEntriesRunnable(EntriesCallback* aSuccessCallback,
    ErrorCallback* aErrorCallback,
    Entry* aEntry) :
    CombinedRunnable(aErrorCallback, aEntry),
    mSuccessCallback(aSuccessCallback)
{
SDCARD_LOG("init ReadEntriesRunnabl!");
  mFile = aEntry->GetFileInternal();
}

ReadEntriesRunnable::~ReadEntriesRunnable()
{
}

void ReadEntriesRunnable::WorkerThreadRun()
{
  SDCARD_LOG("in ReadEntriesRunnable.WorkerThreadRun()!");
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");

  nsresult rv = NS_OK;
  nsCOMPtr<nsISimpleEnumerator> childEnumerator;
  rv = mFile->GetDirectoryEntries(getter_AddRefs(childEnumerator));
  if (NS_FAILED(rv))
  {
    SetErrorCode(rv);
    return;
  }

  bool hasElements;
  while (NS_SUCCEEDED(childEnumerator->HasMoreElements(&hasElements))
      && hasElements) {
    nsCOMPtr<nsISupports> child;
    rv = childEnumerator->GetNext(getter_AddRefs(child));
    if (NS_FAILED(rv)) {
      SetErrorCode(rv);
      return;
    }

    nsCOMPtr<nsIFile> childFile = do_QueryInterface(child);

    nsRefPtr<Entry> entry;

    bool isDir;
    childFile->IsDirectory(&isDir);
    bool isFile;
    childFile->IsFile(&isFile);

    if (isDir || isFile) {
      mChildren.AppendElement(childFile);
    }
  }
}

void ReadEntriesRunnable::OnSuccess()
{
  SDCARD_LOG("in ReadEntriesRunnable.OnSuccess()!");
  MOZ_ASSERT(mSuccessCallback, "Must pass successCallback!");

  Sequence<OwningNonNull<Entry> > entries;
  int n = mChildren.Length();
  for (int i = 0; i < n; i++) {
    nsRefPtr<Entry> entry = Entry::CreateFromFile(GetEntry()->GetFilesystem(), mChildren[i].get());
    *entries.AppendElement() = entry.forget();
  }
  ErrorResult rv;
  mSuccessCallback->Call(entries, rv);
}


} // namespace sdcard
} // namespace dom
} // namespace mozilla
