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

struct JSContext;

namespace mozilla {
namespace dom {
namespace sdcard {

class FileEntry MOZ_FINAL : public nsISupports /* Change nativeOwnership in the binding configuration if you don't want this */,
                            public nsWrapperCache /* Change wrapperCache in the binding configuration if you don't want this */,
                            public Entry
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(FileEntry)

public:
  FileEntry();

  ~FileEntry();

  // TODO: return something sensible here, and change the return type
  FileEntry* GetParentObject() const
  {
    return NULL;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);
  bool IsFile() const
  {
    return true;
  }

  bool IsDirectory() const
  {
    return false;
  }
/*
  void CreateWriter(FileWriterCallback& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback);

  void File(FileCallback& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback);
  */
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
