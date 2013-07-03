/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FileSystem.h"
#include "Metadata.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"
#include "Path.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

// NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(FileSystem)
NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(FileSystem, mRoot);

NS_IMPL_CYCLE_COLLECTING_ADDREF(FileSystem)
NS_IMPL_CYCLE_COLLECTING_RELEASE(FileSystem)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(FileSystem)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


FileSystem::FileSystem(nsIDOMNavigator* aNavigator, const nsAString& aName, const nsAString& aPath) : mNavigator(aNavigator), mName(aName)
{
  SDCARD_LOG("construct FileSystem");
  MOZ_ASSERT(aNavigator, "Parent navigator object should be provided");
  nsCOMPtr<nsIFile> rootDir;
  nsresult rv = NS_NewLocalFile(Path::base, false, getter_AddRefs(rootDir));
  if (NS_FAILED(rv)) {
    SDCARD_LOG("Fail to create root nsIFile.");
    mRoot = nullptr;
  } else {
    SDCARD_LOG("Create root nsIFile successfully.");
    mRoot = new DirectoryEntry(this, rootDir);
  }
  SetIsDOMBinding();
}

FileSystem::~FileSystem()
{
  SDCARD_LOG("destruct FileSystem");
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
}

JSObject*
FileSystem::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return FileSystemBinding::Wrap(aCx, aScope, this);
}

void FileSystem::GetName(nsString& retval) const
{
  SDCARD_LOG("in FileSystem.GetName()");
  SDCARD_LOG("mName=%s", NS_ConvertUTF16toUTF8(mName).get());
  retval = mName;
  SDCARD_LOG("retval=%s", NS_ConvertUTF16toUTF8(retval).get());
}


already_AddRefed<DirectoryEntry> FileSystem::Root()
{
    SDCARD_LOG("in FileSystem.Root()");
    nsRefPtr<DirectoryEntry> root(mRoot);
    return root.forget().get();
}

bool FileSystem::IsValid() const
{
    return mRoot && mRoot->Exists();
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
