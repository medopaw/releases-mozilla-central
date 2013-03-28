/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#define DEBUGGING
#ifdef DEBUGGING
// #define SDCARD_LOG printf
#define SDCARD_LOG(fmt, ...) printf("\n\n"); printf(fmt, ##__VA_ARGS__)
#else
#define SDCARD_LOG(fmt, ...)
#endif

/*
// Some header defines a LOG macro, but we don't want it here.
#ifdef LOG
#undef LOG
#endif

#define LOG printf
// Enable logging by setting
//
//   NSPR_LOG_MODULES=ProcessPriorityManager:5
//
// in your environment.  Or just comment out the "&& 0" below, if you're on
// Android/B2G.

#if defined(ANDROID) && 0
#include <android/log.h>
#define LOG(fmt, ...) \
  __android_log_print(ANDROID_LOG_INFO, \
      "Gecko:ProcessPriorityManager", \
      fmt, ## __VA_ARGS__)
#elif defined(PR_LOGGING)
static PRLogModuleInfo*
GetPPMLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("ProcessPriorityManager");
  return sLog;
}
#define LOG(fmt, ...) \
  PR_LOG(GetPPMLog(), PR_LOG_DEBUG,                                     \
         ("[%d] ProcessPriorityManager - " fmt, getpid(), ##__VA_ARGS__))
#else
#define LOG(fmt, ...)
#endif
*/

// LOG("in Utils.h, testing LOG()");
