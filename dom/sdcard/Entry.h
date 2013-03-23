/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
// #include "nsCycleCollectionParticipant.h"
// #include "nsWrapperCache.h"

#include "nsString.h"

struct JSContext;

namespace mozilla {
namespace dom {
namespace sdcard {

class Entry /*: public nsISupports  Change nativeOwnership in the binding configuration if you don't want this ,*/
                        /*public nsWrapperCache  Change wrapperCache in the binding configuration if you don't want this */
{
public:
//  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
//  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Entry)

public:
  Entry();

  ~Entry();

  // TODO: return something sensible here, and change the return type
  Entry* GetParentObject() const
  {
    return NULL;
  }

//  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);

  virtual bool IsFile() const = 0;

  virtual bool IsDirectory() const = 0;
/*
  void GetMetadata(MetadataCallback& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback);

  void GetName(nsString& retval) const;

  void GetFullPath(nsString& retval) const;

  // Mark this as resultNotAddRefed to return raw pointers
  already_AddRefed<sdcard::FileSystem> Filesystem() const;

  void MoveTo(mozilla::dom::sdcard::DirectoryEntry& parent, const Optional< nsAString >& newName, const Optional< OwningNonNull<EntryCallback> >& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback);

  void CopyTo(mozilla::dom::sdcard::DirectoryEntry& parent, const Optional< nsAString >& newName, const Optional< OwningNonNull<EntryCallback> >& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback);

  void ToURL(nsString& retval);

  void Remove(VoidCallback& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback);

  void GetParent(EntryCallback& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback);
  */

protected:
  nsString mFullPath;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
