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

#include "Entry.h"
#include "Utils.h"

struct JSContext;

namespace mozilla {
namespace dom {
namespace sdcard {

class DirectoryEntry MOZ_FINAL : public Entry//,
                                /*public nsISupports  Change nativeOwnership in the binding configuration if you don't want this */,
                                public nsWrapperCache /* Change wrapperCache in the binding configuration if you don't want this*/
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DirectoryEntry)

//  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
//  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(DirectoryEntry, Entry)

//  NS_DECL_ISUPPORTS_INHERITED
/* NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DirectoryEntry, Entry)
*/
public:
  explicit DirectoryEntry(const nsAString& aFullPath);
 // DirectoryEntry(const nsAString& aFullPath);

  ~DirectoryEntry();

  // TODO: return something sensible here, and change the return type
  /*DirectoryEntry* GetParentObject() const
  {
    return NULL;
  }
  */

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);

  bool IsFile() const MOZ_OVERRIDE;

  bool IsDirectory() const MOZ_OVERRIDE;

  /*
  void GetFullPath(nsString& retval) const
  {
    printf("\nin DirectoryEntry.GetFullPath()!!!\n");
    // Entry::GetFullPath(retval);
    printf("%s\n", NS_ConvertUTF16toUTF8(mFullPath).get());
    retval.AssignLiteral("fuckfuckfuck");
    // retval = mFullPath;
    printf("%s\n", NS_ConvertUTF16toUTF8(retval).get());
  }
  */
/*
  already_AddRefed<DirectoryReader> CreateReader();

  void GetFile(const nsAString& path, const Flags& options, const Optional< OwningNonNull<EntryCallback> >& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback);

  void GetDirectory(const nsAString& path, const Flags& options, const Optional< OwningNonNull<EntryCallback> >& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback);

  void RemoveRecursively(VoidCallback& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback);
  */
private:
//  nsString mFullPath;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
