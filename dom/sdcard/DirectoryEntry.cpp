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
#include "ErrorRunnable.h"
#include "Path.h"
#include "Utils.h"

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
  SDCARD_LOG("init DirectoryEntry");
  SetIsDOMBinding();
}

DirectoryEntry::~DirectoryEntry()
{
}

JSObject*
DirectoryEntry::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return DirectoryEntryBinding::Wrap(aCx, aScope, this, aTriedToWrap);
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

    // if not passed, assign callback nullptr
    EntryCallback* pSuccessCallback = nullptr;
    ErrorCallback* pErrorCallback = nullptr;
    if (successCallback.WasPassed()) {
      pSuccessCallback = successCallback.Value().get();
    }
    if (errorCallback.WasPassed()) {
      pErrorCallback = errorCallback.Value().get();
    }

    // check if path is valid
    if (!Path::IsValidPath(path)) {
      nsRefPtr<ErrorRunnable> r = new ErrorRunnable(pErrorCallback, DOM_ERROR_ENCODING);
      r->Start();
    }

    // turn relative path to absolute
    nsString fullPath;
    GetFullPath(fullPath);
    nsString absolutePath;
    Path::Absolutize(path, fullPath, absolutePath);
    nsString realPath;
    Path::InnerPathToRealPath(absolutePath, realPath);

    nsRefPtr<GetEntryRunnable> runnable = new GetEntryRunnable(realPath, options.mCreate, options.mExclusive, pSuccessCallback, pErrorCallback, this, nsIFile::NORMAL_FILE_TYPE);
    runnable->Start();
}

void DirectoryEntry::GetDirectory(const nsAString& path, const FileSystemFlags& options, const Optional< OwningNonNull<EntryCallback> >& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback)
{
    SDCARD_LOG("in DirectoryEntry.GetDirectory()");

    // if not passed, assign callback nullptr
    EntryCallback* pSuccessCallback = nullptr;
    ErrorCallback* pErrorCallback = nullptr;
    if (successCallback.WasPassed()) {
      pSuccessCallback = successCallback.Value().get();
    }
    if (errorCallback.WasPassed()) {
      pErrorCallback = errorCallback.Value().get();
    }

    // check if path is valid
    if (!Path::IsValidPath(path)) {
      nsRefPtr<ErrorRunnable> r = new ErrorRunnable(pErrorCallback, DOM_ERROR_ENCODING);
      r->Start();
      return;
    }

    // turn relative path to absolute
    nsString fullPath;
    GetFullPath(fullPath);
    nsString absolutePath;
    Path::Absolutize(path, fullPath, absolutePath);
    nsString realPath;
    Path::InnerPathToRealPath(absolutePath, realPath);

    nsRefPtr<GetEntryRunnable> runnable = new GetEntryRunnable(realPath, options.mCreate, options.mExclusive, pSuccessCallback, pErrorCallback, this, nsIFile::DIRECTORY_TYPE);
    runnable->Start();
}

void DirectoryEntry::RemoveRecursively(VoidCallback& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback)
{
  SDCARD_LOG("in DirectoryEntry.RemoveRecursively()");

  ErrorCallback* pErrorCallback = nullptr;
  if (errorCallback.WasPassed()) {
    pErrorCallback = errorCallback.Value().get();
  }
  nsRefPtr<RemoveRunnable> runnable = new RemoveRunnable(&successCallback,
      pErrorCallback, this, true);
  runnable->Start();
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
