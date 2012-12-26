/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

this.EXPORTED_SYMBOLS = ["SDCardUtils", /*"DOMFileSystem", */"DOMDirectoryEntry", "DOMFileEntry", "DOMDirectoryReader", "DOMEntryArray", /*"LocalFile", */"DOMDOMError", "ResultEvent", "ReadEntriesEvent", "GetParentEvent"];

const DEBUG = true;

function debug(aStr) {
  if (DEBUG)
    dump("SDCardUtils: " + aStr + "\n");
}

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");

// this.DOMFileSystem = Components.Constructor("@mozilla.org/sdcard/filesystem;1", "nsIDOMFileSystem");

this.DOMDirectoryEntry = Components.Constructor("@mozilla.org/sdcard/directoryentry;1", "nsIDOMDirectoryEntry");

this.DOMFileEntry = Components.Constructor("@mozilla.org/sdcard/fileentry;1", "nsIDOMFileEntry");

this.DOMDirectoryReader = Components.Constructor("@mozilla.org/sdcard/directoryreader;1", "nsIDOMDirectoryReader");

this.DOMEntryArray = Components.Constructor("@mozilla.org/sdcard/entryarray;1", "nsIDOMEntryArray");

// this.LocalFile = Components.Constructor("@mozilla.org/file/local;1", "nsIFile");

this.DOMDOMError = Components.Constructor("@mozilla.org/dom-error;1", "nsIDOMDOMError");

this.SDCardUtils = {
    setPrivates: function(args) {
        for (let i in args) {
            this["_" + i] = args[i];
        }
    },

    createEventType: function(run) {
        // need to be seperate functions instead of refs to the same function, otherwise prototype would be messed
        let eventType = function() {
            SDCardUtils.setPrivates.apply(this, arguments);
        };
        eventType.prototype.QueryInterface = XPCOMUtils.generateQI([Ci.nsIRunnable]);
        eventType.prototype.run = run;
        return eventType;
    },

    postToBackstage: function(runnable) {
        // Cc["@mozilla.org/network/stream-transport-service;1"].getService().QueryInterface(Ci.nsIEventTarget).dispatch(runnable, Ci.nsIEventTarget.DISPATCH_NORMAL);
        Services.tm.currentThread.dispatch(runnable, Ci.nsIEventTarget.DISPATCH_NORMAL);
        // Services.tm.newThread(0).dispatch(runnable, Ci.nsIEventTarget.DISPATCH_NORMAL);
    },

    postToMainThread: function(runnable) {
        Services.tm.mainThread.dispatch(runnable, Ci.nsIEventTarget.DISPATCH_NORMAL);
    },

    createDOMError: function(aError) {
        // let error = Cc["@mozilla.org/dom-error;1"].createInstance(Ci.nsIDOMDOMError);
        let error = new DOMDOMError(); // this.DOMDOMError == undefined. amazing!
        error.wrappedJSObject.init(aError);
        debug(error.wrappedJSObject);
        return error;
    },

    exceptionToDOMError: function(ex) {
        debug('exception.name='+ex.name);
        let error = '';
        switch (ex.name) {
            case 'NS_ERROR_FILE_NOT_DIRECTORY':
                error = 'InvalidAccessError';
                break;
            case 'NS_ERROR_FILE_TARGET_DOES_NOT_EXIST':
                error = 'NotFoundError';
                break;
            default:
                error = 'UnknowError';
                break;
        }
        debug('error='+error);
        return this.createDOMError(error);
    },

    nsIFileToDOMEntry: function(file) {
        let entry = null;

        if (file.isDirectory()) {
            entry = new DOMDirectoryEntry();
            entry.wrappedJSObject.jsinit({
               isDirectory: true,
               isFile: false,
               name: file.leafName,
               fullPath: file.path
            });
        } else {
            entry = new DOMFileEntry();
            entry.wrappedJSObject.jsinit({
               isDirectory: false,
               isFile: true,
               name: file.leafName,
               fullPath: file.path
            });
        }

        return entry;
    }
};

this.ResultEvent = SDCardUtils.createEventType(function() {
    debug('ResultEvent.run() callback='+this._callback+'result='+this._result);
    this._callback(this._result);
});

this.CopyEvent = SDCardUtils.createEventType(function() {
    try {
        let file = new FileUtils.File(this._path), parent = new FileUtils.File(this._parent.fullPath);
        file.copyTo(parent, this._newName);
        let newFile = parent.append(this._newName);
        SDCardUtils.postToMainThread(new ResultEvent({
            callback: this._onsuccess,
            result: SDCardUtils.nsIFileToDOMEntry(newFile)
        }));
    } catch (ex) {
        debug('Exception caught: '+ex);
        SDCardUtils.postToMainThread(new ResultEvent({
            callback: this._onerror,
            result: SDCardUtils.exceptionToDOMError(ex)
        }));
    }
});

this.GetParentEvent = SDCardUtils.createEventType(function() {
    try {
        let file = new FileUtils.File(this._path), parent = file.parent || file;
        /* let entry = new DOMDirectoryEntry();
        entry.wrappedJSObject.jsinit({
            isDirectory: true,
            isFile: false,
            name: parent.leafName,
            fullPath: parent.path
        });*/
        SDCardUtils.postToMainThread(new ResultEvent({
            callback: this._onsuccess,
            result: SDCardUtils.nsIFileToDOMEntry(parent)// entry
        }));
    } catch (ex) {
        debug('Exception caught: '+ex);
        SDCardUtils.postToMainThread(new ResultEvent({
            callback: this._onerror,
            result: SDCardUtils.exceptionToDOMError(ex)
        }));
    }
});

this.ReadEntriesEvent = SDCardUtils.createEventType(function() {
    try {
        debug('ReadEntriesEvent.run(). path='+this._path);
        let dir = new FileUtils.File(this._path);
        debug('dir.path='+dir.path);
        let children = dir.directoryEntries;
        let child, list = [];
        while (children.hasMoreElements()) {
            child = children.getNext().QueryInterface(Ci.nsIFile);
            debug('child.path='+child.path);
/*            if (child.isDirectory()) {
                entry = new DOMDirectoryEntry();
                entry.wrappedJSObject.jsinit({
                   isDirectory: true,
                   isFile: false,
                   name: child.leafName,
                   fullPath: child.path
                });
                list.push(entry);
            } else {
                entry = new DOMFileEntry();
                entry.wrappedJSObject.jsinit({
                   isDirectory: false,
                   isFile: true,
                   name: child.leafName,
                   fullPath: child.path
                });
                list.push(entry);
            }*/
            list.push(SDCardUtils.nsIFileToDOMEntry(child));
        }
        let entryArray = new DOMEntryArray();
        entryArray.wrappedJSObject.jsinit({entryarray: list});
        // throw({name: 'haha'});
        SDCardUtils.postToMainThread(new ResultEvent({
            callback: this._onsuccess,
            result: entryArray
        }));
    } catch(ex) {
        debug('Exception caught: '+ex);
        SDCardUtils.postToMainThread(new ResultEvent({
            callback: this._onerror,
            result: SDCardUtils.exceptionToDOMError(ex)
        }));
    }
});
