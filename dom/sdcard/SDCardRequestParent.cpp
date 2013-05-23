/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SDCardRequestParent.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

NS_IMPL_THREADSAFE_ADDREF(SDCardRequestParent)
NS_IMPL_THREADSAFE_RELEASE(SDCardRequestParent)

SDCardRequestParent::SDCardRequestParent(const SDCardParams& aParams)
  : mParams(aParams)
{
  SDCARD_LOG("construct SDCardRequestParent");
  MOZ_COUNT_CTOR(SDCardRequestParent);
}

SDCardRequestParent::~SDCardRequestParent()
{
  SDCARD_LOG("destruct SDCardRequestParent");
  MOZ_COUNT_DTOR(SDCardRequestParent);
}

void
SDCardRequestParent::Dispatch()
{
  SDCARD_LOG("in SDCardRequestParent.Dispatch()");

  switch(mParams.type()) {

    case SDCardParams::TSDCardCopyAndMoveParams:
    {
    }

    case SDCardParams::TSDCardGetParams:
    {
    }

    case SDCardParams::TSDCardMetadataParams:
    {
    }

    case SDCardParams::TSDCardGetAllParams:
    {
    }

    case SDCardParams::TSDCardRemoveParams:
    {
      SDCardRemoveParams p = mParams;
      SDCARD_LOG("Remove %s with recursive=%d", NS_ConvertUTF16toUTF8(p.relpath()).get(), p.recursive());
      break;
    }

    default:
    {
      NS_RUNTIMEABORT("not reached");
      break;
    }

  }
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
