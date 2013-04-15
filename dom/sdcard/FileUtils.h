/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "nsIFile.h"

namespace mozilla {
namespace dom {
namespace sdcard {

/*
 * nsIFile related utilities.
 * All methods in this class are static.
 */
class FileUtils
{
public:
  static unsigned long GetType(bool isFile);
  static nsresult IsDirectoryEmpty(nsIFile* dir, bool* retval);
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
