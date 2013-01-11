/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

/* static functions */
const DEBUG = true;

function debug(aStr) {
  if (DEBUG)
    dump("Metadata: " + aStr + "\n");
}

const {
    classes: Cc,
    interfaces: Ci,
    utils: Cu,
    results: Cr
} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/SDCardUtils.jsm");

const METADATA_CONTRACTID = "@mozilla.org/sdcard/metadata;1";
const METADATA_CID        = Components.ID("{2B3C3424-31A5-413A-942B-3CAB1F21CAB8}");
const nsIDOMMetadata   = Ci.nsIDOMMetadata;
const nsIClassInfo     = Ci.nsIClassInfo;

function Metadata() {
  debug("Constructor");
  this.wrappedJSObject = this;
}

Metadata.prototype = {

  classID : METADATA_CID,

  QueryInterface : XPCOMUtils.generateQI([nsIDOMMetadata]),

  classInfo : XPCOMUtils.generateCI({
      classID: METADATA_CID,
      contractID: METADATA_CID,
      classDescription: "Metadata",
      interfaces: [nsIDOMMetadata],
      flags: nsIClassInfo.DOM_OBJECT
  }),

  // this method is meant to be called by wrappedJSObject and is not defined in IDL
  jsinit: SDCardUtils.setPrivates,

  get modificationTime() {
      return this._modificationTime;
  },

  get size() {
      return this._size;
  }

};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([Metadata]);
