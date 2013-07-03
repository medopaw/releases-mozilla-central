/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SDCardRequestParent.h"
#include "IPCCopyAndMoveToEvent.h"
#include "IPCRemoveEvent.h"
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

    case SDCardParams::TSDCardCopyAndMoveParams: {
      SDCardCopyAndMoveParams p = mParams;
      SDCARD_LOG("%s %s to %s with newName=%s", p.isCopy() ? "Copy" : "Move", NS_ConvertUTF16toUTF8(p.relpath()).get(), NS_ConvertUTF16toUTF8(p.parentPath()).get(), NS_ConvertUTF16toUTF8(p.newName()).get());

      nsCOMPtr<IPCCopyAndMoveToEvent> r = new IPCCopyAndMoveToEvent(this, p.relpath(), p.parentPath(), p.newName(), p.isCopy());
      r->Start();

      break;
    }

    case SDCardParams::TSDCardGetParams: {
      SDCardGetParams p = mParams;
      SDCARD_LOG("Get %s %s with create=%d, exclusive=%d, isFile=%d", p.isFile() ? "file" : "directory", NS_ConvertUTF16toUTF8(p.relpath()).get(), p.create(), p.exclusive(), p.isFile());
      break;
    }

    case SDCardParams::TSDCardMetadataParams: {
      SDCardMetadataParams p = mParams;
      SDCARD_LOG("Get metadata of %s", NS_ConvertUTF16toUTF8(p.relpath()).get());
      break;
    }

    case SDCardParams::TSDCardParentParams: {
      SDCardMetadataParams p = mParams;
      SDCARD_LOG("Get parent of %s", NS_ConvertUTF16toUTF8(p.relpath()).get());
      break;
    }

    case SDCardParams::TSDCardGetAllParams: {
      SDCardGetAllParams p = mParams;
      SDCARD_LOG("Get direct children of %s", NS_ConvertUTF16toUTF8(p.relpath()).get());
      break;
    }

    case SDCardParams::TSDCardRemoveParams: {
      SDCardRemoveParams p = mParams;
      SDCARD_LOG("Remove %s with recursive=%d", NS_ConvertUTF16toUTF8(p.relpath()).get(), p.recursive());

      nsCOMPtr<IPCRemoveEvent> r = new IPCRemoveEvent(this, p.relpath(), p.recursive());
      r->Start();

      break;
    }

    default: {
      NS_RUNTIMEABORT("not reached");
      break;
    }

  }
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
