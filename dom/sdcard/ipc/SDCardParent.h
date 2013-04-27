/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once


#include "mozilla/dom/sdcard/PSDCardParent.h"

namespace mozilla {
namespace dom {

class ContentParent;

} // namespace dom
} // namespace mozilla

namespace mozilla {
namespace dom {
namespace sdcard {

class SDCardParent :
    public PSDCardParent
{
    friend class mozilla::dom::ContentParent;

    virtual bool
    RecvRemove(
            const nsString& path,
            const bool& recursive);

protected:
    SDCardParent();
    virtual ~SDCardParent();
};
} // namespace sdcard
} // namespace dom
} // namespace mozilla
