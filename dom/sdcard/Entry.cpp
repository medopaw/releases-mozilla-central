/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Entry.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsContentUtils.h"

#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {


// NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(Entry, mFileSystem)
// NS_IMPL_CYCLE_COLLECTION_1(Entry, mFullPath)
/*
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            Entryh& aField,
                            const char* aName,
                            unsigned aFlags)
{
  CycleCollectionNoteChild(aCallback, aField.mInputNode.get(), aName, aFlags);
}

inline void
ImplCycleCollectionUnlink(nsCycleCollectionTraversalCallback& aCallback,
                          Entry::mFullPath& aField,
                          const char* aName,
                          unsigned aFlags)
{
  aField.mInputNode = nullptr;
}
*/

NS_IMPL_ADDREF(Entry)
NS_IMPL_RELEASE(Entry)
NS_INTERFACE_MAP_BEGIN(Entry)
//  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

/*
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(Entry)
  // NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(Entry)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE(Entry)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Entry)
NS_INTERFACE_MAP_END(Entry)
*/

// NS_IMPL_CYCLE_COLLECTION_1(Entry, mFileSystem);
//
/*
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(Entry)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(Entry)
//  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  return NS_SUCCESS_INTERRUPTED_TRAVERSE;
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(Entry)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
*/
/*
NS_IMPL_CYCLE_COLLECTING_ADDREF(Entry)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Entry)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Entry)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
*/

Entry::Entry(const nsAString& aFullPath) : mFullPath(aFullPath)
{
  SDCARD_LOG("init Entry");
  // printf("\ninit Entry\n");
 // printf("%s\n", NS_ConvertUTF16toUTF8(mFullPath).get());
//  SetIsDOMBinding();
}

Entry::~Entry()
{
}

void Entry::GetFullPath(nsString& retval) const
{
  retval = mFullPath;
  SDCARD_LOG("in Entry.GetFullPath()!!!!");
  SDCARD_LOG("mFullPath=%s", NS_ConvertUTF16toUTF8(mFullPath).get());
  SDCARD_LOG("retval=%s", NS_ConvertUTF16toUTF8(retval).get());
}

/*
JSObject*
Entry::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return EntryBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}
*/


} // namespace sdcard
} // namespace dom
} // namespace mozilla
