/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Entry.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

/*
NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(Entry)
NS_IMPL_CYCLE_COLLECTING_ADDREF(Entry)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Entry)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Entry)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
*/

Entry::Entry()
{
//  SetIsDOMBinding();
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


} // namespace sdcard
} // namespace dom
} // namespace mozilla
