/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SDCardParent.h"

namespace mozilla {
namespace dom {
namespace sdcard {

bool
SDCardParent::RecvRemove(
        const nsString& path,
        const bool& recursive)
{
    return false;
}

SDCardParent::SDCardParent()
{
    MOZ_COUNT_CTOR(SDCardParent);
}

SDCardParent::~SDCardParent()
{
    MOZ_COUNT_DTOR(SDCardParent);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
