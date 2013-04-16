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

struct JSContext;

namespace mozilla {
namespace dom {
namespace sdcard {

class Metadata MOZ_FINAL : public nsISupports /* Change nativeOwnership in the binding configuration if you don't want this */,
                           public nsWrapperCache /* Change wrapperCache in the binding configuration if you don't want this */
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Metadata)

public:
  Metadata();

  virtual ~Metadata();

  // TODO: return something sensible here, and change the return type
  Metadata* GetParentObject() const
  {
    return NULL;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope);

  JS::Value ModificationTime(JSContext* cx) const;

  uint64_t Size() const;

public:
  void SetModificationTime(PRTime mtime);

  void SetSize(uint64_t value)
  {
    mSize = value;
  }

private:
  uint64_t mSize;
  // Modification timestamp in milliseconds
  double mDate;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
