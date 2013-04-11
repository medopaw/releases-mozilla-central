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

const nsString Path::separator = NS_LITERAL_STRING("/");
const nsString Path::selfReference = NS_LITERAL_STRING(".");
const nsString Path::parentReference = NS_LITERAL_STRING("..");
const nsString Path::root = Path::separator;
const nsString Path::base = Path::root + NS_LITERAL_STRING("sdcard");

bool Path::IsRoot(const nsAString& aPath)
{
  MOZ_ASSERT(Path::IsAbsolute(aPath), "Path must be absolute!");
  return aPath == Path::root;
}

bool Path::IsBase(const nsAString& aPath)
{
  MOZ_ASSERT(Path::IsAbsolute(aPath), "Path must be absolute!");
  return aPath == Path::base;
}

bool Path::WithinBase(const nsAString& aPath)
{
  MOZ_ASSERT(Path::IsAbsolute(aPath), "Path must be absolute!");
  return Path::IsParentOf(Path::base, aPath) || Path::IsBase(aPath);
}

void Path::RealPathToInnerPath(const nsAString& aRealPath, nsString& aInnerPath)
{
  MOZ_ASSERT(Path::WithinBase(aRealPath), "Path must be within the scope of FileSystem!");
  // special case for root
  if (Path::IsBase(aRealPath)) {
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
  if (Path::IsRoot(aInnerPath)) {
    aRealPath = Path::base;
  } else {
    aRealPath = Path::base + aInnerPath;
  }
}

bool Path::BeginsWithSeparator(const nsAString& aPath)
{
  return StringBeginsWith(aPath, Path::separator);
}

bool Path::EndsWithSeparator(const nsAString& aPath)
{
  return StringEndsWith(aPath, Path::separator);
}

bool Path::IsAbsolute(const nsAString& aPath)
{
  return StringBeginsWith(aPath, Path::root);
}

bool Path::IsValidName(const nsAString& aName)
{
  if (FindInReadable(Path::separator, aName)) {
    return false;
  }
  if (aName == Path::selfReference || aName == Path::parentReference) {
    return false;
  }
  return Path::IsValidPath(aName);
}

bool Path::IsValidPath(const nsAString& aPath)
{
  if (aPath.IsEmpty() || Path::IsRoot(aPath)) {
    return true;
  }
  if (FindInReadable(NS_LITERAL_STRING("\0"), aPath)) {
    return false;
  }
  if (FindInReadable(NS_LITERAL_STRING("\\"), aPath)) {
    return false;
  }
  return true;
}

bool Path::IsParentOf(const nsAString& aParent, const nsAString& aMayBeChild)
{
  MOZ_ASSERT(Path::IsAbsolute(aParent) && Path::IsAbsolute(aMayBeChild), "Path must be absolute!");
  // check length
  if (aParent.Length() >= aMayBeChild.Length() || !StringBeginsWith(aParent, aMayBeChild)) {
    return false;
  }
  // check separator
  if (Substring(aMayBeChild, aParent.Length(), 1) != Path::separator) {
    return false;
  }
  return true;
}

void Path::Split(const nsAString& aPath, nsTArray<nsString>& aArray)
{
  MOZ_ASSERT(Path::separator.Length() == 1, "Path separator must be a single character!");

  // nsCString conversion needed here
  nsTArray<nsCString> array;
  ParseString(NS_ConvertUTF16toUTF8(aPath), NS_ConvertUTF16toUTF8(Path::separator)[0], array);

  // no need to clear aArray first
  for (PRUint32 i = 0; i < array.Length(); i++) {
    aArray.AppendElement(NS_ConvertUTF8toUTF16(array[i]));
  }
}

void Path::Decapitate(nsString& aPath, const nsAString& aHead)
{
  MOZ_ASSERT(StringBeginsWith(aPath, aHead), "aPath must starts with aHead");
  aPath = Substring(aPath, aHead.Length(), aPath.Length() - aHead.Length());
}

void Path::EnsureDirectory(nsString& aPath)
{
  if (!Path::EndsWithSeparator(aPath)) {
    aPath += Path::separator;
  }
}

void Path::Append(nsString& aPath, const nsAString& aToAppend)
{
  MOZ_ASSERT(!Path::IsAbsolute(aToAppend), "Path must be relative!");
  Path::EnsureDirectory(aPath);
  aPath += aToAppend;
}

void Path::Append(const nsAString& aParent, const nsAString& aToAppend, nsString& retval)
{
  retval = aParent;
  Path::Append(retval, aToAppend);
}

void Path::Absolutize(nsString& aPath, const nsAString& aParent)
{
  nsString absolutePath;
  Path::Absolutize(aPath, aParent, absolutePath);
  aPath = absolutePath;
}

void Path::Absolutize(const nsAString& aPath, const nsAString& aParent, nsString& retval)
{
  MOZ_ASSERT(Path::IsAbsolute(aParent), "Path must be absolute!");
  if (!Path::IsAbsolute(aPath)) {
    Path::Append(aParent, aPath, retval);
  } else {
    retval = aPath;
  }
  Path::RemoveExtraParentReferences(retval);
}

void Path::RemoveExtraParentReferences(nsString& aPath)
{
  MOZ_ASSERT(Path::IsAbsolute(aPath), "Path must be absolute!");

  nsTArray<nsString> components;
  nsTArray<nsString> canonicalized;

  Path::Split(aPath, components);

  for (PRUint32 i = 0; i < components.Length(); i++) {
    if (components[i] == Path::selfReference) {
      continue;
    }
    if (components[i] == Path::parentReference) {
      if (!canonicalized.IsEmpty()) {
        components.RemoveElementAt(components.Length() - 1);
      }
      continue;
    }
    canonicalized.AppendElement(components[i]);
  }

  if (canonicalized.IsEmpty()) {
    aPath = Path::root;
  }

  nsString result;
  for (PRUint32 i = 0; i < canonicalized.Length(); i++) {
    result += Path::separator;
    result += canonicalized[i];
  }
  if (Path::EndsWithSeparator(aPath)) {
    Path::EnsureDirectory(result);
  }
  aPath = result;
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
