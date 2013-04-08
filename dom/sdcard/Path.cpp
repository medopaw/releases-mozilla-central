/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
#include "Metadata.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"
*/

#include "Path.h"

namespace mozilla {
namespace dom {
namespace sdcard {

nsString Path::separator = NS_LITERAL_STRING("/");
nsString Path::root = Path::separator;
nsString Path::base = Path::root;

void Path::RealPathToInnerPath(const nsAString& aRealPath, nsString& aInnerPath)
{
  MOZ_ASSERT(Path::IsParentOf(Path::base, aRealPath) || Path::base == aRealPath, "Path must be within the scope of FileSystem!");
  // special case for root
  if (aRealPath.Equals(Path::base)) {
    aInnerPath = Path::root;
  } else {
    aInnerPath = aRealPath;
    Path::Decapitate(aInnerPath, Path::base);
  }
}

void Path::InnerPathToRealPath(const nsAString& aInnerPath, nsString& aRealPath)
{
  MOZ_ASSERT(Path::IsAbsolute(aInnerPath), "Path must be absolute!");
  // special case for root
  if (aInnerPath.Equals(Path::root)) {
    aRealPath = Path::base;
  } else {
    aRealPath = Path::base + aInnerPath;
  }
}

bool Path::StartsWith(const nsAString& aPath, const nsAString& aMayBeHead)
{
  return aMayBeHead.Equals(Substring(aPath, 0, aMayBeHead.Length()));
}

bool Path::IsAbsolute(const nsAString& aPath)
{
  return Path::StartsWith(aPath, Path::root);
}

bool Path::IsParentOf(const nsAString& aParent, const nsAString& aMayBeChild)
{
  MOZ_ASSERT(Path::IsAbsolute(aParent) && Path::IsAbsolute(aMayBeChild), "Path must be absolute!");
  // check length
  if (aParent.Length() >= aMayBeChild.Length() || !Path::StartsWith(aParent, aMayBeChild)) {
    return false;
  }
  // check separator
  if (!Substring(aMayBeChild, aParent.Length(), 1).Equals(Path::separator)) {
    return false;
  }
  return true;
}

void Path::Decapitate(nsString& aPath, const nsAString& aHead)
{
  MOZ_ASSERT(Path::StartsWith(aPath, aHead), "aPath must starts with aHead");
  aPath = Substring(aPath, aHead.Length(), aPath.Length() - aHead.Length());
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
