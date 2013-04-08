/*
 * GetParentRunnable.cpp
 *
 *  Created on: Apr 8, 2013
 *      Author: yuan
 */

#include "GetParentRunnable.h"

namespace mozilla {
namespace dom {
namespace sdcard {

GetParentRunnable::~GetParentRunnable()
{
  // TODO Auto-generated destructor stub
}

NS_IMETHODIMP GetParentRunnable::Run()
{
  SDCARD_LOG("in GetParentRunnable.Run()!");
  SDCARD_LOG("on main thread:%d", NS_IsMainThread());
  MOZ_ASSERT(!NS_IsMainThread(), "Never call on main thread!");
  Entry* parent = mEntry->GetParentInternal();
  if (parent) {
    // success callback
    nsCOMPtr<nsIRunnable> r = new ResultRunnable<EntryCallback, Entry>(
        *mSuccessCallback, parent);
    NS_DispatchToMainThread(r);
  } else {
    // error callback
  }
  return NS_OK;
}


} // namespace sdcard
} // namespace dom
} // namespace mozilla
