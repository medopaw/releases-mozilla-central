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
// NS_IMPL_CYCLE_COLLECTION_INHERITED_0(DirectoryEntry, Entry)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DirectoryEntry)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DirectoryEntry)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DirectoryEntry)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

/*
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(DirectoryEntry)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
NS_INTERFACE_MAP_END_INHERITING(DirectoryEntry)

NS_IMPL_ADDREF_INHERITED(DirectoryEntry, Entry)
NS_IMPL_RELEASE_INHERITED(DirectoryEntry, Entry)
*/

  /*
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(DirectoryEntry, Entry)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(DirectoryEntry, Entry)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(DirectoryEntry)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(DirectoryEntry)
NS_INTERFACE_MAP_END_INHERITING(Entry)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DirectoryEntry)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DirectoryEntry)
*/

DirectoryEntry::DirectoryEntry(const nsAString& aFullPath) : Entry(aFullPath)
{
  SDCARD_LOG("init DirectoryEntry");
  // printf("\ninit DirectoryEntry\n");
//  mFullPath = aFullPath;
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
  // printf("\nin DirectoryEntry.IsFile()!!!\n");
  return false;
}

bool DirectoryEntry::IsDirectory() const
{
  SDCARD_LOG("in DirectoryEntry.IsDirectory()!!!");
  // printf("\nin DirectoryEntry.IsDirectory()!!!\n");
  return true;
}


} // namespace sdcard
} // namespace dom
} // namespace mozilla
