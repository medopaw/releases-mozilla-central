/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FileUtils.h"
#include "nsISimpleEnumerator.h"

namespace mozilla {
namespace dom {
namespace sdcard {

unsigned long FileUtils::GetType(bool isFile)
{
  return isFile ? nsIFile::NORMAL_FILE_TYPE : nsIFile::DIRECTORY_TYPE;
}

nsresult FileUtils::IsDirectoryEmpty(nsIFile* dir, bool* retval)
{
  nsCOMPtr<nsISimpleEnumerator> childEnumerator;
  nsresult rv = dir->GetDirectoryEntries(getter_AddRefs(childEnumerator));
  if (NS_SUCCEEDED(rv)) {
    bool hasElements;
    rv = childEnumerator->HasMoreElements(&hasElements);
    *retval = !hasElements;
  }
  return rv;
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
