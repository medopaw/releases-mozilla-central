/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "mozilla/dom/sdcard/PSDCardChild.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class SDCardChild :
    public PSDCardChild
{
  virtual bool
  RecvOnVoidResult();

  virtual bool
  RecvOnError();

protected:
  SDCardChild();
  virtual ~SDCardChild();
};
} // namespace sdcard
} // namespace dom
} // namespace mozilla
