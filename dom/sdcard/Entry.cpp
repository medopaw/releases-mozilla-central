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
#include "Path.h"
#include "Utils.h"
#include "GetParentRunnable.h"
#include "CopyAndMoveToRunnable.h"
#include "GetMetadataRunnable.h"
#include "RemoveRunnable.h"
#include "FileEntry.h"
#include "DirectoryEntry.h"

#include "mozilla/dom/ContentChild.h"
#include "SDCardRequestChild.h"

namespace mozilla {
namespace dom {
namespace sdcard {

NS_IMPL_ADDREF(Entry)
NS_IMPL_RELEASE(Entry)
NS_INTERFACE_MAP_BEGIN(Entry)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

Entry* Entry::CreateFromFile(FileSystem* aFilesystem, nsIFile* aFile)
{
  MOZ_ASSERT(aFile, "Entry::CreateFromFile creation failed. aFile can't be null.");

  bool isFile;
  aFile->IsFile(&isFile);
  bool isDirectory;
  aFile->IsDirectory(&isDirectory);
  if (isFile) {
    return new FileEntry(aFilesystem, aFile);
  } else {
    return new DirectoryEntry(aFilesystem, aFile);
  }
  return nullptr;
}

Entry::Entry(FileSystem* aFilesystem, nsIFile* aFile, bool aIsFile, bool aIsDirectory) : mFilesystem(aFilesystem), mIsFile(aIsFile), mIsDirectory(aIsDirectory)
{
  SDCARD_LOG("construct Entry");

  // copy nsIFile object and hold it
  nsCOMPtr<nsIFile> file;
  aFile->Clone(getter_AddRefs(mFile));
  // NS_NewLocalFile(mFullPath, false, getter_AddRefs(mEntry));
}

Entry::~Entry()
{
  SDCARD_LOG("destruct Entry");
}

/*
JSObject*
Entry::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return EntryBinding::Wrap(aCx, aScope, this);
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
  nsRefPtr<GetMetadataRunnable> runnable = new GetMetadataRunnable(&successCallback, pErrorCallback, this);
  runnable->Start();
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
  Path::RealPathToDOMPath(path, fullPath);
  SDCARD_LOG("entry fullPath=%s", NS_ConvertUTF16toUTF8(fullPath).get());
  retval = fullPath;
}

already_AddRefed<FileSystem> Entry::Filesystem() const
{
  SDCARD_LOG("in Entry.Filesystem()");
  nsRefPtr<FileSystem> fileSystem(mFilesystem);
  return fileSystem.forget();
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

  nsRefPtr<CopyAndMoveToRunnable> runnable = new CopyAndMoveToRunnable(&parent,
      newNamePtr, successCallbackPtr,
      errorCallbackPtr, this, isCopy);
  runnable->Start();
}

void Entry::Remove(VoidCallback& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback)
{
  SDCARD_LOG("in Entry.Remove()");

  ErrorCallback* pErrorCallback = nullptr;
  if (errorCallback.WasPassed()) {
    pErrorCallback = errorCallback.Value().get();
  }

  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    SDCARD_LOG("in b2g process");
    nsRefPtr<RemoveRunnable> runnable = new RemoveRunnable(&successCallback, pErrorCallback, this);
    runnable->Start();
  } else {
    SDCARD_LOG("in app process");
    // PSDCardRequestChild* child = new SDCardRequestChild();
    // ContentChild::GetSingleton()->SendPSDCardRequestConstructor(child);
  }

}

void Entry::GetParent(EntryCallback& successCallback,
    const Optional<OwningNonNull<ErrorCallback> >& errorCallback)
{
  ErrorCallback* errorCallbackPtr = nullptr;
  if (errorCallback.WasPassed()) {
    errorCallbackPtr = errorCallback.Value().get();
  }
  nsRefPtr<GetParentRunnable> runnable = new GetParentRunnable(&successCallback,
      errorCallbackPtr, this);
  runnable->Start();
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
