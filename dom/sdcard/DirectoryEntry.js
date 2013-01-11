/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

/* static functions */
const DEBUG = true;

function debug(aStr) {
  if (DEBUG)
    dump("DirectoryEntry: " + aStr + "\n");
}

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/SDCardUtils.jsm");

const DIRECTORYENTRY_CONTRACTID = "@mozilla.org/sdcard/directoryentry;1";
const DIRECTORYENTRY_CID        = Components.ID("{7decc5e9-2f55-4f9f-ae1a-0b2f7ee083eb}");
const nsIDOMDirectoryEntry   = Ci.nsIDOMDirectoryEntry;
const nsIClassInfo             = Ci.nsIClassInfo;

function DirectoryEntry() {
  debug("Constructor");
  this.wrappedJSObject = this;
}

DirectoryEntry.prototype = {

  classID : DIRECTORYENTRY_CID,

  QueryInterface : XPCOMUtils.generateQI([nsIDOMDirectoryEntry]),

  classInfo : XPCOMUtils.generateCI({ classID: DIRECTORYENTRY_CID,
                                      contractID: DIRECTORYENTRY_CID,
                                      classDescription: "DirectoryEntry",
                                      interfaces: [nsIDOMDirectoryEntry],
                                      flags: nsIClassInfo.DOM_OBJECT }),

  // this method is meant to be called by wrappedJSObject and is not defined in IDL
  jsinit: SDCardUtils.setPrivates,

  get isFile() {
      return this._isFile;
  },

  get isDirectory() {
      return this._isDirectory;
  },

  get name() {
      return this._name;
  },

  get fullPath() {
      return this._fullPath;
  },

  /* get filesystem() {
      return this._filesystem;
  },*/

  getMetadata: function(successCallback, errorCallback) {
      if (!successCallback) {
          return;
      }
      SDCardUtils.postToBackstage(new GetMetadataEvent({
          path: this._fullPath,
          onsuccess: successCallback.handleEvent,
          onerror: errorCallback && errorCallback.handleEvent
      }));
  },

  copyTo: function(parent, newName, successCallback, errorCallback) {
      if (!parent) {
          return;
      }
      SDCardUtils.postToBackstage(new CopyEvent({
          path: this._fullPath,
          parent: parent,
          newName: newName,
          onsuccess: successCallback && successCallback.handleEvent,
          onerror: errorCallback && errorCallback.handleEvent
      }));
  },

  getParent: function(successCallback, errorCallback) {
      if (!successCallback) {
          return;
      }
      SDCardUtils.postToBackstage(new GetParentEvent({
          path: this._fullPath,
          onsuccess: successCallback.handleEvent,
          onerror: errorCallback && errorCallback.handleEvent
      }));
  },

  createReader: function() {
      // this._reader = Cc["@mozilla.org/sdcard/directoryreader;1"].createInstance(Ci.nsIDOMDirectoryReader);
      this._reader = new DOMDirectoryReader();
      this._reader.wrappedJSObject.jsinit({
          fullPath: this._fullPath
      });
      return this._reader;
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DirectoryEntry]);
