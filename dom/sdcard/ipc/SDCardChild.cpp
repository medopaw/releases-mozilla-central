/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "SDCardChild.h"
#include "../Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

bool
SDCardChild::RecvOnVoidResult()
{
  SDCARD_LOG("in SDCardChild.RecvOnVoidResult()");
  return false;
}

bool
SDCardChild::RecvOnError()
{
  SDCARD_LOG("in SDCardChild.RecvOnError()");
  return false;
}

SDCardChild::SDCardChild()
{
  SDCARD_LOG("construct SDCardChild");
  MOZ_COUNT_CTOR(SDCardChild);
}

SDCardChild::~SDCardChild()
{
  SDCARD_LOG("destruct SDCardChild");
  MOZ_COUNT_DTOR(SDCardChild);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
