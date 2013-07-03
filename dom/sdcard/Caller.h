/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "nsString.h"
#include "mozilla/dom/FileSystemBinding.h"

namespace mozilla {
namespace dom {
namespace sdcard {

/*
 * This class is for callback handling.
 */
class Caller
{
public:
  Caller();
  Caller(CallbackFunction* aSuccessCallback, ErrorCallback* aErrorCallback);
  virtual ~Caller();

  void CallErrorCallback(const nsAString& error);
  void CallEntryCallback(const nsAString& path);
  void CallEntriesCallback(const InfallibleTArray<nsString>& paths);
  void CallMetadataCallback(int64_t modificationTime, uint64_t size);
  void CallVoidCallback();

private:
  nsRefPtr<CallbackFunction> mSuccessCallback;
  nsRefPtr<ErrorCallback> mErrorCallback;
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
