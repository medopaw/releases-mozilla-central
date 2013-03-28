/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Entry.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"

#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

NS_IMPL_ADDREF(Entry)
NS_IMPL_RELEASE(Entry)
NS_INTERFACE_MAP_BEGIN(Entry)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

Entry::Entry(const nsAString& aFullPath) : mFullPath(aFullPath)
{
  SDCARD_LOG("init Entry");
}

Entry::~Entry()
{
}

void Entry::GetFullPath(nsString& retval) const
{
  retval = mFullPath;
  SDCARD_LOG("in Entry.GetFullPath()!!!!");
  SDCARD_LOG("mFullPath=%s", NS_ConvertUTF16toUTF8(mFullPath).get());
  SDCARD_LOG("retval=%s", NS_ConvertUTF16toUTF8(retval).get());
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
