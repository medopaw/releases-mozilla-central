/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Entry.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"

#include "FileSystem.h"
#include "Path.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

NS_IMPL_ADDREF(Entry)
NS_IMPL_RELEASE(Entry)
NS_INTERFACE_MAP_BEGIN(Entry)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

/*
Entry::Entry(FileSystem* aFilesystem, const nsAString& aFullPath) : mFilesystem(aFilesystem), mFullPath(aFullPath)
{
  SDCARD_LOG("init Entry");
  NS_NewLocalFile(mFullPath, false, getter_AddRefs(mEntry));
}
*/

Entry::Entry(FileSystem* aFilesystem, nsIFile* aFile) : mFilesystem(aFilesystem)
{
  SDCARD_LOG("init Entry");
  // copy nsIFile object and hold it
  nsCOMPtr<nsIFile> file;
  aFile->Clone(getter_AddRefs(mFile));
  // NS_NewLocalFile(mFullPath, false, getter_AddRefs(mEntry));
}

Entry::~Entry()
{
}

void Entry::GetName(nsString& retval) const
{
  SDCARD_LOG("in Entry.GetName()");
  nsString name;
  mFile->GetLeafName(name);
  SDCARD_LOG("entry name=%s", NS_ConvertUTF16toUTF8(name).get());
  retval = name;
}

void Entry::GetFullPath(nsString& retval) const
{
  /*
  retval = mFullPath;
  // call the conversion function
  SDCARD_LOG("in Entry.GetFullPath()!!!!");
  SDCARD_LOG("mFullPath=%s", NS_ConvertUTF16toUTF8(mFullPath).get());
  SDCARD_LOG("retval=%s", NS_ConvertUTF16toUTF8(retval).get());
  */
  SDCARD_LOG("in Entry.GetFullPath()!!!!");
  nsString path, fullPath;
  mFile->GetPath(path);
  SDCARD_LOG("mFile Path=%s", NS_ConvertUTF16toUTF8(path).get());
  Path::RealPathToInnerPath(path, fullPath);
  SDCARD_LOG("entry fullPath=%s", NS_ConvertUTF16toUTF8(fullPath).get());
  retval = fullPath;
}

/*
already_AddRefed<FileSystem> Entry::Filesystem() const
{
  SDCARD_LOG("in Entry.Filesystem()");
  nsRefPtr<FileSystem> filesystem(mFilesystem);
  NS_IF_ADDREF(filesystem);
  return filesystem.get();
}
*/

FileSystem* Entry::Filesystem() const
{
  SDCARD_LOG("in Entry.Filesystem()");
  return mFilesystem;
}

bool Entry::Exists() const
{
  bool exists = false;
  mFile->Exists(&exists);
  return exists;
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
