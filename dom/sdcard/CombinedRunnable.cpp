/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CombinedRunnable.h"
#include "Entry.h"
#include "mozilla/dom/FileSystemBinding.h"
#include "nsString.h"
#include "Utils.h"

namespace mozilla {
namespace dom {
namespace sdcard {

// nsCOMPtr<nsIThread> CombinedRunnable::sWorkerThread;

const nsString DOM_ERROR_ENCODING                =   NS_LITERAL_STRING("EncodingError");
const nsString DOM_ERROR_INVALID_MODIFICATION    =   NS_LITERAL_STRING("InvalidModificationError");
const nsString DOM_ERROR_INVALID_STATE           =   NS_LITERAL_STRING("InvalidStateError");
const nsString DOM_ERROR_NOT_FOUND               =   NS_LITERAL_STRING("NotFoundError");
const nsString DOM_ERROR_NOT_READABLE            =   NS_LITERAL_STRING("NotReadableError");
const nsString DOM_ERROR_NO_MODIFICATION_ALLOWED =   NS_LITERAL_STRING("NoModificationAllowedError");
const nsString DOM_ERROR_PATH_EXISTS             =   NS_LITERAL_STRING("PathExistsError");
const nsString DOM_ERROR_QUOTA_EXCEEDED          =   NS_LITERAL_STRING("QuotaExceededError");
const nsString DOM_ERROR_SECURITY                =   NS_LITERAL_STRING("SecurityError");
const nsString DOM_ERROR_TYPE_MISMATCH           =   NS_LITERAL_STRING("TypeMismatchError");
const nsString DOM_ERROR_UNKNOWN                 =  NS_LITERAL_STRING("Unknown");

CombinedRunnable::CombinedRunnable(Entry* entry) :
    mEntry(entry),
    mErrorCode(NS_OK),
    mWorkerThread(nullptr)
{
}

CombinedRunnable::~CombinedRunnable()
{
}

void CombinedRunnable::Start()
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  // run worker thread
  if (!mWorkerThread) {
    nsresult rv = NS_NewThread(getter_AddRefs(mWorkerThread));
    if (NS_FAILED(rv)) {
      mWorkerThread = nullptr;
      // we need to call errorcallback here
      // mErrorCallback.Call(xxx);
      return;
    }
  }
  mWorkerThread->Dispatch(this, NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP CombinedRunnable::Run()
{
  if (!NS_IsMainThread()) {
    SDCARD_LOG("CombinedRunnable.Run() on worker thread.");
    // run worker thread tasks, majorly file io operations
    WorkerThreadRun();
    // dispatch itself to main thread
    NS_DispatchToMainThread(this);
  } else {
    SDCARD_LOG("CombinedRunnable.Run() on main thread.");
    // shutdown mWorkerThread
    if (!mWorkerThread) {
      mWorkerThread->Shutdown();
    }
    // run main thread tasks, majorly call callbacks
    MainThreadRun();
    // ensure mEntry is released on main thread
    mEntry = nullptr;
  }

  return NS_OK;
}

already_AddRefed<nsIDOMDOMError> CombinedRunnable::GetDOMError() const
{
  already_AddRefed<nsIDOMDOMError> domError = nullptr;
  nsString name;
  if (!mErrorName.IsEmpty()) {
    name = mErrorName;
  } else if (mErrorCode != NS_OK) {
    switch (mErrorCode) {
    case NS_ERROR_FILE_INVALID_PATH:
    case NS_ERROR_FILE_UNRECOGNIZED_PATH:
      name = DOM_ERROR_ENCODING;
      break;
    case NS_ERROR_FILE_DESTINATION_NOT_DIR:
      name = DOM_ERROR_INVALID_MODIFICATION;
      break;
    case NS_ERROR_FILE_ACCESS_DENIED:
    case NS_ERROR_FILE_DIR_NOT_EMPTY:
      name = DOM_ERROR_NO_MODIFICATION_ALLOWED;
      break;
    case NS_ERROR_FILE_TARGET_DOES_NOT_EXIST:
    case NS_ERROR_NOT_AVAILABLE:
      name = DOM_ERROR_NOT_FOUND;
      break;
    case NS_ERROR_FILE_ALREADY_EXISTS:
      name = DOM_ERROR_PATH_EXISTS;
      break;
    case NS_ERROR_DOM_SECURITY_ERR:
    case NS_ERROR_OUT_OF_MEMORY:
      name = DOM_ERROR_SECURITY;
      break;
    case NS_ERROR_FILE_NOT_DIRECTORY:
      name = DOM_ERROR_TYPE_MISMATCH;
      break;
    case NS_ERROR_UNEXPECTED:
    default:
      name = DOM_ERROR_UNKNOWN;
      break;
    }
  }
  if (!name.IsEmpty()) {
    domError = DOMError::CreateWithName(name);
  }
  return domError;
}

Entry* CombinedRunnable::GetEntry() const
{
  MOZ_ASSERT(NS_IsMainThread(), "only call on main thread!");
  if (NS_IsMainThread()) {
    return mEntry;
  } else {
    return nullptr;
  }
}

} /* namespace sdcard */
} /* namespace dom */
} /* namespace mozilla */
