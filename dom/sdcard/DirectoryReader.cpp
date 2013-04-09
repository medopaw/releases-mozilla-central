/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DirectoryReader.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"
#include "ReadEntriesRunnable.h"
#include "DirectoryEntry.h"

namespace mozilla {
namespace dom {
namespace sdcard {


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(DirectoryReader)
NS_IMPL_CYCLE_COLLECTING_ADDREF(DirectoryReader)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DirectoryReader)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DirectoryReader)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

DirectoryReader::DirectoryReader(DirectoryEntry* entry) :
    mEntry(entry)
{
  SetIsDOMBinding();
}

DirectoryReader::~DirectoryReader()
{
}

JSObject*
DirectoryReader::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return DirectoryReaderBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

void DirectoryReader::ReadEntries(EntriesCallback& successCallback,
    const Optional<OwningNonNull<ErrorCallback> >& errorCallback)
{
  nsCOMPtr<nsIThread> thread;
  nsresult rv = NS_NewThread(getter_AddRefs(thread));
  ErrorCallback* errorCallbackPtr = nullptr;
  if (errorCallback.WasPassed()) {
    errorCallbackPtr = errorCallback.Value().get();
  }
  if (NS_FAILED(rv) ) {
    if (errorCallbackPtr) {
      nsString errorName = DOM_ERROR_UNKNOWN;
      nsCOMPtr<nsIRunnable> r = new ErrorRunnable(errorCallbackPtr, errorName);
      NS_DispatchToMainThread(r);
    }
  } else {
    nsCOMPtr<nsIRunnable> r = new ReadEntriesRunnable(&successCallback,
        errorCallbackPtr, mEntry);
    thread->Dispatch(r, NS_DISPATCH_NORMAL);
  }
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
