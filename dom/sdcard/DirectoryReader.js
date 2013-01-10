/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

/* static functions */
const DEBUG = true;

function debug(aStr) {
  if (DEBUG)
    dump("DirectoryReader: " + aStr + "\n");
}

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/SDCardUtils.jsm");

const DIRECTORYREADER_CONTRACTID = "@mozilla.org/sdcard/directoryreader;1";
const DIRECTORYREADER_CID        = Components.ID("{5e59c1a1-25fa-499c-ae8c-c71d6ac0284b}");
const nsIDOMDirectoryReader   = Ci.nsIDOMDirectoryReader;
const nsIClassInfo             = Ci.nsIClassInfo;

function DirectoryReader() {
  debug("Constructor");
  this.wrappedJSObject = this;
}

DirectoryReader.prototype = {

  classID : DIRECTORYREADER_CID,

  QueryInterface : XPCOMUtils.generateQI([nsIDOMDirectoryReader]),

  classInfo : XPCOMUtils.generateCI({ classID: DIRECTORYREADER_CID,
                                      contractID: DIRECTORYREADER_CID,
                                      classDescription: "DirectoryReader",
                                      interfaces: [nsIDOMDirectoryReader],
                                      flags: nsIClassInfo.DOM_OBJECT }),

  // this method is meant to be called by wrappedJSObject and is not defined in IDL
  jsinit: SDCardUtils.setPrivates,

  readEntries: function(successCallback, errorCallback) {
      debug('in readEntries(), fullPath='+this._fullPath);
      debug('successCallback='+successCallback);
      if (!successCallback) {
          return;
      }
      SDCardUtils.postToBackstage(new ReadEntriesEvent({
          path: this._fullPath,
          onsuccess: successCallback.handleEvent,
          onerror: errorCallback && errorCallback.handleEvent
      }));
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DirectoryReader]);
