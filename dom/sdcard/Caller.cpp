/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Caller.h"
#include "Entry.h"
#include "mozilla/dom/DOMError.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

NS_IMPL_ADDREF(Caller)
NS_IMPL_RELEASE(Caller)

Caller::Caller(FileSystem* aFileSystem, CallbackFunction* aSuccessCallback, ErrorCallback* aErrorCallback) :
    mFilesystem(aFileSystem),
    mSuccessCallback(aSuccessCallback),
    mErrorCallback(aErrorCallback)
{
  SDCARD_LOG("construct Caller");
}

Caller::~Caller()
{
  SDCARD_LOG("destruct Caller");
}

void Caller::CallErrorCallback(const nsAString& error)
{
  SDCARD_LOG("Call ErrorCallback with error=%s", NS_ConvertUTF16toUTF8(error).get());

  if (mErrorCallback) {
    nsRefPtr<nsIDOMDOMError> domError = DOMError::CreateWithName(error);
    ErrorResult rv;
    mErrorCallback->Call(domError, rv);
  }
}

void Caller::CallEntryCallback(const nsAString& path)
{
  // create an Entry from path
  if (mSuccessCallback) {
    ErrorResult rv;
    static_cast<EntryCallback*>(mSuccessCallback.get())->Call(*(Entry::CreateFromPath(mFilesystem, path)), rv);
  }
}

void Caller::CallEntriesCallback(const InfallibleTArray<nsString>& paths)
{

}

void Caller::CallMetadataCallback(int64_t modificationTime, uint64_t size)
{

}

void Caller::CallVoidCallback()
{
  SDCARD_LOG("Call VoidCallback");

  if (mSuccessCallback) {
    ErrorResult rv;
    static_cast<VoidCallback*>(mSuccessCallback.get())->Call(rv);
  }
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
