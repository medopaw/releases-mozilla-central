/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DirectoryReader.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"
#include "Window.h"
#include "ReadEntriesRunnable.h"
#include "DirectoryEntry.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(DirectoryReader, mEntry)
NS_IMPL_CYCLE_COLLECTING_ADDREF(DirectoryReader)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DirectoryReader)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DirectoryReader)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

DirectoryReader::DirectoryReader(DirectoryEntry* entry) :
    mEntry(entry)
{
  SDCARD_LOG("construct DirectoryReader");
  SetIsDOMBinding();
}

DirectoryReader::~DirectoryReader()
{
  SDCARD_LOG("destruct DirectoryReader");
}

nsPIDOMWindow*
DirectoryReader::GetParentObject() const
{
  return Window::GetWindow();
}

JSObject*
DirectoryReader::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return DirectoryReaderBinding::Wrap(aCx, aScope, this);
}

void DirectoryReader::ReadEntries(EntriesCallback& successCallback,
    const Optional<OwningNonNull<ErrorCallback> >& errorCallback)
{
  ErrorCallback* errorCallbackPtr = nullptr;
  if (errorCallback.WasPassed()) {
    errorCallbackPtr = errorCallback.Value().get();
  }
  nsCOMPtr<ReadEntriesRunnable> runnable = new ReadEntriesRunnable(&successCallback,
          errorCallbackPtr, mEntry);
  runnable->Start();
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
