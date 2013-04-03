/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsIDOMNavigator.h"
#include "DirectoryEntry.h"

struct JSContext;

namespace mozilla {
namespace dom {
namespace sdcard {

class FileSystem MOZ_FINAL : public nsISupports /* Change nativeOwnership in the binding configuration if you don't want this */,
                             public nsWrapperCache /* Change wrapperCache in the binding configuration if you don't want this */
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(FileSystem)

public:
  explicit FileSystem(nsIDOMNavigator* aNavigator, const nsAString& aName, const nsAString& aPath);

  ~FileSystem();

  // TODO: return something sensible here, and change the return type
  nsIDOMNavigator* GetParentObject() const
  {
    return mNavigator.get();
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);

  void GetName(nsString& retval) const;

  // Mark this as resultNotAddRefed to return raw pointers
  already_AddRefed<DirectoryEntry> Root();
/*  {
    printf("\nin FileSystem.Root()\n");
    if (!mRoot) {
      mRoot = nullptr;
      mRoot = new DirectoryEntry();
    }
    NS_IF_ADDREF(mRoot);
    return mRoot.get();

    // DirectoryEntry* root = mRoot;
    // NS_IF_ADDREF(root);
    // return root;

    // nsCOMPtr<DirectoryEntry> root(do_QueryInterface(mRoot));
    // return root.forget();
  }
*/

  bool IsValid() const;

private:
  nsCOMPtr<nsIDOMNavigator> mNavigator;
  nsString mName;
  // DirectoryEntry* mRoot;
  nsRefPtr<DirectoryEntry> mRoot;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
