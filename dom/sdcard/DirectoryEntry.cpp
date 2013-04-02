/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DirectoryEntry.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"

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

DirectoryEntry::DirectoryEntry(FileSystem* aFilesystem, nsIFile* aFile) : Entry(aFilesystem, aFile)
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

bool DirectoryEntry::IsFile() const
{
  SDCARD_LOG("in DirectoryEntry.IsFile()!!!");
  return false;
}

bool DirectoryEntry::IsDirectory() const
{
  SDCARD_LOG("in DirectoryEntry.IsDirectory()!!!");
  return true;
}


} // namespace sdcard
} // namespace dom
} // namespace mozilla
