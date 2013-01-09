/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

/* static functions */
const DEBUG = true;

function debug(aStr) {
  if (DEBUG)
    dump("FileSystem: " + aStr + "\n");
}

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
// Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/SDCardUtils.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");

const FILESYSTEM_CONTRACTID = "@mozilla.org/sdcard/filesystem;1";
const FILESYSTEM_CID        = Components.ID("{05d87ade-f1aa-4e0b-bb04-c068e22aa7eb}");
const nsIDOMFileSystem   = Ci.nsIDOMFileSystem;
const nsIClassInfo             = Ci.nsIClassInfo;

function FileSystem() {
  debug("Constructor");
}

FileSystem.prototype = {

  __proto__: DOMRequestIpcHelper.prototype,

  classID : FILESYSTEM_CID,

  QueryInterface : XPCOMUtils.generateQI([nsIDOMFileSystem, Ci.nsIDOMGlobalPropertyInitializer]),

  classInfo : XPCOMUtils.generateCI({ classID: FILESYSTEM_CID,
                                      contractID: FILESYSTEM_CID,
                                      classDescription: "FileSystem",
                                      interfaces: [nsIDOMFileSystem],
                                      flags: nsIClassInfo.DOM_OBJECT }),


  get name() {
      return this._name;
  },

  get root() {
        return this._root;
  },

  // nsIDOMGlobalPropertyInitializer implementation
  init: function (aWindow) {
    debug("init()");
    this._name = "SD Card";
    // this._tmp = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
   //  this._root = Cc["@mozilla.org/sdcard/directoryentry;1"].createInstance(Ci.nsIDOMDirectoryEntry);
   this._root = new DOMDirectoryEntry();
   this._root.wrappedJSObject.jsinit({
       isFile: false,
       isDirectory: true,
       name: "",
       fullPath: "/" // SDCardUtils.rootPath
        // filesystem: this
   });

    /*initializer = this._root.QueryInterface(Ci.nsISDCardInitializer);
    initializer.init({
        path: ,
        dfjdk,
    });
    initializer.initPath("");*/

/*
    // Set navigator.mozAlarms to null.
    if (!Services.prefs.getBoolPref("dom.mozAlarms.enabled"))
      return null;

    let principal = aWindow.document.nodePrincipal;
    let secMan = Cc["@mozilla.org/scriptsecuritymanager;1"].getService(Ci.nsIScriptSecurityManager);

    let perm = Services.perms.testExactPermissionFromPrincipal(principal, "alarms");

    // Only pages with perm set can use the alarms.
    this.hasPrivileges = perm == Ci.nsIPermissionManager.ALLOW_ACTION;

    if (!this.hasPrivileges)
      return null;

    this._cpmm = Cc["@mozilla.org/childprocessmessagemanager;1"].getService(Ci.nsISyncMessageSender);

    // Add the valid messages to be listened.
    this.initHelper(aWindow, ["AlarmsManager:Add:Return:OK", "AlarmsManager:Add:Return:KO",
                              "AlarmsManager:GetAll:Return:OK", "AlarmsManager:GetAll:Return:KO"]);

    // Get the manifest URL if this is an installed app
    let appsService = Cc["@mozilla.org/AppsService;1"]
                        .getService(Ci.nsIAppsService);
    this._pageURL = principal.URI.spec;
    this._manifestURL = appsService.getManifestURLByLocalId(principal.appId);
    */
  },

  // Called from DOMRequestIpcHelper.
  uninit: function () {
    debug("uninit()");
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([FileSystem]);
