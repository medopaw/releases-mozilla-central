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
#include "mozilla/dom/FileSystemBinding.h"

struct JSContext;

namespace mozilla {
namespace dom {
namespace sdcard {

class DirectoryEntry;

class DirectoryReader MOZ_FINAL : public nsISupports /* Change nativeOwnership in the binding configuration if you don't want this */,
                                  public nsWrapperCache /* Change wrapperCache in the binding configuration if you don't want this */
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DirectoryReader)

public:
  DirectoryReader(DirectoryEntry* entry);

  virtual ~DirectoryReader();

  // TODO: return something sensible here, and change the return type
  DirectoryReader* GetParentObject() const
  {
    return NULL;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope);

  void ReadEntries(EntriesCallback& successCallback,
      const Optional<OwningNonNull<ErrorCallback> >& errorCallback);
private:
  nsRefPtr<DirectoryEntry> mEntry;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
