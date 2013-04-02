/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FileEntry.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"

#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(FileEntry)

NS_IMPL_CYCLE_COLLECTING_ADDREF(FileEntry)
NS_IMPL_CYCLE_COLLECTING_RELEASE(FileEntry)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(FileEntry)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

FileEntry::FileEntry(FileSystem* aFilesystem, nsIFile* aFile) : Entry(aFilesystem, aFile)
{
  SDCARD_LOG("init FileEntryn");
  SetIsDOMBinding();
}

FileEntry::~FileEntry()
{
}

JSObject*
FileEntry::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return FileEntryBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

bool FileEntry::IsFile() const
{
  SDCARD_LOG("in FileEntry.IsFile()!!!");
  return true;
}

bool FileEntry::IsDirectory() const
{
  SDCARD_LOG("in FileEntry.IsDirectory()!!!");
  return false;
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
