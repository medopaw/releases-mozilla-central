/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SDCardRequestParent.h"

namespace mozilla {
namespace dom {
namespace sdcard {

NS_IMPL_THREADSAFE_ADDREF(SDCardRequestParent)
NS_IMPL_THREADSAFE_RELEASE(SDCardRequestParent)

SDCardRequestParent::SDCardRequestParent()
{
  MOZ_COUNT_CTOR(SDCardRequestParent);
}

SDCardRequestParent::~SDCardRequestParent()
{
  MOZ_COUNT_DTOR(SDCardRequestParent);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
