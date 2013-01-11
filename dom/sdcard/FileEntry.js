/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

/* static functions */
const DEBUG = true;

function debug(aStr) {
  if (DEBUG)
    dump("FileEntry: " + aStr + "\n");
}

const {
    classes: Cc,
    interfaces: Ci,
    utils: Cu,
    results: Cr
} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/SDCardUtils.jsm");

const FILEENTRY_CONTRACTID = "@mozilla.org/sdcard/fileentry;1";
const FILEENTRY_CID        = Components.ID("{5a8c0260-74a6-46dd-8fa4-926b9557c1e3}");
const nsIDOMFileEntry   = Ci.nsIDOMFileEntry;
const nsIClassInfo             = Ci.nsIClassInfo;

function FileEntry() {
  debug("Constructor");
  this.wrappedJSObject = this;
}

FileEntry.prototype = {

  classID : FILEENTRY_CID,

  QueryInterface : XPCOMUtils.generateQI([nsIDOMFileEntry]),

  classInfo : XPCOMUtils.generateCI({
      classID: FILEENTRY_CID,
      contractID: FILEENTRY_CID,
      classDescription: "FileEntry",
      interfaces: [nsIDOMFileEntry],
      flags: nsIClassInfo.DOM_OBJECT
  }),

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

  moveTo: function(parent, newName, successCallback, errorCallback) {
      if (!parent) {
          return;
      }
      SDCardUtils.postToBackstage(new MoveEvent({
          path: this._fullPath,
          parent: parent,
          newName: newName,
          onsuccess: successCallback && successCallback.handleEvent,
          onerror: errorCallback && errorCallback.handleEvent
      }));
 },

 remove: function(successCallback, errorCallback) {
      if (!successCallback) {
          return;
      }
      SDCardUtils.postToBackstage(new RemoveEvent({
          path: this._fullPath,
          onsuccess: successCallback.handleEvent,
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
  }

  /* get filesystem() {
      return this._filesystem;
  },*/

};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([FileEntry]);
