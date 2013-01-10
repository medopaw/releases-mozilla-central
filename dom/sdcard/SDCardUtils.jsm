/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

this.EXPORTED_SYMBOLS = ["SDCardUtils", /*"DOMFileSystem", */"DOMDirectoryEntry", "DOMFileEntry", "DOMDirectoryReader", "DOMEntryArray", /*"LocalFile", */"DOMDOMError", "ResultEvent", "ReadEntriesEvent", "CopyEvent", "MoveEvent", "RemoveEvent", "GetParentEvent"];

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
    root: "/sdcard",

    realPath: function(path) {
        return this.root + path;
    },

    nsIFile: function(path) {
        return new FileUtils.File(this.realPath(path));
    },

    nsIFileToDOMEntry: function(file) {
        debug("in nsiIFileToDOMEntry(), file="+file);
        let entry = null, path = file.path.slice(this.root.length) || "/", name = path == "/" ? "" : file.leafName; // root (/sdcard) is the special case

        if (file.isDirectory()) {
            entry = new DOMDirectoryEntry();
            entry.wrappedJSObject.jsinit({
               isDirectory: true,
               isFile: false,
               name: name,
               fullPath: path
            });
        } else {
            entry = new DOMFileEntry();
            entry.wrappedJSObject.jsinit({
               isDirectory: false,
               isFile: true,
               name: name,
               fullPath: path
            });
        }

        return entry;
    },

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
        debug("in postToBackstage()");
        // Cc["@mozilla.org/network/stream-transport-service;1"].getService().QueryInterface(Ci.nsIEventTarget).dispatch(runnable, Ci.nsIEventTarget.DISPATCH_NORMAL);
        Services.tm.currentThread.dispatch(runnable, Ci.nsIEventTarget.DISPATCH_NORMAL);
        // Services.tm.newThread(0).dispatch(runnable, Ci.nsIEventTarget.DISPATCH_NORMAL);
    },

    postToMainThread: function(runnable) {
        Services.tm.mainThread.dispatch(runnable, Ci.nsIEventTarget.DISPATCH_NORMAL);
    },

    handleException: function(exception, errorCallback) {
        debug('Exception caught: '+exception);
        if (errorCallback) {
            SDCardUtils.postToMainThread(new ResultEvent({
                callback: errorCallback,
                result: SDCardUtils.exceptionToDOMError(exception)
            }));
        }
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
    }
};

this.ResultEvent = SDCardUtils.createEventType(function() {
    debug('ResultEvent.run() callback='+this._callback+'result='+this._result);
    this._callback(this._result);
});

this.CopyEvent = SDCardUtils.createEventType(function() {
    try {
        // let file = new FileUtils.File(SDCardUtils.realPath(this._path)), parent = new FileUtils.File(this._parent.fullPath);
        let file = SDCardUtils.nsIFile(this._path), parent = SDCardUtils.nsIFile(this._parent.fullPath);
        debug('Copy '+file.path+' to '+parent.path);
        debug('this._path='+this._path+', this._parent='+this._parent+', this._newName='+this._newName+'; file='+file);
        file.copyTo(parent, this._newName);
        let newFile = parent.clone();
        newFile.append(this._newName || file.leafName);
        debug('newName='+(this._newName||file.leafName)+'newFile='+newFile);
        if (this._onsuccess) {
            SDCardUtils.postToMainThread(new ResultEvent({
                callback: this._onsuccess,
                result: SDCardUtils.nsIFileToDOMEntry(newFile)
            }));
        }
    } catch (ex) {
        SDCardUtils.handleException(ex, this._onerror);
    }
});

this.MoveEvent = SDCardUtils.createEventType(function() {
    try {
        let file = SDCardUtils.nsIFile(this._path), parent = SDCardUtils.nsIFile(this._parent.fullPath);
        debug('Move '+file.path+' to '+parent.path);
        debug('this._path='+this._path+', this._parent='+this._parent+', this._newName='+this._newName+'; file='+file);
        file.moveTo(parent, this._newName);
        let newFile = parent.clone();
        newFile.append(this._newName || file.leafName);
        debug('newName='+(this._newName||file.leafName)+'newFile='+newFile);
        if (this._onsuccess) {
            SDCardUtils.postToMainThread(new ResultEvent({
                callback: this._onsuccess,
                result: SDCardUtils.nsIFileToDOMEntry(newFile)
            }));
        }
    } catch (ex) {
        SDCardUtils.handleException(ex, this._onerror);
    }
});

this.RemoveEvent = SDCardUtils.createEventType(function() {
    try {
        let file = SDCardUtils.nsIFile(this._path);
        file.remove(false); // parameter is a must
        SDCardUtils.postToMainThread(new ResultEvent({
            callback: this._onsuccess
        }));
    } catch (ex) {
        SDCardUtils.handleException(ex, this._onerror);
    }
});

this.GetParentEvent = SDCardUtils.createEventType(function() {
    try {
        let file = SDCardUtils.nsIFile(this._path), parent = this._path == "/" ? file : (file.parent || file);
        SDCardUtils.postToMainThread(new ResultEvent({
            callback: this._onsuccess,
            result: SDCardUtils.nsIFileToDOMEntry(parent)
        }));
    } catch (ex) {
        SDCardUtils.handleException(ex, this._onerror);
    }
});

this.ReadEntriesEvent = SDCardUtils.createEventType(function() {
    try {
        debug('in ReadEntriesEvent.run(). path='+this._path);
        // let dir = new FileUtils.File(this._path);
        let dir = SDCardUtils.nsIFile(this._path);
        debug('dir.path='+dir.path);
        let children = dir.directoryEntries;
        let child, list = [];
        while (children.hasMoreElements()) {
            child = children.getNext().QueryInterface(Ci.nsIFile);
            debug('child.path='+child.path);
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
        SDCardUtils.handleException(ex, this._onerror);
    }
});
