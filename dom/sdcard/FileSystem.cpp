/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FileSystem.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {
namespace sdcard {


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(FileSystem)
NS_IMPL_CYCLE_COLLECTING_ADDREF(FileSystem)
NS_IMPL_CYCLE_COLLECTING_RELEASE(FileSystem)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(FileSystem)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


FileSystem::FileSystem()
{
  SetIsDOMBinding();
}

FileSystem::~FileSystem()
{
}

JSObject*
FileSystem::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return FileSystemBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

void FileSystem::GetName(nsString& retval) const {
  retval.AssignLiteral("SD Card");
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
