/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Metadata.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"
#define __STDC_FORMAT_MACROS
#include "inttypes.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(Metadata)
NS_IMPL_CYCLE_COLLECTING_ADDREF(Metadata)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Metadata)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Metadata)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

Metadata::Metadata() :
    mSize(0),
    mDate(0)
{
  SDCARD_LOG("init Metadata");
  SetIsDOMBinding();
}

Metadata::~Metadata()
{
}

JSObject*
Metadata::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return MetadataBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

JS::Value Metadata::ModificationTime(JSContext* cx) const
{
  JSObject* date = JS_NewDateObjectMsec(cx, mDate);
  JS::Value value;
  value.setObject(*date);
  return value;
}

uint64_t Metadata::Size() const
{
  SDCARD_LOG("in Metadata.Size()");
  SDCARD_LOG("size=%" PRIu64, mSize);
  return mSize;
}

void Metadata::SetModificationTime(PRTime mtime)
{
  mDate = static_cast<double>(mtime);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
