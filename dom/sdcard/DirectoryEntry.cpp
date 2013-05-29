/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DirectoryEntry.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"

#include "DirectoryReader.h"
#include "FileSystem.h"
#include "GetEntryRunnable.h"
#include "RemoveRunnable.h"
#include "Error.h"
#include "Path.h"
#include "Utils.h"

#include "SPGetEntryEvent.h"
#include "SPRemoveEvent.h"
#include "mozilla/dom/ContentChild.h"
#include "SDCardRequestChild.h"

namespace mozilla {
namespace dom {
namespace sdcard {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(DirectoryEntry)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DirectoryEntry)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DirectoryEntry)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DirectoryEntry)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

DirectoryEntry::DirectoryEntry(FileSystem* aFilesystem, nsIFile* aFile) : Entry(aFilesystem, aFile, false, true)
{
  SDCARD_LOG("construct DirectoryEntry");
  SetIsDOMBinding();
}

DirectoryEntry::~DirectoryEntry()
{
  SDCARD_LOG("destruct DirectoryEntry");
}

JSObject*
DirectoryEntry::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return DirectoryEntryBinding::Wrap(aCx, aScope, this);
}

already_AddRefed<DirectoryReader> DirectoryEntry::CreateReader()
{
  SDCARD_LOG("in DirectoryEntry.CreateReader()");
  nsRefPtr<DirectoryReader> reader = new DirectoryReader(this);
  return reader.forget();
}

void DirectoryEntry::GetFile(const nsAString& path, const FileSystemFlags& options, const Optional< OwningNonNull<EntryCallback> >& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback)
{
  SDCARD_LOG("in DirectoryEntry.GetFile()");
  GetEntry(path, options, successCallback, errorCallback, true);
}

void DirectoryEntry::GetDirectory(const nsAString& path, const FileSystemFlags& options, const Optional< OwningNonNull<EntryCallback> >& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback)
{
  SDCARD_LOG("in DirectoryEntry.GetDirectory()");
  GetEntry(path, options, successCallback, errorCallback, false);
}

void DirectoryEntry::RemoveRecursively(VoidCallback& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback)
{
  SDCARD_LOG("in DirectoryEntry.RemoveRecursively()");

  ErrorCallback* pErrorCallback = nullptr;
  if (errorCallback.WasPassed()) {
    pErrorCallback = errorCallback.Value().get();
  }

  nsRefPtr<Caller> pCaller = new Caller(mFilesystem, &successCallback, pErrorCallback);
  nsString path;
  mFile->GetPath(path);
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    SDCARD_LOG("in b2g process");
    nsRefPtr<SPRemoveEvent> r = new SPRemoveEvent(pCaller, path, true);
    r->Start();
  } else {
    SDCARD_LOG("in app process");
    SDCardRemoveParams params(path, true);
    PSDCardRequestChild* child = new SDCardRequestChild(pCaller);
    ContentChild::GetSingleton()->SendPSDCardRequestConstructor(child, params);
  }

}

void DirectoryEntry::GetEntry(const nsAString& path, const FileSystemFlags& options, const Optional< OwningNonNull<EntryCallback> >& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback, bool isFile)
{
  SDCARD_LOG("in DirectoryEntry.GetEntry()");

  // Assign callback nullptr if not passed
  EntryCallback* pSuccessCallback = nullptr;
  ErrorCallback* pErrorCallback = nullptr;
  if (successCallback.WasPassed()) {
    pSuccessCallback = successCallback.Value().get();
  }
  if (errorCallback.WasPassed()) {
    pErrorCallback = errorCallback.Value().get();
  }

  // Check if path is valid.
  if (!Path::IsValidPath(path)) {
    SDCARD_LOG("Invalid path!");
    Error::HandleError(pErrorCallback, Error::DOM_ERROR_ENCODING);
    return;
  }

  // Make sure path is absolute.
  nsString fullPath;
  GetFullPath(fullPath);
  nsString absolutePath;
  Path::Absolutize(path, fullPath, absolutePath);
  nsString realPath;
  Path::DOMPathToRealPath(absolutePath, realPath);

  nsRefPtr<Caller> pCaller = new Caller(mFilesystem, pSuccessCallback, pErrorCallback);
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    SDCARD_LOG("in b2g process");
    nsRefPtr<SPGetEntryEvent> r = new SPGetEntryEvent(pCaller, realPath, options.mCreate, options.mExclusive, isFile);
    r->Start();
  } else {
    SDCARD_LOG("in app process");
    SDCardGetParams params(realPath, options.mCreate, options.mExclusive, isFile);
    PSDCardRequestChild* child = new SDCardRequestChild(pCaller);
    ContentChild::GetSingleton()->SendPSDCardRequestConstructor(child, params);
  }
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
