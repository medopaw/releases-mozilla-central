/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"

#include "mozilla/dom/FileSystemBinding.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"
#include "nsIFile.h"
#include "nsString.h"

struct JSContext;

namespace mozilla {
namespace dom {
namespace sdcard {

class FileSystem;

class Entry : public nsISupports, /* Change nativeOwnership in the binding configuration if you don't want this */
              public nsWrapperCache
{
public:
  NS_DECL_ISUPPORTS

public:
  // nsIFile to Entry
  static Entry* FromFile(FileSystem* aFilesystem, nsIFile* aFile);

  virtual ~Entry();

  // TODO: return something sensible here, and change the return type
  Entry* GetParentObject() const
  {
    return NULL;
  }

//  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);

  bool IsFile() const;

  bool IsDirectory() const;

  void GetMetadata(MetadataCallback& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback);

  void GetName(nsString& retval) const;

  void GetFullPath(nsString& retval) const;

  already_AddRefed<FileSystem> Filesystem() const;

  void MoveTo(DirectoryEntry& parent, const Optional<nsAString >& newName,
      const Optional<OwningNonNull<EntryCallback> >& successCallback,
      const Optional<OwningNonNull<ErrorCallback> >& errorCallback);

  void CopyTo(DirectoryEntry& parent, const Optional<nsAString >& newName,
      const Optional<OwningNonNull<EntryCallback> >& successCallback,
      const Optional<OwningNonNull<ErrorCallback> >& errorCallback);

/*
  void ToURL(nsString& retval);
*/

  void Remove(VoidCallback& successCallback, const Optional< OwningNonNull<ErrorCallback> >& errorCallback);

  void GetParent(EntryCallback& successCallback,
      const Optional<OwningNonNull<ErrorCallback> >& errorCallback);

  bool Exists() const;

public:
  bool IsRoot() const;

  FileSystem* GetFilesystem() const
  {
    return mFilesystem.get();
  }

  nsIFile* GetFileInternal() const
  {
    return mFile;
  }

  nsRefPtr<Metadata> mMetadata;

protected:
  // full path in filesystem
  // explicit Entry(FileSystem* aFilesystem, const nsAString& aFullPath);
  explicit Entry(FileSystem* aFilesystem, nsIFile* aFile, bool aIsFile, bool aIsDirectory);

  nsRefPtr<FileSystem> mFilesystem;
  nsCOMPtr<nsIFile> mFile;
  bool mIsFile;
  bool mIsDirectory;

private:
  void CopyAndMoveTo(DirectoryEntry& parent,
      const Optional<nsAString >& newName,
      const Optional<OwningNonNull<EntryCallback> >& successCallback,
      const Optional<OwningNonNull<ErrorCallback> >& errorCallback,
      bool isCopy);
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
