/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Entry.h"
// #include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"

#include "FileSystem.h"
#include "Metadata.h"
#include "FileSystemRunnable.h"
#include "Path.h"
#include "Utils.h"
#include "GetParentRunnable.h"
#include "CopyAndMoveToRunnable.h"

namespace mozilla {
namespace dom {
namespace sdcard {

NS_IMPL_ADDREF(Entry)
NS_IMPL_RELEASE(Entry)
NS_INTERFACE_MAP_BEGIN(Entry)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

/*
Entry::Entry(FileSystem* aFilesystem, const nsAString& aFullPath) : mFilesystem(aFilesystem), mFullPath(aFullPath)
{
  SDCARD_LOG("init Entry");
  NS_NewLocalFile(mFullPath, false, getter_AddRefs(mEntry));
}
*/

Entry::Entry(FileSystem* aFilesystem, nsIFile* aFile, bool aIsFile, bool aIsDirectory) : mFilesystem(aFilesystem), mIsFile(aIsFile), mIsDirectory(aIsDirectory)
{
  SDCARD_LOG("init Entry");
  // copy nsIFile object and hold it
  nsCOMPtr<nsIFile> file;
  aFile->Clone(getter_AddRefs(mFile));
  // NS_NewLocalFile(mFullPath, false, getter_AddRefs(mEntry));
  mMetadata = new Metadata;
}

Entry::~Entry()
{
}

/*
JSObject*
Entry::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return EntryBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}
*/

bool Entry::IsFile() const
{
  return mIsFile;
}

bool Entry::IsDirectory() const
{
  return mIsDirectory;
}

void Entry::GetMetadata(MetadataCallback& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback)
{
  SDCARD_LOG("in Entry.GetMetadata()");

  ErrorCallback* pErrorCallback = nullptr;
  if (errorCallback.WasPassed()) {
    pErrorCallback = errorCallback.Value().get();
  }
  nsIRunnable* r = new GetMetadataRunnable(&successCallback, pErrorCallback, this);
  mFilesystem->DispatchToWorkerThread(r, pErrorCallback);
}

void Entry::GetName(nsString& retval) const
{
  SDCARD_LOG("in Entry.GetName()");

  nsString name;
  mFile->GetLeafName(name);
  SDCARD_LOG("entry name=%s", NS_ConvertUTF16toUTF8(name).get());
  retval = name;
}

void Entry::GetFullPath(nsString& retval) const
{
  SDCARD_LOG("in Entry.GetFullPath()!!!!");

  nsString path, fullPath;
  mFile->GetPath(path);
  SDCARD_LOG("mFile Path=%s", NS_ConvertUTF16toUTF8(path).get());
  Path::RealPathToInnerPath(path, fullPath);
  SDCARD_LOG("entry fullPath=%s", NS_ConvertUTF16toUTF8(fullPath).get());
  retval = fullPath;
}

FileSystem* Entry::Filesystem() const
{
  SDCARD_LOG("in Entry.Filesystem()");
  nsRefPtr<FileSystem> fileSystem(mFilesystem);
  return fileSystem.forget().get();
}

void Entry::MoveTo(DirectoryEntry& parent, const Optional<nsAString >& newName,
    const Optional<OwningNonNull<EntryCallback> >& successCallback,
    const Optional<OwningNonNull<ErrorCallback> >& errorCallback)
{
  CopyAndMoveTo(parent, newName, successCallback, errorCallback, false);
}

void Entry::CopyTo(DirectoryEntry& parent, const Optional<nsAString >& newName,
    const Optional<OwningNonNull<EntryCallback> >& successCallback,
    const Optional<OwningNonNull<ErrorCallback> >& errorCallback)
{
  CopyAndMoveTo(parent, newName, successCallback, errorCallback, true);
}

void Entry::CopyAndMoveTo(DirectoryEntry& parent,
    const Optional<nsAString >& newName,
    const Optional<OwningNonNull<EntryCallback> >& successCallback,
    const Optional<OwningNonNull<ErrorCallback> >& errorCallback,
    bool isCopy)
{
  const nsAString* newNamePtr = nullptr;
  if (newName.WasPassed()) {
    newNamePtr = &newName.Value();
  }
  EntryCallback* successCallbackPtr = nullptr;
  if (successCallback.WasPassed()) {
    successCallbackPtr = successCallback.Value().get();
  }
  ErrorCallback* errorCallbackPtr = nullptr;
  if (errorCallback.WasPassed()) {
    errorCallbackPtr = errorCallback.Value().get();
  }

  nsIRunnable* r = new CopyAndMoveToRunnable(&parent, newNamePtr, successCallbackPtr,
      errorCallbackPtr, this, isCopy);
  mFilesystem->DispatchToWorkerThread(r, errorCallbackPtr);
}

void Entry::Remove(VoidCallback& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback)
{
  SDCARD_LOG("in Entry.Remove()");

  ErrorCallback* pErrorCallback = nullptr;
  if (errorCallback.WasPassed()) {
    pErrorCallback = errorCallback.Value().get();
  }
  nsIRunnable* r = new RemoveRunnable(&successCallback, pErrorCallback, this);
  mFilesystem->DispatchToWorkerThread(r, pErrorCallback);
}

void Entry::GetParent(EntryCallback& successCallback,
    const Optional<OwningNonNull<ErrorCallback> >& errorCallback)
{
  ErrorCallback* errorCallbackPtr = nullptr;
  if (errorCallback.WasPassed()) {
    errorCallbackPtr = errorCallback.Value().get();
  }
  nsIRunnable* r = new GetParentRunnable(&successCallback,
      errorCallbackPtr, this);
  mFilesystem->DispatchToWorkerThread(r, errorCallbackPtr);
}

bool Entry::Exists() const
{
  bool exists = false;
  mFile->Exists(&exists);
  return exists;
}

bool Entry::IsRoot() const
{
  nsString path;
  GetFullPath(path);
  return path == NS_LITERAL_STRING("/");
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
