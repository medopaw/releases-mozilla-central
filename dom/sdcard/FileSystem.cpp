/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FileSystem.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"

// #include "DirectoryEntry.h"

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


FileSystem::FileSystem(nsIDOMNavigator* aNavigator, const nsAString& aName, const nsAString& aPath) : mNavigator(aNavigator), mName(aName), mRoot(new DirectoryEntry(aPath))
{
  MOZ_ASSERT(aNavigator, "Parent navigator object should be provided");
 //  mRoot = new DirectoryEntry(/*aPath*/);
//  mRoot = nullptr;
  SetIsDOMBinding();
}

FileSystem::~FileSystem()
{
  mRoot = nullptr;
}

JSObject*
FileSystem::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return FileSystemBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

already_AddRefed<DirectoryEntry> FileSystem::Root()
{
    printf("\nin FileSystem.Root()\n");
/*
    if (!mRoot) {
      mRoot = nullptr;
      mRoot = new DirectoryEntry();
    }
    */
    NS_IF_ADDREF(mRoot);
    return mRoot.get();

    // DirectoryEntry* root = mRoot;
    // NS_IF_ADDREF(root);
    // return root;

    // nsCOMPtr<DirectoryEntry> root(do_QueryInterface(mRoot));
    // return root.forget();
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
