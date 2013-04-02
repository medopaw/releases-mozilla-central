/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

/*
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
*/
#include "nsString.h"

namespace mozilla {
namespace dom {
namespace sdcard {

// FileSystem path utilities. All methods in this class are static.
class Path {
public:
    static nsString separator;
    static nsString root; // different from FileSystem.root, normally "/"
    static nsString base; // the real path of FileSystem.root, normally "/sdcard"

    static void RealPathToInnerPath(const nsAString& aRealPath, nsString& aInnerPath);
    static void InnerPathToRealPath(const nsAString& aInnerPath, nsString& aRealPath);

    static bool StartsWith(const nsAString& aPath, const nsAString& aMaybeHead);
    static bool IsAbsolute(const nsAString& aPath);
    static bool IsParentOf(const nsAString& aPath, const nsAString& aHead);
    static void Decapitate(nsString& aPath, const nsAString& aHead);
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
