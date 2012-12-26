/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

/* static functions */
const DEBUG = true;

function debug(aStr) {
  if (DEBUG)
    dump("EntryArray: " + aStr + "\n");
}

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/SDCardUtils.jsm");

const ENTRYARRAY_CONTRACTID = "@mozilla.org/sdcard/entryarray;1";
const ENTRYARRAY_CID        = Components.ID("{4e9cd6dc-44e0-4cb7-a31d-dbf4b39f3ecf}");
const nsIDOMEntryArray   = Ci.nsIDOMEntryArray;
const nsIClassInfo             = Ci.nsIClassInfo;

function EntryArray() {
  debug("Constructor");
  this.wrappedJSObject = this;
}

EntryArray.prototype = {

  classID : ENTRYARRAY_CID,

  QueryInterface : XPCOMUtils.generateQI([nsIDOMEntryArray]),

  classInfo : XPCOMUtils.generateCI({ classID: ENTRYARRAY_CID,
                                      contractID: ENTRYARRAY_CID,
                                      classDescription: "EntryArray",
                                      interfaces: [nsIDOMEntryArray],
                                      flags: nsIClassInfo.DOM_OBJECT }),

  // this method is meant to be called by wrappedJSObject and is not defined in IDL
  jsinit: SDCardUtils.setPrivates,

  get length() {
      return this._entryarray.length;
  },

  item: function(index) {
      return this._entryarray[index];
  }

};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([EntryArray]);
