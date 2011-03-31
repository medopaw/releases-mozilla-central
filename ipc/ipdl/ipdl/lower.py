# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is mozilla.org code.
#
# Contributor(s):
#   Chris Jones <jones.chris.g@gmail.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either of the GNU General Public License Version 2 or later (the "GPL"),
# or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

import os, re, sys
from copy import deepcopy

import ipdl.ast
from ipdl.cxx.ast import *
from ipdl.type import TypeVisitor

# FIXME/cjones: the chromium Message logging code doesn't work on
# gcc/POSIX, because it wprintf()s across the chromium/mozilla
# boundary. one side builds with -fshort-wchar, the other doesn't.
# this code will remain off until the chromium base lib is replaced
EMIT_LOGGING_CODE = ('win32' == sys.platform)

##-----------------------------------------------------------------------------
## "Public" interface to lowering
##
class LowerToCxx:
    def lower(self, tu):
        '''returns |[ header: File ], [ cpp : File ]| representing the
lowered form of |tu|'''
        # annotate the AST with IPDL/C++ IR-type stuff used later
        tu.accept(_DecorateWithCxxStuff())

        pname = tu.protocol.name

        pheader = File(pname +'.h')
        _GenerateProtocolHeader().lower(tu, pheader)

        parentheader, parentcpp = File(pname +'Parent.h'), File(pname +'Parent.cpp')
        _GenerateProtocolParentCode().lower(
            tu, pname+'Parent', parentheader, parentcpp)

        childheader, childcpp = File(pname +'Child.h'), File(pname +'Child.cpp')
        _GenerateProtocolChildCode().lower(
            tu, pname+'Child', childheader, childcpp)

        return [ pheader, parentheader, childheader ], [ parentcpp, childcpp ]


##-----------------------------------------------------------------------------
## Helper code
##

_NULL_ACTOR_ID = ExprLiteral.ZERO
_FREED_ACTOR_ID = ExprLiteral.ONE

class _struct: pass

def _protocolHeaderName(p, side=''):
    if side: side = side.title()
    base = p.name + side

    
    pfx = '/'.join([ ns.name for ns in p.namespaces ])
    if pfx: return pfx +'/'+ base
    else:   return base

def _includeGuardMacroName(headerfile):
    return re.sub(r'[./]', '_', headerfile.name)

def _includeGuardStart(headerfile):
    guard = _includeGuardMacroName(headerfile)
    return [ CppDirective('ifndef', guard),
             CppDirective('define', guard)  ]

def _includeGuardEnd(headerfile):
    guard = _includeGuardMacroName(headerfile)
    return [ CppDirective('endif', '// ifndef '+ guard) ]

def _messageStartName(ptype):
    return ptype.name() +'MsgStart'

def _protocolId(ptype):
    return ExprVar(_messageStartName(ptype))

def _protocolIdType():
    return Type('int32')

def _actorName(pname, side):
    """|pname| is the protocol name. |side| is 'Parent' or 'Child'."""
    tag = side
    if not tag[0].isupper():  tag = side.title()
    return pname + tag

def _actorIdType():
    return Type('int32')

def _actorId(actor=None):
    if actor is not None:
        return ExprSelect(actor, '->', 'mId')
    return ExprVar('mId')

def _actorHId(actorhandle):
    return ExprSelect(actorhandle, '.', 'mId')

def _actorChannel(actor):
    return ExprSelect(actor, '->', 'mChannel')

def _actorManager(actor):
    return ExprSelect(actor, '->', 'mManager')

def _actorState(actor):
    return ExprSelect(actor, '->', 'mState')

def _nullState(proto=None):
    pfx = ''
    if proto is not None:  pfx = proto.name() +'::'
    return ExprVar(pfx +'__Null')

def _errorState(proto=None):
    pfx = ''
    if proto is not None:  pfx = proto.name() +'::'
    return ExprVar(pfx +'__Error')

def _deadState(proto=None):
    pfx = ''
    if proto is not None:  pfx = proto.name() +'::'
    return ExprVar(pfx +'__Dead')

def _startState(proto=None, fq=False):
    pfx = ''
    if proto:
        if fq:  pfx = proto.fullname() +'::'
        else:   pfx = proto.name() +'::'
    return ExprVar(pfx +'__Start')

def _deleteId():
    return ExprVar('Msg___delete____ID')

def _lookupListener(idexpr):
    return ExprCall(ExprVar('Lookup'), args=[ idexpr ])

def _shmemType(ptr=0, const=1, ref=0):
    return Type('Shmem', ptr=ptr, ref=ref)

def _rawShmemType(ptr=0):
    return Type('Shmem::SharedMemory', ptr=ptr)

def _shmemIdType(ptr=0):
    return Type('Shmem::id_t', ptr=ptr)

def _shmemTypeType():
    return Type('Shmem::SharedMemory::SharedMemoryType')

def _shmemBackstagePass():
    return ExprCall(ExprVar(
        'Shmem::IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead'))

def _shmemCtor(rawmem, idexpr):
    return ExprCall(ExprVar('Shmem'),
                    args=[ _shmemBackstagePass(), rawmem, idexpr ])

def _shmemId(shmemexpr):
    return ExprCall(ExprSelect(shmemexpr, '.', 'Id'),
                    args=[ _shmemBackstagePass() ])

def _shmemSegment(shmemexpr):
    return ExprCall(ExprSelect(shmemexpr, '.', 'Segment'),
                    args=[ _shmemBackstagePass() ])

def _shmemAlloc(size, type, unsafe):
    # starts out UNprotected
    return ExprCall(ExprVar('Shmem::Alloc'),
                    args=[ _shmemBackstagePass(), size, type, unsafe ])

def _shmemDealloc(rawmemvar):
    return ExprCall(ExprVar('Shmem::Dealloc'),
                    args=[ _shmemBackstagePass(), rawmemvar ])

def _shmemShareTo(shmemvar, processvar, route):
    return ExprCall(ExprSelect(shmemvar, '.', 'ShareTo'),
                    args=[ _shmemBackstagePass(),
                           processvar, route ])

def _shmemOpenExisting(descriptor, outid):
    # starts out protected
    return ExprCall(ExprVar('Shmem::OpenExisting'),
                    args=[ _shmemBackstagePass(),
                           # true => protect
                           descriptor, outid, ExprLiteral.TRUE ])

def _shmemUnshareFrom(shmemvar, processvar, route):
    return ExprCall(ExprSelect(shmemvar, '.', 'UnshareFrom'),
                    args=[ _shmemBackstagePass(),
                           processvar, route ])

def _shmemForget(shmemexpr):
    return ExprCall(ExprSelect(shmemexpr, '.', 'forget'),
                    args=[ _shmemBackstagePass() ])

def _shmemRevokeRights(shmemexpr):
    return ExprCall(ExprSelect(shmemexpr, '.', 'RevokeRights'),
                    args=[ _shmemBackstagePass() ])

def _lookupShmem(idexpr):
    return ExprCall(ExprVar('LookupSharedMemory'), args=[ idexpr ])


def _makeForwardDeclForQClass(clsname, quals):
    fd = ForwardDecl(clsname, cls=1)
    if 0 == len(quals):
        return fd

    outerns = Namespace(quals[0])
    innerns = outerns
    for ns in quals[1:]:
        tmpns = Namespace(ns)
        innerns.addstmt(tmpns)
        innerns = tmpns

    innerns.addstmt(fd)
    return outerns

def _makeForwardDeclForActor(ptype, side):
    return _makeForwardDeclForQClass(_actorName(ptype.qname.baseid, side),
                                     ptype.qname.quals)

def _makeForwardDecl(type):
    return _makeForwardDeclForQClass(type.name(), type.qname.quals)


def _putInNamespaces(cxxthing, namespaces):
    """|namespaces| is in order [ outer, ..., inner ]"""
    if 0 == len(namespaces):  return cxxthing

    outerns = Namespace(namespaces[0].name)
    innerns = outerns
    for ns in namespaces[1:]:
        newns = Namespace(ns.name)
        innerns.addstmt(newns)
        innerns = newns
    innerns.addstmt(cxxthing)
    return outerns

def _sendPrefix(msgtype):
    """Prefix of the name of the C++ method that sends |msgtype|."""
    if msgtype.isRpc():
        return 'Call'
    return 'Send'

def _recvPrefix(msgtype):
    """Prefix of the name of the C++ method that handles |msgtype|."""
    if msgtype.isRpc():
        return 'Answer'
    return 'Recv'

def _flatTypeName(ipdltype):
    """Return a 'flattened' IPDL type name that can be used as an
identifier.
E.g., |Foo[]| --> |ArrayOfFoo|."""
    # NB: this logic depends heavily on what IPDL types are allowed to
    # be constructed; e.g., Foo[][] is disallowed.  needs to be kept in
    # sync with grammar.
    if ipdltype.isIPDL() and ipdltype.isArray():
        return 'ArrayOf'+ ipdltype.basetype.name()
    return ipdltype.name()


def _hasVisibleActor(ipdltype):
    """Return true iff a C++ decl of |ipdltype| would have an Actor* type.
For example: |Actor[]| would turn into |Array<ActorParent*>|, so this
function would return true for |Actor[]|."""
    return (ipdltype.isIPDL()
            and (ipdltype.isActor()
                 or (ipdltype.isArray()
                     and _hasVisibleActor(ipdltype.basetype))))

def _abortIfFalse(cond, msg):
    return StmtExpr(ExprCall(
        ExprVar('NS_ABORT_IF_FALSE'),
        [ cond, ExprLiteral.String(msg) ]))

def _runtimeAbort(msg):
    return StmtExpr(ExprCall(ExprVar('NS_RUNTIMEABORT'),
                                     [ ExprLiteral.String(msg) ]))

def _autoptr(T):
    return Type('nsAutoPtr', T=T)

def _autoptrGet(expr):
    return ExprCall(ExprSelect(expr, '.', 'get'))

def _autoptrForget(expr):
    return ExprCall(ExprSelect(expr, '.', 'forget'))

def _cxxArrayType(basetype, const=0, ref=0):
    return Type('InfallibleTArray', T=basetype, const=const, ref=ref)

def _callCxxArrayLength(arr):
    return ExprCall(ExprSelect(arr, '.', 'Length'))

def _callCxxArraySetLength(arr, lenexpr, sel='.'):
    return ExprCall(ExprSelect(arr, sel, 'SetLength'),
                    args=[ lenexpr ])

def _callCxxArrayInsertSorted(arr, elt):
    return ExprCall(ExprSelect(arr, '.', 'InsertElementSorted'),
                    args=[ elt ])

def _callCxxArrayRemoveSorted(arr, elt):
    return ExprCall(ExprSelect(arr, '.', 'RemoveElementSorted'),
                    args=[ elt ])

def _callCxxArrayClear(arr):
    return ExprCall(ExprSelect(arr, '.', 'Clear'))

def _cxxArrayHasElementSorted(arr, elt):
    return ExprBinary(
        ExprSelect(arr, '.', 'NoIndex'), '!=',
        ExprCall(ExprSelect(arr, '.', 'BinaryIndexOf'), args=[ elt ]))

def _otherSide(side):
    if side == 'child':  return 'parent'
    if side == 'parent':  return 'child'
    assert 0

def _ifLogging(stmts):
    iflogging = StmtIf(ExprCall(ExprVar('mozilla::ipc::LoggingEnabled')))
    iflogging.addifstmts(stmts)
    return iflogging

# We need the ASTs of structs and unions to generate pickling code for
# them, but the pickling codegen only has their type info.  This map
# allows the pickling code to get these ASTs given the type info.
_typeToAST = { }                        # [ Type -> Node ]

# XXX we need to remove these and install proper error handling
def _printErrorMessage(msg):
    if isinstance(msg, str):
        msg = ExprLiteral.String(msg)
    return StmtExpr(
        ExprCall(ExprVar('NS_ERROR'), args=[ msg ]))

def _printWarningMessage(msg):
    if isinstance(msg, str):
        msg = ExprLiteral.String(msg)
    return StmtExpr(
        ExprCall(ExprVar('NS_WARNING'), args=[ msg ]))

def _fatalError(msg):
    return StmtExpr(
        ExprCall(ExprVar('FatalError'), args=[ ExprLiteral.String(msg) ]))

def _killProcess(pid):
    return ExprCall(
        ExprVar('base::KillProcess'),
        args=[ pid,
               # XXX this is meaningless on POSIX
               ExprVar('base::PROCESS_END_KILLED_BY_USER'),
               ExprLiteral.FALSE ])

def _badTransition():
    # FIXME: make this a FatalError()
    return [ _printWarningMessage('bad state transition!') ]

# Results that IPDL-generated code returns back to *Channel code.
# Users never see these
class _Result:
    @staticmethod
    def Type():
        return Type('Result')

    Processed = ExprVar('MsgProcessed')
    NotKnown = ExprVar('MsgNotKnown')
    NotAllowed = ExprVar('MsgNotAllowed')
    PayloadError = ExprVar('MsgPayloadError')
    ProcessingError = ExprVar('MsgProcessingError')
    RouteError = ExprVar('MsgRouteError')
    ValuError = ExprVar('MsgValueError') # [sic]

# these |errfn*| are functions that generate code to be executed on an
# error, such as "bad actor ID".  each is given a Python string
# containing a description of the error

# used in user-facing Send*() methods
def errfnSend(msg, errcode=ExprLiteral.FALSE):
    return [
        _fatalError(msg),
        StmtReturn(errcode)
    ]

def errfnSendCtor(msg):  return errfnSend(msg, errcode=ExprLiteral.NULL)

# TODO should this error handling be strengthened for dtors?
def errfnSendDtor(msg):
    return [
        _printErrorMessage(msg),
        StmtReturn.FALSE
    ]

# used in |OnMessage*()| handlers that hand in-messages off to Recv*()
# interface methods
def errfnRecv(msg, errcode=_Result.ValuError):
    return [
        _fatalError(msg),
        StmtReturn(errcode)
    ]

# used in Read() methods
def errfnRead(msg):
    return [ StmtReturn.FALSE ]

def _destroyMethod():
    return ExprVar('ActorDestroy')

class _DestroyReason:
    @staticmethod
    def Type():  return Type('ActorDestroyReason')

    Deletion = ExprVar('Deletion')
    AncestorDeletion = ExprVar('AncestorDeletion')
    NormalShutdown = ExprVar('NormalShutdown')
    AbnormalShutdown = ExprVar('AbnormalShutdown')
    FailedConstructor = ExprVar('FailedConstructor')

##-----------------------------------------------------------------------------
## Intermediate representation (IR) nodes used during lowering

class _ConvertToCxxType(TypeVisitor):
    def __init__(self, side):  self.side = side

    def visitBuiltinCxxType(self, t):
        return Type(t.name())

    def visitImportedCxxType(self, t):
        return Type(t.name())

    def visitActorType(self, a):
        return Type(_actorName(a.protocol.name(), self.side), ptr=1)

    def visitStructType(self, s):
        return Type(s.name())

    def visitUnionType(self, u):
        return Type(u.name())

    def visitArrayType(self, a):
        basecxxtype = a.basetype.accept(self)
        return _cxxArrayType(basecxxtype)

    def visitShmemType(self, s):
        return Type(s.name())

    def visitProtocolType(self, p): assert 0
    def visitMessageType(self, m): assert 0
    def visitVoidType(self, v): assert 0
    def visitStateType(self, st): assert 0

def _cxxBareType(ipdltype, side):
    return ipdltype.accept(_ConvertToCxxType(side))

def _cxxRefType(ipdltype, side):
    t = _cxxBareType(ipdltype, side)
    t.ref = 1
    return t

def _cxxConstRefType(ipdltype, side):
    t = _cxxBareType(ipdltype, side)
    if ipdltype.isIPDL() and ipdltype.isActor():
        return t
    if ipdltype.isIPDL() and ipdltype.isShmem():
        t.ref = 1
        return t
    t.const = 1
    t.ref = 1
    return t

def _cxxPtrToType(ipdltype, side):
    t = _cxxBareType(ipdltype, side)
    if ipdltype.isIPDL() and ipdltype.isActor():
        t.ptr = 0
        t.ptrptr = 1
        return t
    t.ptr = 1
    return t

def _cxxConstPtrToType(ipdltype, side):
    t = _cxxBareType(ipdltype, side)
    if ipdltype.isIPDL() and ipdltype.isActor():
        t.ptr = 0
        t.ptrconstptr = 1
        return t
    t.const = 1
    t.ptrconst = 1
    return t

def _allocMethod(ptype):
    return ExprVar('Alloc'+ ptype.name())

def _deallocMethod(ptype):
    return ExprVar('Dealloc'+ ptype.name())

##
## A _HybridDecl straddles IPDL and C++ decls.  It knows which C++
## types correspond to which IPDL types, and it also knows how
## serialize and deserialize "special" IPDL C++ types.
##
class _HybridDecl:
    """A hybrid decl stores both an IPDL type and all the C++ type
info needed by later passes, along with a basic name for the decl."""
    def __init__(self, ipdltype, name):
        self.ipdltype = ipdltype
        self.name = name
        self.idnum = 0

    def var(self):
        return ExprVar(self.name)

    def bareType(self, side):
        """Return this decl's unqualified C++ type."""
        return _cxxBareType(self.ipdltype, side)

    def refType(self, side):
        """Return this decl's C++ type as a 'reference' type, which is not
necessarily a C++ reference."""
        return _cxxRefType(self.ipdltype, side)

    def constRefType(self, side):
        """Return this decl's C++ type as a const, 'reference' type."""
        return _cxxConstRefType(self.ipdltype, side)

    def ptrToType(self, side):
        return _cxxPtrToType(self.ipdltype, side)

    def constPtrToType(self, side):
        return _cxxConstPtrToType(self.ipdltype, side)

    def inType(self, side):
        """Return this decl's C++ Type with inparam semantics."""
        if self.ipdltype.isIPDL() and self.ipdltype.isActor():
            return self.bareType(side)
        return self.constRefType(side)

    def outType(self, side):
        """Return this decl's C++ Type with outparam semantics."""
        t = self.bareType(side)
        if self.ipdltype.isIPDL() and self.ipdltype.isActor():
            t.ptr = 0;  t.ptrptr = 1
            return t
        t.ptr = 1
        return t

##--------------------------------------------------

class HasFQName:
    def fqClassName(self):
        return self.decl.type.fullname()

class _CompoundTypeComponent(_HybridDecl):
    def __init__(self, ipdltype, name, side, ct):
        _HybridDecl.__init__(self, ipdltype, name)
        self.side = side
        self.special = _hasVisibleActor(ipdltype)
        self.recursive = ct.decl.type.mutuallyRecursiveWith(ipdltype)

    def internalType(self):
        if self.recursive:
            return self.ptrToType()
        else:
            return self.bareType()

    # @override the following methods to pass |self.side| instead of
    # forcing the caller to remember which side we're declared to
    # represent.
    def bareType(self, side=None):
        return _HybridDecl.bareType(self, self.side)
    def refType(self, side=None):
        return _HybridDecl.refType(self, self.side)
    def constRefType(self, side=None):
        return _HybridDecl.constRefType(self, self.side)
    def ptrToType(self, side=None):
        return _HybridDecl.ptrToType(self, self.side)
    def constPtrToType(self, side=None):
        return _HybridDecl.constPtrToType(self, self.side)
    def inType(self, side=None):
        return _HybridDecl.inType(self, self.side)


class StructDecl(ipdl.ast.StructDecl, HasFQName):
    @staticmethod
    def upgrade(structDecl):
        assert isinstance(structDecl, ipdl.ast.StructDecl)
        structDecl.__class__ = StructDecl
        return structDecl

class _StructField(_CompoundTypeComponent):
    def __init__(self, ipdltype, name, sd, side=None):
        fname = name
        special = _hasVisibleActor(ipdltype)
        if special:
            fname += side.title()

        _CompoundTypeComponent.__init__(self, ipdltype, fname, side, sd)

    def getMethod(self, thisexpr=None, sel='.'):
        meth = self.var()
        if thisexpr is not None:
            return ExprSelect(thisexpr, sel, meth.name)
        return meth

    def initExpr(self, thisexpr):
        expr = ExprCall(self.getMethod(thisexpr=thisexpr))
        if self.ipdltype.isIPDL() and self.ipdltype.isActor():
            expr = ExprCast(expr, self.bareType(), const=1)
        return expr

    def refExpr(self, thisexpr=None):
        ref = self.memberVar()
        if thisexpr is not None:
            ref = ExprSelect(thisexpr, '.', ref.name)
        if self.recursive:
            ref = ExprDeref(ref)
        return ref

    def constRefExpr(self, thisexpr=None):
        # sigh, gross hack
        refexpr = self.refExpr(thisexpr)
        if 'Shmem' == self.ipdltype.name():
            refexpr = ExprCast(refexpr, Type('Shmem', ref=1), const=1)
        return refexpr

    def argVar(self):
        return ExprVar('_'+ self.name)

    def memberVar(self):
        return ExprVar(self.name + '_')

    def initStmts(self):
        if self.recursive:
            return [ StmtExpr(ExprAssn(self.memberVar(),
                                       ExprNew(self.bareType()))) ]
        else:
            return []

    def destructStmts(self):
        if self.recursive:
            return [ StmtExpr(ExprDelete(self.memberVar())) ]
        else:
            return []


class UnionDecl(ipdl.ast.UnionDecl, HasFQName):
    def callType(self, var=None):
        func = ExprVar('type')
        if var is not None:
            func = ExprSelect(var, '.', func.name)
        return ExprCall(func)

    @staticmethod
    def upgrade(unionDecl):
        assert isinstance(unionDecl, ipdl.ast.UnionDecl)
        unionDecl.__class__ = UnionDecl
        return unionDecl


class _UnionMember(_CompoundTypeComponent):
    """Not in the AFL sense, but rather a member (e.g. |int;|) of an
IPDL union type."""
    def __init__(self, ipdltype, ud, side=None, other=None):
        flatname = _flatTypeName(ipdltype)
        special = _hasVisibleActor(ipdltype)
        if special:
            flatname += side.title()

        _CompoundTypeComponent.__init__(self, ipdltype, 'V'+ flatname, side, ud)
        self.flattypename = flatname
        if special:
            if other is not None:
                self.other = other
            else:
                self.other = _UnionMember(ipdltype, ud, _otherSide(side), self)

    def enum(self):
        return 'T' + self.flattypename

    def pqEnum(self):
        return self.ud.name +'::'+ self.enum()

    def enumvar(self):
        return ExprVar(self.enum())

    def unionType(self):
        """Type used for storage in generated C union decl."""
        if self.recursive:
            return self.ptrToType()
        else:
            return TypeArray(Type('char'), ExprSizeof(self.internalType()))

    def unionValue(self):
        # NB: knows that Union's storage C union is named |mValue|
        return ExprSelect(ExprVar('mValue'), '.', self.name)

    def typedef(self):
        return self.flattypename +'__tdef'

    def callGetConstPtr(self):
        """Return an expression of type self.constptrToSelfType()"""
        return ExprCall(ExprVar(self.getConstPtrName()))

    def callGetPtr(self):
        """Return an expression of type self.ptrToSelfType()"""
        return ExprCall(ExprVar(self.getPtrName()))

    def callOperatorEq(self, rhs):
        if self.ipdltype.isIPDL() and self.ipdltype.isActor():
            rhs = ExprCast(rhs, self.bareType(), const=1)
        return ExprAssn(ExprDeref(self.callGetPtr()), rhs)

    def callCtor(self, expr=None):
        assert not isinstance(expr, list)
        
        if expr is None:
            args = None
        elif self.ipdltype.isIPDL() and self.ipdltype.isActor():
            args = [ ExprCast(expr, self.bareType(), const=1) ]
        else:
            args = [ expr ]

        if self.recursive:
            return ExprAssn(self.callGetPtr(),
                            ExprNew(self.bareType(self.side),
                                    args=args))
        else:
            return ExprNew(self.bareType(self.side),
                           args=args,
                           newargs=[ self.callGetPtr() ])

    def callDtor(self):
        if self.recursive:
            return ExprDelete(self.callGetPtr())
        else:
            return ExprCall(
                ExprSelect(self.callGetPtr(), '->', '~'+ self.typedef()))

    def getTypeName(self): return 'get_'+ self.flattypename
    def getConstTypeName(self): return 'get_'+ self.flattypename

    def getOtherTypeName(self): return 'get_'+ self.otherflattypename

    def getPtrName(self): return 'ptr_'+ self.flattypename
    def getConstPtrName(self): return 'constptr_'+ self.flattypename

    def ptrToSelfExpr(self):
        """|*ptrToSelfExpr()| has type |self.bareType()|"""
        v = self.unionValue()
        if self.recursive:
            return v
        else:
            return ExprCast(ExprAddrOf(v), self.ptrToType(), reinterpret=1)

    def constptrToSelfExpr(self):
        """|*constptrToSelfExpr()| has type |self.constType()|"""
        v = self.unionValue()
        if self.recursive:
            return v
        return ExprCast(ExprAddrOf(v), self.constPtrToType(), reinterpret=1)

    def ptrToInternalType(self):
        t = self.ptrToType()
        if self.recursive:
            t.ref = 1
        return t

    def defaultValue(self):
        if self.ipdltype.isIPDL() and self.ipdltype.isActor():
            return ExprCast(ExprLiteral.NULL, self.bareType(), static=1)
        # XXX sneaky here, maybe need ExprCtor()?
        return ExprCall(self.bareType())

    def getConstValue(self):
        v = ExprDeref(self.callGetConstPtr())
        # sigh
        if 'Shmem' == self.ipdltype.name():
            v = ExprCast(v, Type('Shmem', ref=1), const=1)
        return v

##--------------------------------------------------

class MessageDecl(ipdl.ast.MessageDecl):
    def baseName(self):
        return self.name
    
    def recvMethod(self):
        name = _recvPrefix(self.decl.type) + self.baseName()
        if self.decl.type.isCtor():
            name += 'Constructor'
        return ExprVar(name)

    def sendMethod(self):
        name = _sendPrefix(self.decl.type) + self.baseName()
        if self.decl.type.isCtor():
            name += 'Constructor'
        return ExprVar(name)

    def hasReply(self):
        return (self.decl.type.hasReply()
                or self.decl.type.isCtor()
                or self.decl.type.isDtor())

    def msgClass(self):
        return 'Msg_%s'% (self.decl.progname)

    def prettyMsgName(self, pfx=''):
        return pfx + self.msgClass()

    def pqMsgClass(self):
        return '%s::%s'% (self.namespace, self.msgClass())

    def msgCast(self, msgexpr):
        return ExprCast(msgexpr, self.msgCxxType(const=1, ptr=1), static=1)

    def msgCxxType(self, const=0, ref=0, ptr=0):
        return Type(self.pqMsgClass(), const=const, ref=ref, ptr=ptr)

    def msgId(self):  return self.msgClass()+ '__ID'
    def pqMsgId(self):
        return '%s::%s'% (self.namespace, self.msgId())

    def replyClass(self):
        return 'Reply_%s'% (self.decl.progname)

    def pqReplyClass(self):
        return '%s::%s'% (self.namespace, self.replyClass())

    def replyCast(self, replyexpr):
        return ExprCast(replyexpr, Type(self.pqReplyClass(), const=1, ptr=1),
                        static=1)

    def replyId(self):  return self.replyClass()+ '__ID'
    def pqReplyId(self):
        return '%s::%s'% (self.namespace, self.replyId())

    def prettyReplyName(self, pfx=''):
        return pfx + self.replyClass()

    def actorDecl(self):
        return self.params[0]

    def makeCxxParams(self, paramsems='in', returnsems='out',
                      side=None, implicit=1):
        """Return a list of C++ decls per the spec'd configuration.
|params| and |returns| is the C++ semantics of those: 'in', 'out', or None."""

        def makeDecl(d, sems):
            if sems is 'in':
                return Decl(d.inType(side), d.name)
            elif sems is 'out':
                return Decl(d.outType(side), d.name)
            else: assert 0

        cxxparams = [ ]
        if paramsems is not None:
            cxxparams.extend([ makeDecl(d, paramsems) for d in self.params ])

        if returnsems is not None:
            cxxparams.extend([ makeDecl(r, returnsems) for r in self.returns ])

        if not implicit and self.decl.type.hasImplicitActorParam():
            cxxparams = cxxparams[1:]

        return cxxparams

    def makeCxxArgs(self, params=1, retsems='out', retcallsems='out',
                    implicit=1):
        assert not implicit or params     # implicit => params
        assert not retcallsems or retsems # retcallsems => returnsems
        cxxargs = [ ]

        if params:
            cxxargs.extend([ p.var() for p in self.params ])

        for ret in self.returns:
            if retsems is 'in':
                if retcallsems is 'in':
                    cxxargs.append(ret.var())
                elif retcallsems is 'out':
                    cxxargs.append(ExprAddrOf(ret.var()))
                else: assert 0
            elif retsems is 'out':
                if retcallsems is 'in':
                    cxxargs.append(ExprDeref(ret.var()))
                elif retcallsems is 'out':
                    cxxargs.append(ret.var())
                else: assert 0

        if not implicit:
            assert self.decl.type.hasImplicitActorParam()
            cxxargs = cxxargs[1:]

        return cxxargs


    @staticmethod
    def upgrade(messageDecl):
        assert isinstance(messageDecl, ipdl.ast.MessageDecl)
        if messageDecl.decl.type.hasImplicitActorParam():
            messageDecl.params.insert(
                0,
                _HybridDecl(
                    ipdl.type.ActorType(
                        messageDecl.decl.type.constructedType()),
                    'actor'))
        messageDecl.__class__ = MessageDecl
        return messageDecl

##--------------------------------------------------
def _semsToChannelParts(sems):
    if ipdl.ast.ASYNC == sems:   channel = 'AsyncChannel'
    elif ipdl.ast.SYNC == sems:  channel = 'SyncChannel'
    elif ipdl.ast.RPC == sems:   channel = 'RPCChannel'
    return [ 'mozilla', 'ipc', channel ]

def _semsToListener(sems):
    return { ipdl.ast.ASYNC: 'AsyncListener',
             ipdl.ast.SYNC: 'SyncListener',
             ipdl.ast.RPC: 'RPCListener' }[sems]

def _usesShmem(p):
    for md in p.messageDecls:
        for param in md.inParams:
            if ipdl.type.hasshmem(param.type):
                return True
        for ret in md.outParams:
            if ipdl.type.hasshmem(ret.type):
                return True
    return False

def _subtreeUsesShmem(p):
    if _usesShmem(p):
        return True

    ptype = p.decl.type
    for mgd in ptype.manages:
        if ptype is not mgd:
            if _subtreeUsesShmem(mgd._p):
                return True
    return False


class Protocol(ipdl.ast.Protocol):
    def cxxTypedefs(self):
        return self.decl.cxxtypedefs

    def sendSems(self):
        return self.decl.type.toplevel().sendSemantics

    def channelName(self):
        return '::'.join(_semsToChannelParts(self.sendSems()))

    def channelSel(self):
        if self.decl.type.isToplevel():  return '.'
        return '->'

    def channelType(self):
        return Type('Channel', ptr=not self.decl.type.isToplevel())

    def channelHeaderFile(self):
        return '/'.join(_semsToChannelParts(self.sendSems())) +'.h'

    def listenerName(self):
        return _semsToListener(self.sendSems())

    def fqListenerName(self):
        return self.channelName() +'::'+ _semsToListener(self.sendSems())

    def managerInterfaceType(self, ptr=0):
        return Type('mozilla::ipc::IProtocolManager',
                    ptr=ptr,
                    T=Type(self.fqListenerName()))

    def _ipdlmgrtype(self):
        assert 1 == len(self.decl.type.managers)
        for mgr in self.decl.type.managers:  return mgr

    def managerActorType(self, side, ptr=0):
        return Type(_actorName(self._ipdlmgrtype().name(), side),
                    ptr=ptr)

    def managerMethod(self, actorThis=None):
        _ = self._ipdlmgrtype()
        if actorThis is not None:
            return ExprSelect(actorThis, '->', 'Manager')
        return ExprVar('Manager');

    def stateMethod(self):
        return ExprVar('state');

    def registerMethod(self):
        return ExprVar('Register')

    def registerIDMethod(self):
        return ExprVar('RegisterID')

    def lookupIDMethod(self):
        return ExprVar('Lookup')

    def unregisterMethod(self, actorThis=None):
        if actorThis is not None:
            return ExprSelect(actorThis, '->', 'Unregister')
        return ExprVar('Unregister')

    def removeManageeMethod(self):
        return ExprVar('RemoveManagee')

    def createSharedMemory(self):
        return ExprVar('CreateSharedMemory')

    def adoptSharedMemory(self):
        return ExprVar('AdoptSharedMemory')
 
    def lookupSharedMemory(self):
        return ExprVar('LookupSharedMemory')

    def isTrackingSharedMemory(self):
        return ExprVar('IsTrackingSharedMemory')

    def destroySharedMemory(self):
        return ExprVar('DestroySharedMemory')

    def otherProcessMethod(self):
        return ExprVar('OtherProcess')

    def processingErrorVar(self):
        assert self.decl.type.isToplevel()
        return ExprVar('ProcessingError')

    def shouldContinueFromTimeoutVar(self):
        assert self.decl.type.isToplevel()
        return ExprVar('ShouldContinueFromReplyTimeout')

    def enteredCxxStackVar(self):
        assert self.decl.type.isToplevel()
        return ExprVar('EnteredCxxStack')

    def exitedCxxStackVar(self):
        assert self.decl.type.isToplevel()
        return ExprVar('ExitedCxxStack')

    def enteredCallVar(self):
        assert self.decl.type.isToplevel()
        return ExprVar('EnteredCall')

    def exitedCallVar(self):
        assert self.decl.type.isToplevel()
        return ExprVar('ExitedCall')

    def onCxxStackVar(self):
        assert self.decl.type.isToplevel()
        return ExprVar('IsOnCxxStack')

    def nextActorIdExpr(self, side):
        assert self.decl.type.isToplevel()
        if side is 'parent':   op = '++'
        elif side is 'child':  op = '--'
        else: assert 0
        return ExprPrefixUnop(self.lastActorIdVar(), op)

    def actorIdInit(self, side):
        assert self.decl.type.isToplevel()

        # parents go up from FREED, children go down from NULL
        if side is 'parent':  return _FREED_ACTOR_ID
        elif side is 'child': return _NULL_ACTOR_ID
        else: assert 0

    # an actor's C++ private variables
    def lastActorIdVar(self):
        assert self.decl.type.isToplevel()
        return ExprVar('mLastRouteId')

    def actorMapVar(self):
        assert self.decl.type.isToplevel()
        return ExprVar('mActorMap')

    def channelVar(self, actorThis=None):
        if actorThis is not None:
            return ExprSelect(actorThis, '->', 'mChannel')
        return ExprVar('mChannel')

    def channelForSubactor(self):
        if self.decl.type.isToplevel():
            return ExprAddrOf(self.channelVar())
        return self.channelVar()

    def routingId(self, actorThis=None):
        if self.decl.type.isToplevel():
            return ExprVar('MSG_ROUTING_CONTROL')
        if actorThis is not None:
            return ExprSelect(actorThis, '->', self.idVar().name)
        return self.idVar()

    def idVar(self):
        assert not self.decl.type.isToplevel()
        return ExprVar('mId')

    def stateVar(self):
        return ExprVar('mState')

    def fqStateType(self):
        return Type(self.decl.type.name() +'::State')

    def startState(self):
        return _startState(self.decl.type)

    def nullState(self):
        return _nullState(self.decl.type)

    def deadState(self):
        return _deadState(self.decl.type)

    def managerVar(self, thisexpr=None):
        assert thisexpr is not None or not self.decl.type.isToplevel()
        mvar = ExprVar('mManager')
        if thisexpr is not None:
            mvar = ExprSelect(thisexpr, '->', mvar.name)
        return mvar

    def otherProcessVar(self):
        assert self.decl.type.isToplevel()
        return ExprVar('mOtherProcess')

    def managedCxxType(self, actortype, side):
        assert self.decl.type.isManagerOf(actortype)
        return Type(_actorName(actortype.name(), side), ptr=1)

    def managedMethod(self, actortype, side):
        assert self.decl.type.isManagerOf(actortype)
        return ExprVar('Managed'+  _actorName(actortype.name(), side))

    def managedVar(self, actortype, side):
        assert self.decl.type.isManagerOf(actortype)
        return ExprVar('mManaged'+ _actorName(actortype.name(), side))

    def managedVarType(self, actortype, side, const=0, ref=0):
        assert self.decl.type.isManagerOf(actortype)
        return _cxxArrayType(self.managedCxxType(actortype, side),
                             const=const, ref=ref)

    def managerArrayExpr(self, thisvar, side):
        """The member var my manager keeps of actors of my type."""
        assert self.decl.type.isManaged()
        return ExprSelect(
            ExprCall(self.managerMethod(thisvar)),
            '->', 'mManaged'+ _actorName(self.decl.type.name(), side))

    # shmem stuff
    def shmemMapType(self):
        assert self.decl.type.isToplevel()
        return Type('IDMap', T=_rawShmemType())

    def shmemIteratorType(self):
        assert self.decl.type.isToplevel()
        # XXX breaks abstractions
        return Type('IDMap<SharedMemory>::const_iterator')

    def shmemMapVar(self):
        assert self.decl.type.isToplevel()
        return ExprVar('mShmemMap')

    def lastShmemIdVar(self):
        assert self.decl.type.isToplevel()
        return ExprVar('mLastShmemId')

    def shmemIdInit(self, side):
        assert self.decl.type.isToplevel()
        # use the same scheme for shmem IDs as actor IDs
        if side is 'parent':  return _FREED_ACTOR_ID
        elif side is 'child': return _NULL_ACTOR_ID
        else: assert 0

    def nextShmemIdExpr(self, side):
        assert self.decl.type.isToplevel()
        if side is 'parent':   op = '++'
        elif side is 'child':  op = '--'
        return ExprPrefixUnop(self.lastShmemIdVar(), op)

    def removeShmemId(self, idexpr):
        return ExprCall(ExprSelect(self.shmemMapVar(), '.', 'Remove'),
                        args=[ idexpr ])

    # XXX this is sucky, fix
    def usesShmem(self):
        return _usesShmem(self)

    def subtreeUsesShmem(self):
        return _subtreeUsesShmem(self)

    @staticmethod
    def upgrade(protocol):
        assert isinstance(protocol, ipdl.ast.Protocol)
        protocol.__class__ = Protocol
        return protocol

##-----------------------------------------------------------------------------

class _DecorateWithCxxStuff(ipdl.ast.Visitor):
    """Phase 1 of lowering: decorate the IPDL AST with information
relevant to C++ code generation.

This pass results in an AST that is a poor man's "IR"; in reality, a
"hybrid" AST mainly consisting of IPDL nodes with new C++ info along
with some new IPDL/C++ nodes that are tuned for C++ codegen."""

    def __init__(self):
        # the set of typedefs that allow generated classes to
        # reference known C++ types by their "short name" rather than
        # fully-qualified name. e.g. |Foo| rather than |a::b::Foo|.
        self.typedefs = [ 
            Typedef(Type('mozilla::ipc::ActorHandle'), 'ActorHandle')
        ]
        self.protocolName = None

    def visitProtocol(self, pro):
        self.protocolName = pro.name
        pro.decl.cxxtypedefs = self.typedefs
        Protocol.upgrade(pro)
        return ipdl.ast.Visitor.visitProtocol(self, pro)


    def visitUsingStmt(self, using):
        if using.decl.fullname is not None:
            self.typedefs.append(Typedef(Type(using.decl.fullname),
                                         using.decl.shortname))

    def visitStructDecl(self, sd):
        sd.decl.special = 0
        newfields = [ ]
        for f in sd.fields:
            ftype = f.decl.type
            if _hasVisibleActor(ftype):
                sd.decl.special = 1
                # if ftype has a visible actor, we need both
                # |ActorParent| and |ActorChild| fields
                newfields.append(_StructField(ftype, f.name, sd, side='parent'))
                newfields.append(_StructField(ftype, f.name, sd, side='child'))
            else:
                newfields.append(_StructField(ftype, f.name, sd))
        sd.fields = newfields
        StructDecl.upgrade(sd)
        _typeToAST[sd.decl.type] = sd

        if sd.decl.fullname is not None:
            self.typedefs.append(Typedef(Type(sd.fqClassName()), sd.name))


    def visitUnionDecl(self, ud):
        ud.decl.special = 0
        newcomponents = [ ]
        for ctype in ud.decl.type.components:
            if _hasVisibleActor(ctype):
                ud.decl.special = 1
                # if ctype has a visible actor, we need both
                # |ActorParent| and |ActorChild| union members
                newcomponents.append(_UnionMember(ctype, ud, side='parent'))
                newcomponents.append(_UnionMember(ctype, ud, side='child'))
            else:
                newcomponents.append(_UnionMember(ctype, ud))
        ud.components = newcomponents
        UnionDecl.upgrade(ud)
        _typeToAST[ud.decl.type] = ud

        if ud.decl.fullname is not None:
            self.typedefs.append(Typedef(Type(ud.fqClassName()), ud.name))


    def visitDecl(self, decl):
        return _HybridDecl(decl.type, decl.progname)

    def visitMessageDecl(self, md):
        md.namespace = self.protocolName
        md.params = [ param.accept(self) for param in md.inParams ]
        md.returns = [ ret.accept(self) for ret in md.outParams ]
        MessageDecl.upgrade(md)

    def visitTransitionStmt(self, ts):
        name = ts.state.decl.progname
        ts.state.decl.cxxname = name
        ts.state.decl.cxxenum = ExprVar(self.protocolName +'::'+ name)

##-----------------------------------------------------------------------------

class _GenerateProtocolHeader(ipdl.ast.Visitor):
    '''Creates a header containing code common to both the parent and
child actors.'''
    def __init__(self):
        self.protocol = None     # protocol we're generating a class for
        self.file = None         # File stuff is stuck in
        self.structUnionDefns = []

    def lower(self, tu, outcxxfile):
        self.protocol = tu.protocol
        self.file = outcxxfile
        tu.accept(self)

    def visitTranslationUnit(self, tu):
        f = self.file

        f.addthing(Whitespace('''//
// Automatically generated by the IPDL compiler.
// Edit at your own risk
//

'''))
        f.addthings(_includeGuardStart(f))
        f.addthing(Whitespace.NL)

        ipdl.ast.Visitor.visitTranslationUnit(self, tu)

        f.addthings(self.structUnionDefns)

        f.addthing(Whitespace.NL)
        f.addthings(_includeGuardEnd(f))


    def visitCxxInclude(self, inc):
        self.file.addthing(CppDirective('include', '"'+ inc.file +'"'))

    def processStructOrUnionClass(self, su, which, forwarddecls, cls):
        clsdecl, methoddefns = _splitClassDeclDefn(cls, inlinedefns=1)
        
        self.file.addthings(
            [  Whitespace.NL ]
            + forwarddecls
            + [ Whitespace("""
//-----------------------------------------------------------------------------
// Declaration of the IPDL type |%s %s|
//
"""% (which, su.name)),
                _putInNamespaces(clsdecl, su.namespaces),
            ])

        self.structUnionDefns.extend([
            Whitespace("""
//-----------------------------------------------------------------------------
// Method definitions for the IPDL type |%s %s|
//
"""% (which, su.name)),
            _putInNamespaces(methoddefns, su.namespaces),
        ])

    def visitStructDecl(self, sd):
        return self.processStructOrUnionClass(sd, 'struct',
                                              *_generateCxxStruct(sd))

    def visitUnionDecl(self, ud):
        return self.processStructOrUnionClass(ud, 'union',
                                              *_generateCxxUnion(ud))

    def visitProtocol(self, p):
        self.file.addthing(Whitespace("""
//-----------------------------------------------------------------------------
// Code common to %sChild and %sParent
//
"""% (p.name, p.name)))

        # construct the namespace into which we'll stick all our decls
        ns = Namespace(self.protocol.name)
        self.file.addthing(_putInNamespaces(ns, p.namespaces))
        ns.addstmt(Whitespace.NL)

        # state information
        stateenum = TypeEnum('State')
        # NB: __Dead is the first state on purpose, so that it has
        # value '0'
        stateenum.addId(_deadState().name)
        stateenum.addId(_nullState().name)
        stateenum.addId(_errorState().name)
        for ts in p.transitionStmts:
            stateenum.addId(ts.state.decl.cxxname)
        if len(p.transitionStmts):
            startstate = p.transitionStmts[0].state.decl.cxxname
        else:
            startstate = _nullState().name
        stateenum.addId(_startState().name, startstate)

        ns.addstmts([ StmtDecl(Decl(stateenum,'')), Whitespace.NL ])

        # spit out message type enum and classes
        msgenum = TypeEnum('MessageType')
        msgstart = _messageStartName(self.protocol.decl.type) +' << 16'
        msgenum.addId(self.protocol.name + 'Start', msgstart)
        msgenum.addId(self.protocol.name +'PreStart', '('+ msgstart +') - 1')

        for md in p.messageDecls:
            msgenum.addId(md.msgId())
            if md.hasReply():
                msgenum.addId(md.replyId())

        msgenum.addId(self.protocol.name +'End')
        ns.addstmts([ StmtDecl(Decl(msgenum, '')), Whitespace.NL ])

        ns.addstmts([ self.genTransitionFunc(), Whitespace.NL ])

        typedefs = self.protocol.decl.cxxtypedefs
        for md in p.messageDecls:
            ns.addstmts([
                _generateMessageClass(md.msgClass(), md.msgId(),
                                      typedefs, md.prettyMsgName(p.name+'::')),
                Whitespace.NL ])
            if md.hasReply():
                ns.addstmts([
                    _generateMessageClass(
                        md.replyClass(), md.replyId(),
                        typedefs, md.prettyReplyName(p.name+'::')),
                    Whitespace.NL ])

        ns.addstmts([ Whitespace.NL, Whitespace.NL ])


    def genTransitionFunc(self):
        ptype = self.protocol.decl.type
        usesend, sendvar = set(), ExprVar('__Send')
        userecv, recvvar = set(), ExprVar('__Recv')
        
        def sameTrigger(trigger, actionexpr):
            if trigger is ipdl.ast.SEND or trigger is ipdl.ast.CALL:
                usesend.add('yes')
                return ExprBinary(sendvar, '==', actionexpr)
            else:
                userecv.add('yes')
                return ExprBinary(recvvar, '==',
                                  actionexpr)

        def stateEnum(s):
            if s is ipdl.ast.State.DEAD:
                return _deadState()
            else:
                return ExprVar(s.decl.cxxname)

        # bool Transition(State from, Trigger trigger, State* next)
        fromvar = ExprVar('from')
        triggervar = ExprVar('trigger')
        nextvar = ExprVar('next')
        msgexpr = ExprSelect(triggervar, '.', 'mMsg')
        actionexpr = ExprSelect(triggervar, '.', 'mAction')

        transitionfunc = MethodDefn(MethodDecl(
            'Transition',
            params=[ Decl(Type('State'), fromvar.name),
                     Decl(Type('mozilla::ipc::Trigger'), triggervar.name),
                     Decl(Type('State', ptr=1), nextvar.name) ],
            ret=Type.BOOL,
            inline=1))

        fromswitch = StmtSwitch(fromvar)

        for ts in self.protocol.transitionStmts:
            msgswitch = StmtSwitch(msgexpr)

            msgToTransitions = { }

            for t in ts.transitions:
                msgid = t.msg._md.msgId()

                ifsametrigger = StmtIf(sameTrigger(t.trigger, actionexpr))
                # FIXME multi-out states
                for nextstate in t.toStates: break
                ifsametrigger.addifstmts([
                    StmtExpr(ExprAssn(ExprDeref(nextvar),
                                      stateEnum(nextstate))),
                    StmtReturn(ExprLiteral.TRUE)
                ])

                transitions = msgToTransitions.get(msgid, [ ])
                transitions.append(ifsametrigger)
                msgToTransitions[msgid] = transitions

            for msgid, transitions in msgToTransitions.iteritems():
                block = Block()
                block.addstmts(transitions +[ StmtBreak() ])
                msgswitch.addcase(CaseLabel(msgid), block)

            msgblock = Block()
            msgblock.addstmts([
                msgswitch,
                StmtBreak()
            ])
            fromswitch.addcase(CaseLabel(ts.state.decl.cxxname), msgblock)

        # special cases for Null and Error
        nullerrorblock = Block()
        if ptype.hasDelete:
            ifdelete = StmtIf(ExprBinary(_deleteId(), '==', msgexpr))
            ifdelete.addifstmts([
                StmtExpr(ExprAssn(ExprDeref(nextvar), _deadState())),
                StmtReturn(ExprLiteral.TRUE) ])
            nullerrorblock.addstmt(ifdelete)
        nullerrorblock.addstmt(
            StmtReturn(ExprBinary(_nullState(), '==', fromvar)))
        fromswitch.addfallthrough(CaseLabel(_nullState().name))
        fromswitch.addcase(CaseLabel(_errorState().name), nullerrorblock)

        # special case for Dead
        deadblock = Block()
        deadblock.addstmts([
            _runtimeAbort('__delete__()d actor'),
            StmtReturn(ExprLiteral.FALSE) ])
        fromswitch.addcase(CaseLabel(_deadState().name), deadblock)

        unreachedblock = Block()
        unreachedblock.addstmts([
            _runtimeAbort('corrupted actor state'),
            StmtReturn(ExprLiteral.FALSE) ])
        fromswitch.addcase(DefaultLabel(), unreachedblock)

        if usesend:
            transitionfunc.addstmt(
                StmtDecl(Decl(Type('int32', const=1), sendvar.name),
                         init=ExprVar('mozilla::ipc::Trigger::Send')))
        if userecv:
            transitionfunc.addstmt(
                StmtDecl(Decl(Type('int32', const=1), recvvar.name),
                         init=ExprVar('mozilla::ipc::Trigger::Recv')))
        if usesend or userecv:
            transitionfunc.addstmt(Whitespace.NL)

        transitionfunc.addstmts([
            fromswitch,
            # all --> Error transitions break to here
            StmtExpr(ExprAssn(ExprDeref(nextvar), _errorState())),
            StmtReturn(ExprLiteral.FALSE)
        ])
        return transitionfunc

##--------------------------------------------------

def _generateMessageClass(clsname, msgid, typedefs, prettyName):
    cls = Class(name=clsname, inherits=[ Inherit(Type('IPC::Message')) ])
    cls.addstmt(Label.PRIVATE)
    cls.addstmts(typedefs)
    cls.addstmt(Whitespace.NL)

    cls.addstmt(Label.PUBLIC)

    idenum = TypeEnum()
    idenum.addId('ID', msgid)
    cls.addstmt(StmtDecl(Decl(idenum, '')))

    # make the message constructor
    ctor = ConstructorDefn(
        ConstructorDecl(clsname),
        memberinits=[ ExprMemberInit(ExprVar('IPC::Message'),
                                     [ ExprVar('MSG_ROUTING_NONE'),
                                       ExprVar('ID'),
                                       ExprVar('PRIORITY_NORMAL'),
                                       ExprLiteral.String(prettyName) ]) ])
    cls.addstmts([ ctor, Whitespace.NL ])

    # generate a logging function
    # 'pfx' will be something like "[FooParent] sent"
    pfxvar = ExprVar('__pfx')
    outfvar = ExprVar('__outf')
    logger = MethodDefn(MethodDecl(
        'Log',
        params=([ Decl(Type('std::string', const=1, ref=1), pfxvar.name),
                  Decl(Type('FILE', ptr=True), outfvar.name) ]),
        const=1))
    # TODO/cjones: allow selecting what information is printed to 
    # the log
    msgvar = ExprVar('__logmsg')
    logger.addstmt(StmtDecl(Decl(Type('std::string'), msgvar.name)))

    def appendToMsg(thing):
        return StmtExpr(ExprCall(ExprSelect(msgvar, '.', 'append'),
                                 args=[ thing ]))
    logger.addstmts([
        StmtExpr(ExprCall(
            ExprVar('StringAppendF'),
            args=[ ExprAddrOf(msgvar),
                   ExprLiteral.String('[time:%" PRId64 "]'),
                   ExprCall(ExprVar('PR_Now')) ])),
        appendToMsg(pfxvar),
        appendToMsg(ExprLiteral.String(clsname +'(')),
        Whitespace.NL
    ])

    # TODO turn this back on when string stuff is sorted

    logger.addstmt(appendToMsg(ExprLiteral.String('[TODO])\\n')))

    # and actually print the log message
    logger.addstmt(StmtExpr(ExprCall(
        ExprVar('fputs'),
        args=[ ExprCall(ExprSelect(msgvar, '.', 'c_str')), outfvar ])))

    cls.addstmt(logger)

    return cls

##--------------------------------------------------

class _ComputeTypeDeps(TypeVisitor):
    '''Pass that gathers the C++ types that a particular IPDL type
(recursively) depends on.  There are two kinds of dependencies: (i)
types that need forward declaration; (ii) types that need a |using|
stmt.  Some types generate both kinds.'''

    def __init__(self, fortype):
        ipdl.type.TypeVisitor.__init__(self)
        self.usingTypedefs = [ ]
        self.forwardDeclStmts = [ ]
        self.fortype = fortype

    def maybeTypedef(self, fqname, name):
        if fqname != name:
            self.usingTypedefs.append(Typedef(Type(fqname), name))
        
    def visitBuiltinCxxType(self, t):
        if t in self.visited: return
        self.visited.add(t)
        self.maybeTypedef(t.fullname(), t.name())

    def visitImportedCxxType(self, t):
        if t in self.visited: return
        self.visited.add(t)
        self.maybeTypedef(t.fullname(), t.name())

    def visitActorType(self, t):
        if t in self.visited: return
        self.visited.add(t)

        fqname, name = t.fullname(), t.name()

        self.maybeTypedef(_actorName(fqname, 'Parent'),
                          _actorName(name, 'Parent'))
        self.maybeTypedef(_actorName(fqname, 'Child'),
                          _actorName(name, 'Child'))

        self.forwardDeclStmts.extend([
            _makeForwardDeclForActor(t.protocol, 'parent'), Whitespace.NL,
            _makeForwardDeclForActor(t.protocol, 'child'), Whitespace.NL
        ])

    def visitStructOrUnionType(self, su, defaultVisit):
        if su in self.visited or su == self.fortype: return
        self.visited.add(su)
        self.maybeTypedef(su.fullname(), su.name())

        if su.mutuallyRecursiveWith(self.fortype):
            self.forwardDeclStmts.append(_makeForwardDecl(su))

        return defaultVisit(self, su)

    def visitStructType(self, t):
        return self.visitStructOrUnionType(t, TypeVisitor.visitStructType)

    def visitUnionType(self, t):
        return self.visitStructOrUnionType(t, TypeVisitor.visitUnionType)

    def visitArrayType(self, t):
        return TypeVisitor.visitArrayType(self, t)

    def visitShmemType(self, s):
        if s in self.visited: return
        self.visited.add(s)
        self.usingTypedefs.append(Typedef(Type('mozilla::ipc::Shmem'),
                                          'Shmem'))

    def visitVoidType(self, v): assert 0
    def visitMessageType(self, v): assert 0
    def visitProtocolType(self, v): assert 0
    def visitStateType(self, v): assert 0


def _generateCxxStruct(sd):
    ''' '''
    # compute all the typedefs and forward decls we need to make
    gettypedeps = _ComputeTypeDeps(sd.decl.type)
    for f in sd.fields:
        f.ipdltype.accept(gettypedeps)

    usingTypedefs = gettypedeps.usingTypedefs
    forwarddeclstmts = gettypedeps.forwardDeclStmts

    struct = Class(sd.name, final=1)
    struct.addstmts([ Label.PRIVATE ]
                    + usingTypedefs
                    + [ Whitespace.NL, Label.PUBLIC ])

    constreftype = Type(sd.name, const=1, ref=1)
    initvar = ExprVar('Init')
    callinit = ExprCall(initvar)
    assignvar = ExprVar('Assign')

    def fieldsAsParamList():
        return [ Decl(f.inType(), f.argVar().name) for f in sd.fields ]

    def assignFromOther(oexpr):
        return ExprCall(assignvar,
                        args=[ f.initExpr(oexpr) for f in sd.fields ])

    # Struct()
    defctor = ConstructorDefn(ConstructorDecl(sd.name))
    defctor.addstmt(StmtExpr(callinit))
    struct.addstmts([ defctor, Whitespace.NL ])

    # Struct(const field1& _f1, ...)
    valctor = ConstructorDefn(ConstructorDecl(sd.name,
                                              params=fieldsAsParamList(),
                                              force_inline=1))
    valctor.addstmts([
        StmtExpr(callinit),
        StmtExpr(ExprCall(assignvar,
                          args=[ f.argVar() for f in sd.fields ]))
    ])
    struct.addstmts([ valctor, Whitespace.NL ])

    # Struct(const Struct& _o)
    ovar = ExprVar('_o')
    copyctor = ConstructorDefn(ConstructorDecl(
        sd.name,
        params=[ Decl(constreftype, ovar.name) ],
        force_inline=1))
    copyctor.addstmts([
        StmtExpr(callinit),
        StmtExpr(assignFromOther(ovar))
    ])
    struct.addstmts([ copyctor, Whitespace.NL ])

    # ~Struct()
    dtor = DestructorDefn(DestructorDecl(sd.name))
    for f in sd.fields:
        dtor.addstmts(f.destructStmts())
    struct.addstmts([ dtor, Whitespace.NL ])

    # Struct& operator=(const Struct& _o)
    opeq = MethodDefn(MethodDecl(
        'operator=',
        params=[ Decl(constreftype, ovar.name) ],
        force_inline=1))
    opeq.addstmt(StmtExpr(assignFromOther(ovar)))
    struct.addstmts([ opeq, Whitespace.NL ])

    # bool operator==(const Struct& _o)
    opeqeq = MethodDefn(MethodDecl(
        'operator==',
        params=[ Decl(constreftype, ovar.name) ],
        ret=Type.BOOL,
        const=1))
    for f in sd.fields:
        ifneq = StmtIf(ExprNot(
            ExprBinary(ExprCall(f.getMethod()), '==',
                       ExprCall(f.getMethod(ovar)))))
        ifneq.addifstmt(StmtReturn.FALSE)
        opeqeq.addstmt(ifneq)
    opeqeq.addstmt(StmtReturn.TRUE)
    struct.addstmts([ opeqeq, Whitespace.NL ])

    # field1& f1()
    # const field1& f1() const
    for f in sd.fields:
        get = MethodDefn(MethodDecl(f.getMethod().name,
                                    params=[ ],
                                    ret=f.refType(),
                                    force_inline=1))
        get.addstmt(StmtReturn(f.refExpr()))

        getconstdecl = deepcopy(get.decl)
        getconstdecl.ret = f.constRefType()
        getconstdecl.const = 1
        getconst = MethodDefn(getconstdecl)
        getconst.addstmt(StmtReturn(f.constRefExpr()))

        struct.addstmts([ get, getconst, Whitespace.NL ])

    # private:
    struct.addstmt(Label.PRIVATE)

    # Init()
    init = MethodDefn(MethodDecl(initvar.name))
    for f in sd.fields:
        init.addstmts(f.initStmts())
    struct.addstmts([ init, Whitespace.NL ])

    # Assign(const field1& _f1, ...)
    assign = MethodDefn(MethodDecl(assignvar.name,
                                   params=fieldsAsParamList()))
    assign.addstmts([ StmtExpr(ExprAssn(f.refExpr(), f.argVar()))
                      for f in sd.fields ])
    struct.addstmts([ assign, Whitespace.NL ])

    # members
    struct.addstmts([ StmtDecl(Decl(f.internalType(), f.memberVar().name))
                      for f in sd.fields ])

    return forwarddeclstmts, struct

##--------------------------------------------------

def _generateCxxUnion(ud):
    # This Union class basically consists of a type (enum) and a
    # union for storage.  The union can contain POD and non-POD
    # types.  Each type needs a copy ctor, assignment operator,
    # and dtor.
    #
    # Rather than templating this class and only providing
    # specializations for the types we support, which is slightly
    # "unsafe" in that C++ code can add additional specializations
    # without the IPDL compiler's knowledge, we instead explicitly
    # implement non-templated methods for each supported type.
    #
    # The one complication that arises is that C++, for arcane
    # reasons, does not allow the placement destructor of a
    # builtin type, like int, to be directly invoked.  So we need
    # to hack around this by internally typedef'ing all
    # constituent types.  Sigh.
    #
    # So, for each type, this "Union" class needs:
    # (private)
    #  - entry in the type enum
    #  - entry in the storage union
    #  - [type]ptr() method to get a type* from the underlying union
    #  - same as above to get a const type*
    #  - typedef to hack around placement delete limitations
    # (public)
    #  - placement delete case for dtor
    #  - copy ctor
    #  - case in generic copy ctor
    #  - operator= impl
    #  - case in generic operator=
    #  - operator [type&]
    #  - operator [const type&] const
    #  - [type&] get_[type]()
    #  - [const type&] get_[type]() const
    #
    cls = Class(ud.name, final=1)
    # const Union&, i.e., Union type with inparam semantics
    inClsType = Type(ud.name, const=1, ref=1)
    refClsType = Type(ud.name, ref=1)
    typetype = Type('Type')
    valuetype = Type('Value')
    mtypevar = ExprVar('mType')
    mvaluevar = ExprVar('mValue')
    maybedtorvar = ExprVar('MaybeDestroy')
    assertsanityvar = ExprVar('AssertSanity')
    tnonevar = ExprVar('T__None')
    tlastvar = ExprVar('T__Last')

    def callAssertSanity(uvar=None, expectTypeVar=None):
        func = assertsanityvar
        args = [ ]
        if uvar is not None:
            func = ExprSelect(uvar, '.', assertsanityvar.name)
        if expectTypeVar is not None:
            args.append(expectTypeVar)
        return ExprCall(func, args=args)

    def callMaybeDestroy(newTypeVar):
        return ExprCall(maybedtorvar, args=[ newTypeVar ])

    def maybeReconstruct(memb, newTypeVar):
        ifdied = StmtIf(callMaybeDestroy(newTypeVar))
        ifdied.addifstmt(StmtExpr(memb.callCtor()))
        return ifdied

    # compute all the typedefs and forward decls we need to make
    gettypedeps = _ComputeTypeDeps(ud.decl.type)
    for c in ud.components:
        c.ipdltype.accept(gettypedeps)

    usingTypedefs = gettypedeps.usingTypedefs
    forwarddeclstmts = gettypedeps.forwardDeclStmts

    # the |Type| enum, used to switch on the discunion's real type
    cls.addstmt(Label.PUBLIC)
    typeenum = TypeEnum(typetype.name)
    typeenum.addId(tnonevar.name, 0)
    firstid = ud.components[0].enum()
    typeenum.addId(firstid, 1)
    for c in ud.components[1:]:
        typeenum.addId(c.enum())
    typeenum.addId(tlastvar.name, ud.components[-1].enum())
    cls.addstmts([ StmtDecl(Decl(typeenum,'')),
                   Whitespace.NL ])

    cls.addstmt(Label.PRIVATE)
    cls.addstmts(
        usingTypedefs
        # hacky typedef's that allow placement dtors of builtins
        + [ Typedef(c.internalType(), c.typedef()) for c in ud.components ])
    cls.addstmt(Whitespace.NL)

    # the C++ union the discunion use for storage
    valueunion = TypeUnion(valuetype.name)
    for c in ud.components:
        valueunion.addComponent(c.unionType(), c.name)
    cls.addstmts([ StmtDecl(Decl(valueunion,'')),
                       Whitespace.NL ])

    # for each constituent type T, add private accessors that
    # return a pointer to the Value union storage casted to |T*|
    # and |const T*|
    for c in ud.components:
        getptr = MethodDefn(MethodDecl(
            c.getPtrName(), params=[ ], ret=c.ptrToInternalType(),
            force_inline=1))
        getptr.addstmt(StmtReturn(c.ptrToSelfExpr()))

        getptrconst = MethodDefn(MethodDecl(
            c.getConstPtrName(), params=[ ], ret=c.constPtrToType(),
            const=1, force_inline=1))
        getptrconst.addstmt(StmtReturn(c.constptrToSelfExpr()))

        cls.addstmts([ getptr, getptrconst ])
    cls.addstmt(Whitespace.NL)

    # add a helper method that invokes the placement dtor on the
    # current underlying value, only if |aNewType| is different
    # than the current type, and returns true if the underlying
    # value needs to be re-constructed
    newtypevar = ExprVar('aNewType')
    maybedtor = MethodDefn(MethodDecl(
        maybedtorvar.name,
        params=[ Decl(typetype, newtypevar.name) ],
        ret=Type.BOOL))
    # wasn't /actually/ dtor'd, but it needs to be re-constructed
    ifnone = StmtIf(ExprBinary(mtypevar, '==', tnonevar))
    ifnone.addifstmt(StmtReturn.TRUE)
    # same type, nothing to see here
    ifnochange = StmtIf(ExprBinary(mtypevar, '==', newtypevar))
    ifnochange.addifstmt(StmtReturn.FALSE)
    # need to destroy.  switch on underlying type
    dtorswitch = StmtSwitch(mtypevar)
    for c in ud.components:
        dtorswitch.addcase(
            CaseLabel(c.enum()),
            StmtBlock([ StmtExpr(c.callDtor()),
                        StmtBreak() ]))
    dtorswitch.addcase(
        DefaultLabel(),
        StmtBlock([ _runtimeAbort("not reached"), StmtBreak() ]))
    maybedtor.addstmts([
        ifnone,
        ifnochange,
        dtorswitch,
        StmtReturn.TRUE
    ])
    cls.addstmts([ maybedtor, Whitespace.NL ])

    # add helper methods that ensure the discunion has a
    # valid type
    sanity = MethodDefn(MethodDecl(
        assertsanityvar.name, ret=Type.VOID, const=1, force_inline=1))
    sanity.addstmts([
        _abortIfFalse(ExprBinary(tnonevar, '<=', mtypevar),
                      'invalid type tag'),
        _abortIfFalse(ExprBinary(mtypevar, '<=', tlastvar),
                      'invalid type tag') ])
    cls.addstmt(sanity)

    atypevar = ExprVar('aType')
    sanity2 = MethodDefn(
        MethodDecl(assertsanityvar.name,
                       params=[ Decl(typetype, atypevar.name) ],
                       ret=Type.VOID,
                       const=1, force_inline=1))
    sanity2.addstmts([
        StmtExpr(ExprCall(assertsanityvar)),
        _abortIfFalse(ExprBinary(mtypevar, '==', atypevar),
                      'unexpected type tag') ])
    cls.addstmts([ sanity2, Whitespace.NL ])

    ## ---- begin public methods -----

    # Union() default ctor
    cls.addstmts([
        Label.PUBLIC,
        ConstructorDefn(
            ConstructorDecl(ud.name, force_inline=1),
            memberinits=[ ExprMemberInit(mtypevar, [ tnonevar ]) ]),
        Whitespace.NL
    ])

    # Union(const T&) copy ctors
    othervar = ExprVar('aOther')
    for c in ud.components:
        copyctor = ConstructorDefn(ConstructorDecl(
            ud.name, params=[ Decl(c.inType(), othervar.name) ]))
        copyctor.addstmts([
            StmtExpr(c.callCtor(othervar)),
            StmtExpr(ExprAssn(mtypevar, c.enumvar())) ])
        cls.addstmts([ copyctor, Whitespace.NL ])

    # Union(const Union&) copy ctor
    copyctor = ConstructorDefn(ConstructorDecl(
        ud.name, params=[ Decl(inClsType, othervar.name) ]))
    othertype = ud.callType(othervar)
    copyswitch = StmtSwitch(othertype)
    for c in ud.components:
        copyswitch.addcase(
            CaseLabel(c.enum()),
            StmtBlock([
                StmtExpr(c.callCtor(
                    ExprCall(ExprSelect(othervar,
                                        '.', c.getConstTypeName())))),
                StmtBreak()
            ]))
    copyswitch.addcase(CaseLabel(tnonevar.name),
                       StmtBlock([ StmtBreak() ]))
    copyswitch.addcase(
        DefaultLabel(),
        StmtBlock([ _runtimeAbort('unreached'), StmtReturn() ]))
    copyctor.addstmts([
        StmtExpr(callAssertSanity(uvar=othervar)),
        copyswitch,
        StmtExpr(ExprAssn(mtypevar, othertype))
    ])
    cls.addstmts([ copyctor, Whitespace.NL ])

    # ~Union()
    dtor = DestructorDefn(DestructorDecl(ud.name))
    dtor.addstmt(StmtExpr(callMaybeDestroy(tnonevar)))
    cls.addstmts([ dtor, Whitespace.NL ])

    # type()
    typemeth = MethodDefn(MethodDecl('type', ret=typetype,
                                     const=1, force_inline=1))
    typemeth.addstmt(StmtReturn(mtypevar))
    cls.addstmts([ typemeth, Whitespace.NL ])

    # Union& operator=(const T&) methods
    rhsvar = ExprVar('aRhs')
    for c in ud.components:
        opeq = MethodDefn(MethodDecl(
            'operator=',
            params=[ Decl(c.inType(), rhsvar.name) ],
            ret=refClsType))
        opeq.addstmts([
            # might need to placement-delete old value first
            maybeReconstruct(c, c.enumvar()),
            StmtExpr(c.callOperatorEq(rhsvar)),
            StmtExpr(ExprAssn(mtypevar, c.enumvar())),
            StmtReturn(ExprDeref(ExprVar.THIS))
        ])
        cls.addstmts([ opeq, Whitespace.NL ])

    # Union& operator=(const Union&)
    opeq = MethodDefn(MethodDecl(
        'operator=',
        params=[ Decl(inClsType, rhsvar.name) ],
        ret=refClsType))
    rhstypevar = ExprVar('t')
    opeqswitch = StmtSwitch(rhstypevar)
    for c in ud.components:
        case = StmtBlock()
        case.addstmts([
            maybeReconstruct(c, rhstypevar),
            StmtExpr(c.callOperatorEq(
                ExprCall(ExprSelect(rhsvar, '.', c.getConstTypeName())))),
            StmtBreak()
        ])
        opeqswitch.addcase(CaseLabel(c.enum()), case)
    opeqswitch.addcase(CaseLabel(tnonevar.name),
                       StmtBlock([ StmtExpr(callMaybeDestroy(rhstypevar)),
                                   StmtBreak() ]))
    opeqswitch.addcase(
        DefaultLabel(),
        StmtBlock([ _runtimeAbort('unreached'), StmtBreak() ]))
    opeq.addstmts([
        StmtExpr(callAssertSanity(uvar=rhsvar)),
        StmtDecl(Decl(typetype, rhstypevar.name), init=ud.callType(rhsvar)),
        opeqswitch,
        StmtExpr(ExprAssn(mtypevar, rhstypevar)),
        StmtReturn(ExprDeref(ExprVar.THIS))
    ])
    cls.addstmts([ opeq, Whitespace.NL ])

    # bool operator==(const T&)
    for c in ud.components:
        opeqeq = MethodDefn(MethodDecl(
            'operator==',
            params=[ Decl(c.inType(), rhsvar.name) ],
            ret=Type.BOOL,
            const=1))
        opeqeq.addstmt(StmtReturn(ExprBinary(
            ExprCall(ExprVar(c.getTypeName())), '==', rhsvar)))
        cls.addstmts([ opeqeq, Whitespace.NL ])

    # bool operator==(const Union&)
    opeqeq = MethodDefn(MethodDecl(
        'operator==',
        params=[ Decl(inClsType, rhsvar.name) ],
        ret=Type.BOOL,
        const=1))
    iftypesmismatch = StmtIf(ExprBinary(ud.callType(), '!=',
                                        ud.callType(rhsvar)))
    iftypesmismatch.addifstmt(StmtReturn.FALSE)
    opeqeq.addstmts([ iftypesmismatch, Whitespace.NL ])

    opeqeqswitch = StmtSwitch(ud.callType())
    for c in ud.components:
        case = StmtBlock()
        case.addstmt(StmtReturn(ExprBinary(
            ExprCall(ExprVar(c.getTypeName())), '==',
            ExprCall(ExprSelect(rhsvar, '.', c.getTypeName())))))
        opeqeqswitch.addcase(CaseLabel(c.enum()), case)
    opeqeqswitch.addcase(
        DefaultLabel(),
        StmtBlock([ _runtimeAbort('unreached'),
                    StmtReturn.FALSE ]))
    opeqeq.addstmt(opeqeqswitch)

    cls.addstmts([ opeqeq, Whitespace.NL ])

    # accessors for each type: operator T&, operator const T&,
    # T& get(), const T& get()
    for c in ud.components:
        getValueVar = ExprVar(c.getTypeName())
        getConstValueVar = ExprVar(c.getConstTypeName())

        getvalue = MethodDefn(MethodDecl(getValueVar.name,
                                         ret=c.refType(),
                                         force_inline=1))
        getvalue.addstmts([
            StmtExpr(callAssertSanity(expectTypeVar=c.enumvar())),
            StmtReturn(ExprDeref(c.callGetPtr()))
        ])

        getconstvalue = MethodDefn(MethodDecl(
            getConstValueVar.name, ret=c.constRefType(),
            const=1, force_inline=1))
        getconstvalue.addstmts([
            StmtExpr(callAssertSanity(expectTypeVar=c.enumvar())),
            StmtReturn(c.getConstValue())
        ])

        optype = MethodDefn(MethodDecl('', typeop=c.refType(), force_inline=1))
        optype.addstmt(StmtReturn(ExprCall(getValueVar)))
        opconsttype = MethodDefn(MethodDecl(
            '', const=1, typeop=c.constRefType(), force_inline=1))
        opconsttype.addstmt(StmtReturn(ExprCall(getConstValueVar)))

        cls.addstmts([ getvalue, getconstvalue,
                       optype, opconsttype,
                       Whitespace.NL ])

    # private vars
    cls.addstmts([
        Label.PRIVATE,
        StmtDecl(Decl(valuetype, mvaluevar.name)),
        StmtDecl(Decl(typetype, mtypevar.name))
    ])

    return forwarddeclstmts, cls

##-----------------------------------------------------------------------------

class _FindFriends(ipdl.ast.Visitor):
    def __init__(self):
        self.mytype = None              # ProtocolType
        self.vtype = None               # ProtocolType
        self.friends = set()            # set<ProtocolType>

    def findFriends(self, ptype):
        self.mytype = ptype
        self.walkDownTheProtocolTree(ptype.toplevel())
        return self.friends

    # TODO could make this into a _iterProtocolTreeHelper ...
    def walkDownTheProtocolTree(self, ptype):
        if ptype != self.mytype:
            # don't want to |friend| ourself!
            self.visit(ptype)
        for mtype in ptype.manages:
            if mtype is not ptype:
                self.walkDownTheProtocolTree(mtype)

    def visit(self, ptype):
        # |vtype| is the type currently being visited
        savedptype = self.vtype
        self.vtype = ptype
        ptype._p.accept(self)
        self.vtype = savedptype

    def visitMessageDecl(self, md):
        for it in self.iterActorParams(md):
            if it.protocol == self.mytype:
                self.friends.add(self.vtype)

    def iterActorParams(self, md):
        for param in md.inParams:
            for actor in ipdl.type.iteractortypes(param.type):
                yield actor
        for ret in md.outParams:
            for actor in ipdl.type.iteractortypes(ret.type):
                yield actor


class _GenerateProtocolActorCode(ipdl.ast.Visitor):
    def __init__(self, myside):
        self.side = myside              # "parent" or "child"
        self.prettyside = myside.title()
        self.clsname = None
        self.protocol = None
        self.hdrfile = None
        self.cppfile = None
        self.ns = None
        self.cls = None
        self.includedActorTypedefs = [ ]
        self.includedActorUsings = [ ]
        self.protocolCxxIncludes = [ ]

    def lower(self, tu, clsname, cxxHeaderFile, cxxFile):
        self.clsname = clsname
        self.hdrfile = cxxHeaderFile
        self.cppfile = cxxFile
        tu.accept(self)

    def standardTypedefs(self):
        return [
            Typedef(Type('IPC::Message'), 'Message'),
            Typedef(Type(self.protocol.channelName()), 'Channel'),
            Typedef(Type(self.protocol.fqListenerName()), 'ChannelListener'),
            Typedef(Type('base::ProcessHandle'), 'ProcessHandle'),
            Typedef(Type('mozilla::ipc::SharedMemory'), 'SharedMemory'),
            Typedef(Type('mozilla::ipc::Trigger'), 'Trigger')
        ]


    def visitTranslationUnit(self, tu):
        self.protocol = tu.protocol

        hf = self.hdrfile
        cf = self.cppfile

        disclaimer = Whitespace('''//
// Automatically generated by ipdlc.
// Edit at your own risk
//

''')
        # make the C++ header
        hf.addthings(
            [ disclaimer ]
            + _includeGuardStart(hf)
            +[
                Whitespace.NL,
                CppDirective(
                    'include',
                    '"'+ _protocolHeaderName(tu.protocol) +'.h"')
            ])

        for pinc in tu.protocolIncludes:
            pinc.accept(self)

        # this generates the actor's full impl in self.cls
        tu.protocol.accept(self)

        clsdecl, clsdefn = _splitClassDeclDefn(self.cls)

        # XXX damn C++ ... return types in the method defn aren't in
        # class scope
        for stmt in clsdefn.stmts:
            if isinstance(stmt, MethodDefn):
                if stmt.decl.ret and stmt.decl.ret.name == 'Result':
                    stmt.decl.ret.name = clsdecl.name +'::'+ stmt.decl.ret.name

        def makeNamespace(p, file):
            if 0 == len(p.namespaces):
                return file
            ns = Namespace(p.namespaces[-1].name)
            outerns = _putInNamespaces(ns, p.namespaces[:-1])
            file.addthing(outerns)
            return ns

        hdrns = makeNamespace(self.protocol, self.hdrfile)
        hdrns.addstmts([
            Whitespace.NL,
            Whitespace.NL,
            clsdecl,
            Whitespace.NL,
            Whitespace.NL
        ])

        self.hdrfile.addthings(
            ([
                Whitespace.NL,
                CppDirective('if', '0') ])
            + _GenerateSkeletonImpl(
                _actorName(self.protocol.name, self.side)[1:],
                self.protocol.namespaces).fromclass(self.cls)
            +([
                CppDirective('endif', '// if 0'),
                Whitespace.NL ])
            + _includeGuardEnd(hf))

        # make the .cpp file
        cf.addthings([
            disclaimer,
            Whitespace.NL,
            CppDirective(
                'include',
                '"'+ _protocolHeaderName(self.protocol, self.side) +'.h"')
        ])
             
        if self.protocol.decl.type.isToplevel():
            cf.addthings([
                CppDirective('ifdef', 'MOZ_CRASHREPORTER'),
                CppDirective('  include', '"nsXULAppAPI.h"'),
                CppDirective('endif')
            ])

        cf.addthings((
            [ Whitespace.NL ]
            + self.protocolCxxIncludes
            + [ Whitespace.NL ]
            + self.standardTypedefs()
            + tu.protocol.decl.cxxtypedefs
            + self.includedActorUsings
            + [ Whitespace.NL ]))

        cppns = makeNamespace(self.protocol, cf)
        cppns.addstmts([
            Whitespace.NL,
            Whitespace.NL,
            clsdefn,
            Whitespace.NL,
            Whitespace.NL
        ])


    def visitProtocolInclude(self, pi):
        ip = pi.tu.protocol

        self.hdrfile.addthings([
            _makeForwardDeclForActor(ip.decl.type, self.side),
            Whitespace.NL
        ])
        self.protocolCxxIncludes.append(
            CppDirective(
                'include',
                '"%s.h"'% (_protocolHeaderName(ip, self.side))))

        if ip.decl.fullname is not None:
            self.includedActorTypedefs.append(Typedef(
                Type(_actorName(ip.decl.fullname, self.prettyside)),
                _actorName(ip.decl.shortname, self.prettyside)))
            self.includedActorUsings.append(Using(
                Type(_actorName(ip.decl.fullname, self.prettyside))))


    def visitProtocol(self, p):
        self.hdrfile.addthings([
            CppDirective('ifdef', 'DEBUG'),
            CppDirective('include', '"prenv.h"'),
            CppDirective('endif', '// DEBUG')
        ])

        self.protocol = p
        ptype = p.decl.type
        toplevel = p.decl.type.toplevel()

        # FIXME: all actors impl Iface for now
        if ptype.isManager() or 1:
            self.hdrfile.addthing(CppDirective('include', '"base/id_map.h"'))

        self.hdrfile.addthings([
            CppDirective('include', '"'+ p.channelHeaderFile() +'"'),
            Whitespace.NL ])

        self.cls = Class(
            self.clsname,
            inherits=[ Inherit(Type(p.fqListenerName()), viz='protected'),
                       Inherit(p.managerInterfaceType(), viz='protected') ],
            abstract=True)

        friends = _FindFriends().findFriends(ptype)
        if ptype.isManaged():
            friends.update(ptype.managers)

        # |friend| managed actors so that they can call our Dealloc*()
        friends.update(ptype.manages)

        # don't friend ourself if we're a self-managed protocol
        friends.discard(ptype)

        for friend in friends:
            self.hdrfile.addthings([
                Whitespace.NL,
                _makeForwardDeclForActor(friend, self.prettyside),
                Whitespace.NL
            ])
            self.cls.addstmts([
                FriendClassDecl(_actorName(friend.fullname(),
                                           self.prettyside)),
                Whitespace.NL ])

        self.cls.addstmt(Label.PROTECTED)
        for typedef in p.cxxTypedefs():
            self.cls.addstmt(typedef)
        for typedef in self.includedActorTypedefs:
            self.cls.addstmt(typedef)
        self.cls.addstmt(Whitespace.NL)

        self.cls.addstmts([ Typedef(p.fqStateType(), 'State'), Whitespace.NL ])

        # interface methods that the concrete subclass has to impl
        for md in p.messageDecls:
            isctor, isdtor = md.decl.type.isCtor(), md.decl.type.isDtor()

            if self.receivesMessage(md):
                # generate Recv/Answer* interface
                implicit = (not isdtor)
                recvDecl = MethodDecl(
                    md.recvMethod().name,
                    params=md.makeCxxParams(paramsems='in', returnsems='out',
                                            side=self.side, implicit=implicit),
                    ret=Type.BOOL, virtual=1)

                if isctor or isdtor:
                    defaultRecv = MethodDefn(recvDecl)
                    defaultRecv.addstmt(StmtReturn.TRUE)
                    self.cls.addstmt(defaultRecv)
                else:
                    recvDecl.pure = 1
                    self.cls.addstmt(StmtDecl(recvDecl))

        for md in p.messageDecls:
            managed = md.decl.type.constructedType()
            if not ptype.isManagerOf(managed) or md.decl.type.isDtor():
                continue

            # add the Alloc/Dealloc interface for managed actors
            actortype = md.actorDecl().bareType(self.side)
            
            self.cls.addstmt(StmtDecl(MethodDecl(
                _allocMethod(managed).name,
                params=md.makeCxxParams(side=self.side, implicit=0),
                ret=actortype,
                virtual=1, pure=1)))

            self.cls.addstmt(StmtDecl(MethodDecl(
                _deallocMethod(managed).name,
                params=[ Decl(actortype, 'actor') ],
                ret=Type.BOOL,
                virtual=1, pure=1)))

        # optional ActorDestroy() method; default is no-op
        self.cls.addstmts([
            Whitespace.NL,
            MethodDefn(MethodDecl(
                _destroyMethod().name,
                params=[ Decl(_DestroyReason.Type(), 'why') ],
                virtual=1)),
            Whitespace.NL
        ])

        if ptype.isToplevel():
            # void ProcessingError(code); default to no-op
            processingerror = MethodDefn(
                MethodDecl(p.processingErrorVar().name,
                           params=[ Param(_Result.Type(), 'code') ],
                           virtual=1))

            # bool ShouldContinueFromReplyTimeout(); default to |true|
            shouldcontinue = MethodDefn(
                MethodDecl(p.shouldContinueFromTimeoutVar().name,
                           ret=Type.BOOL, virtual=1))
            shouldcontinue.addstmt(StmtReturn.TRUE)

            # void Entered*()/Exited*(); default to no-op
            entered = MethodDefn(
                MethodDecl(p.enteredCxxStackVar().name, virtual=1))
            exited = MethodDefn(
                MethodDecl(p.exitedCxxStackVar().name, virtual=1))
            enteredcall = MethodDefn(
                MethodDecl(p.enteredCallVar().name, virtual=1))
            exitedcall = MethodDefn(
                MethodDecl(p.exitedCallVar().name, virtual=1))

            self.cls.addstmts([ processingerror,
                                shouldcontinue,
                                entered, exited,
                                enteredcall, exitedcall,
                                Whitespace.NL ])

        self.cls.addstmts((
            [ Label.PUBLIC ]
            + self.standardTypedefs()
            + [ Whitespace.NL ]
        ))

        self.cls.addstmt(Label.PUBLIC)
        # Actor()
        ctor = ConstructorDefn(ConstructorDecl(self.clsname))
        if ptype.isToplevel():
            ctor.memberinits = [
                ExprMemberInit(p.channelVar(), [
                    ExprCall(ExprVar('ALLOW_THIS_IN_INITIALIZER_LIST'),
                             [ ExprVar.THIS ]) ]),
                ExprMemberInit(p.lastActorIdVar(),
                               [ p.actorIdInit(self.side) ]),
                ExprMemberInit(p.lastShmemIdVar(),
                               [ p.shmemIdInit(self.side) ]),
                ExprMemberInit(p.stateVar(),
                               [ p.startState() ])
            ]
        else:
            ctor.memberinits = [
                ExprMemberInit(p.idVar(), [ ExprLiteral.ZERO ]),
                ExprMemberInit(p.stateVar(),
                               [ p.deadState() ])
            ]

        ctor.addstmt(StmtExpr(ExprCall(ExprVar('MOZ_COUNT_CTOR'),
                                       [ ExprVar(self.clsname) ])))
        self.cls.addstmts([ ctor, Whitespace.NL ])

        # ~Actor()
        dtor = DestructorDefn(
            DestructorDecl(self.clsname, virtual=True))
        dtor.addstmt(StmtExpr(ExprCall(ExprVar('MOZ_COUNT_DTOR'),
                                               [ ExprVar(self.clsname) ])))

        self.cls.addstmts([ dtor, Whitespace.NL ])

        if ptype.isToplevel():
            # Open()
            aTransportVar = ExprVar('aTransport')
            aThreadVar = ExprVar('aThread')
            processvar = ExprVar('aOtherProcess')
            openmeth = MethodDefn(
                MethodDecl(
                    'Open',
                    params=[ Decl(Type('Channel::Transport', ptr=True),
                                      aTransportVar.name),
                             Decl(Type('ProcessHandle'), processvar.name),
                             Param(Type('MessageLoop', ptr=True),
                                   aThreadVar.name,
                                   default=ExprLiteral.NULL) ],
                    ret=Type.BOOL))

            openmeth.addstmts([
                StmtExpr(ExprAssn(p.otherProcessVar(), processvar)),
                StmtReturn(ExprCall(ExprSelect(p.channelVar(), '.', 'Open'),
                                    [ aTransportVar, aThreadVar ]))
            ])
            self.cls.addstmts([
                openmeth,
                Whitespace.NL ])

            # Close()
            closemeth = MethodDefn(MethodDecl('Close'))
            closemeth.addstmt(StmtExpr(
                ExprCall(ExprSelect(p.channelVar(), '.', 'Close'))))
            self.cls.addstmts([ closemeth, Whitespace.NL ])

            if ptype.talksSync() or ptype.talksRpc():
                # SetReplyTimeoutMs()
                timeoutvar = ExprVar('aTimeoutMs')
                settimeout = MethodDefn(MethodDecl(
                    'SetReplyTimeoutMs',
                    params=[ Decl(Type.INT32, timeoutvar.name) ]))
                settimeout.addstmt(StmtExpr(
                    ExprCall(
                        ExprSelect(p.channelVar(), '.', 'SetReplyTimeoutMs'),
                        args=[ timeoutvar ])))
                self.cls.addstmts([ settimeout, Whitespace.NL ])

        if not ptype.isToplevel():
            if 1 == len(p.managers):
                ## manager()
                managertype = p.managerActorType(self.side, ptr=1)
                managermeth = MethodDefn(MethodDecl(
                    p.managerMethod().name, ret=managertype))
                managermeth.addstmt(StmtReturn(
                    ExprCast(p.managerVar(), managertype, static=1)))

                self.cls.addstmts([ managermeth, Whitespace.NL ])

        ## Managed[T](Array& inout) const
        ## const Array<T>& Managed() const
        for managed in ptype.manages:
            arrvar = ExprVar('aArr')
            meth = MethodDefn(MethodDecl(
                p.managedMethod(managed, self.side).name,
                params=[ Decl(p.managedVarType(managed, self.side, ref=1),
                              arrvar.name) ],
                const=1))
            meth.addstmt(StmtExpr(ExprAssn(
                arrvar, p.managedVar(managed, self.side))))

            refmeth = MethodDefn(MethodDecl(
                p.managedMethod(managed, self.side).name,
                params=[ ],
                ret=p.managedVarType(managed, self.side, const=1, ref=1),
                const=1))
            refmeth.addstmt(StmtReturn(p.managedVar(managed, self.side)))
            
            self.cls.addstmts([ meth, refmeth, Whitespace.NL ])

        statemethod = MethodDefn(MethodDecl(
            p.stateMethod().name,
            ret=p.fqStateType()))
        statemethod.addstmt(StmtReturn(p.stateVar()))
        self.cls.addstmts([ statemethod, Whitespace.NL ])

        ## OnMessageReceived()/OnCallReceived()

        # save these away for use in message handler case stmts
        msgvar = ExprVar('__msg')
        self.msgvar = msgvar
        replyvar = ExprVar('__reply')
        self.replyvar = replyvar
        itervar = ExprVar('__iter')
        self.itervar = itervar
        var = ExprVar('__v')
        self.var = var
        # for ctor recv cases, we can't read the actor ID into a PFoo*
        # because it doesn't exist on this side yet.  Use a "special"
        # actor handle instead
        handlevar = ExprVar('__handle')
        self.handlevar = handlevar

        msgtype = ExprCall(ExprSelect(msgvar, '.', 'type'), [ ])
        self.asyncSwitch = StmtSwitch(msgtype)
        if toplevel.talksSync():
            self.syncSwitch = StmtSwitch(msgtype)
            if toplevel.talksRpc():
                self.rpcSwitch = StmtSwitch(msgtype)

        # implement Send*() methods and add dispatcher cases to
        # message switch()es
        for md in p.messageDecls:
            self.visitMessageDecl(md)

        # add default cases
        default = StmtBlock()
        default.addstmt(StmtReturn(_Result.NotKnown))
        self.asyncSwitch.addcase(DefaultLabel(), default)
        if toplevel.talksSync():
            self.syncSwitch.addcase(DefaultLabel(), default)
            if toplevel.talksRpc():
                self.rpcSwitch.addcase(DefaultLabel(), default)


        def makeHandlerMethod(name, switch, hasReply, dispatches=0):
            params = [ Decl(Type('Message', const=1, ref=1), msgvar.name) ]
            if hasReply:
                params.append(Decl(Type('Message', ref=1, ptr=1),
                                   replyvar.name))
            
            method = MethodDefn(MethodDecl(name, virtual=True,
                                           params=params, ret=_Result.Type()))
            if dispatches:
                routevar = ExprVar('__route')
                routedecl = StmtDecl(
                    Decl(_actorIdType(), routevar.name),
                    init=ExprCall(ExprSelect(msgvar, '.', 'routing_id')))

                routeif = StmtIf(ExprBinary(
                    ExprVar('MSG_ROUTING_CONTROL'), '!=', routevar))
                routedvar = ExprVar('__routed')
                routeif.ifb.addstmt(
                    StmtDecl(Decl(Type('ChannelListener', ptr=1),
                                  routedvar.name),
                             _lookupListener(routevar)))
                failif = StmtIf(ExprPrefixUnop(routedvar, '!'))
                failif.ifb.addstmt(StmtReturn(_Result.RouteError))
                routeif.ifb.addstmt(failif)

                routeif.ifb.addstmt(StmtReturn(ExprCall(
                    ExprSelect(routedvar, '->', name),
                    args=[ ExprVar(p.name) for p in params ])))

                method.addstmts([ routedecl, routeif, Whitespace.NL ])

            # bug 509581: don't generate the switch stmt if there
            # is only the default case; MSVC doesn't like that
            if switch.nr_cases > 1:
                method.addstmt(switch)
            else:
                method.addstmt(StmtReturn(_Result.NotKnown))

            return method

        dispatches = (ptype.isToplevel() and ptype.isManager())
        self.cls.addstmts([
            makeHandlerMethod('OnMessageReceived', self.asyncSwitch,
                              hasReply=0, dispatches=dispatches),
            Whitespace.NL
        ])
        if toplevel.talksSync():
            self.cls.addstmts([
                makeHandlerMethod('OnMessageReceived', self.syncSwitch,
                                  hasReply=1, dispatches=dispatches),
                Whitespace.NL
            ])
            if toplevel.talksRpc():
                self.cls.addstmts([
                    makeHandlerMethod('OnCallReceived', self.rpcSwitch,
                                      hasReply=1, dispatches=dispatches),
                    Whitespace.NL
                ])

        destroysubtreevar = ExprVar('DestroySubtree')
        deallocsubtreevar = ExprVar('DeallocSubtree')
        deallocshmemvar = ExprVar('DeallocShmems')

        # OnProcesingError(code)
        codevar = ExprVar('code')
        onprocessingerror = MethodDefn(
            MethodDecl('OnProcessingError',
                       params=[ Param(_Result.Type(), codevar.name) ]))
        if ptype.isToplevel():
            onprocessingerror.addstmt(StmtReturn(
                ExprCall(p.processingErrorVar(), args=[ codevar ])))
        else:
            onprocessingerror.addstmt(
                _runtimeAbort("`OnProcessingError' called on non-toplevel actor"))
        self.cls.addstmts([ onprocessingerror, Whitespace.NL ])

        # OnReplyTimeout()
        if toplevel.talksSync() or toplevel.talksRpc():
            ontimeout = MethodDefn(
                MethodDecl('OnReplyTimeout', ret=Type.BOOL))

            if ptype.isToplevel():
                ontimeout.addstmt(StmtReturn(
                    ExprCall(p.shouldContinueFromTimeoutVar())))
            else:
                ontimeout.addstmts([
                    _runtimeAbort("`OnReplyTimeout' called on non-toplevel actor"),
                    StmtReturn.FALSE
                ])

            self.cls.addstmts([ ontimeout, Whitespace.NL ])

        # C++-stack-related methods
        if ptype.isToplevel() and toplevel.talksRpc():
            # OnEnteredCxxStack()
            onentered = MethodDefn(MethodDecl('OnEnteredCxxStack'))
            onentered.addstmt(StmtReturn(ExprCall(p.enteredCxxStackVar())))

            # OnExitedCxxStack()
            onexited = MethodDefn(MethodDecl('OnExitedCxxStack'))
            onexited.addstmt(StmtReturn(ExprCall(p.exitedCxxStackVar())))

            # OnEnteredCxxStack()
            onenteredcall = MethodDefn(MethodDecl('OnEnteredCall'))
            onenteredcall.addstmt(StmtReturn(ExprCall(p.enteredCallVar())))

            # OnExitedCxxStack()
            onexitedcall = MethodDefn(MethodDecl('OnExitedCall'))
            onexitedcall.addstmt(StmtReturn(ExprCall(p.exitedCallVar())))

            # bool IsOnCxxStack()
            onstack = MethodDefn(
                MethodDecl(p.onCxxStackVar().name, ret=Type.BOOL, const=1))
            onstack.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.channelVar(), '.', p.onCxxStackVar().name))))

            # void ProcessIncomingRacingRPCCall
            processincoming = MethodDefn(
                MethodDecl('FlushPendingRPCQueue', ret=Type.VOID))
            processincoming.addstmt(StmtExpr(ExprCall(ExprSelect(_actorChannel(ExprVar.THIS), '.', 'FlushPendingRPCQueue'))))

            self.cls.addstmts([ onentered, onexited,
                                onenteredcall, onexitedcall,
                                onstack, processincoming, Whitespace.NL ])

        # OnChannelClose()
        onclose = MethodDefn(MethodDecl('OnChannelClose'))
        if ptype.isToplevel():
            onclose.addstmts([
                StmtExpr(ExprCall(destroysubtreevar,
                                  args=[ _DestroyReason.NormalShutdown ])),
                StmtExpr(ExprCall(deallocsubtreevar)),
                StmtExpr(ExprCall(deallocshmemvar))
            ])
        else:
            onclose.addstmt(
                _runtimeAbort("`OnClose' called on non-toplevel actor"))
        self.cls.addstmts([ onclose, Whitespace.NL ])

        # OnChannelError()
        onerror = MethodDefn(MethodDecl('OnChannelError'))
        if ptype.isToplevel():
            onerror.addstmts([
                StmtExpr(ExprCall(destroysubtreevar,
                                  args=[ _DestroyReason.AbnormalShutdown ])),
                StmtExpr(ExprCall(deallocsubtreevar)),
                StmtExpr(ExprCall(deallocshmemvar))
            ])
        else:
            onerror.addstmt(
                _runtimeAbort("`OnError' called on non-toplevel actor"))
        self.cls.addstmts([ onerror, Whitespace.NL ])

        # OnChannelConnected()
        onconnected = MethodDefn(MethodDecl('OnChannelConnected',
                                            params=[ Decl(Type.INT32, 'pid') ]))
        if not ptype.isToplevel():
            onconnected.addstmt(
                _runtimeAbort("'OnConnected' called on non-toplevel actor"))

        self.cls.addstmts([ onconnected, Whitespace.NL ])
        # FIXME/bug 535053: only manager protocols and non-manager
        # protocols with union types need Lookup().  we'll give it to
        # all for the time being (simpler)
        if 1 or ptype.isManager():
            self.cls.addstmts(self.implementManagerIface())

        # User-facing shmem methods
        self.cls.addstmts(self.makeShmemIface())

        if (ptype.isToplevel() and self.side is 'parent'
            and ptype.talksRpc()):

            processnative = MethodDefn(
                MethodDecl('ProcessNativeEventsInRPCCall', ret=Type.VOID))

            processnative.addstmts([
                    CppDirective('ifdef', 'OS_WIN'),
                    StmtExpr(ExprCall(
                            ExprSelect(p.channelVar(), '.',
                                       'ProcessNativeEventsInRPCCall'))),
                    CppDirective('else'),
                    _runtimeAbort('This method is Windows-only'),
                    CppDirective('endif'),
                    ])

            self.cls.addstmts([ processnative, Whitespace.NL ])

        if ptype.isToplevel() and self.side is 'parent':
            ## void SetOtherProcess(ProcessHandle pid)
            otherprocessvar = ExprVar('aOtherProcess')
            setotherprocess = MethodDefn(MethodDecl(
                    'SetOtherProcess',
                    params=[ Decl(Type('ProcessHandle'), otherprocessvar.name)]))
            setotherprocess.addstmt(StmtExpr(ExprAssn(p.otherProcessVar(), otherprocessvar)))
            self.cls.addstmts([
                    setotherprocess,
                    Whitespace.NL])

            ## bool GetMinidump(nsIFile** dump)
            self.cls.addstmt(Label.PROTECTED)

            otherpidvar = ExprVar('OtherSidePID')
            otherpid = MethodDefn(MethodDecl(
                otherpidvar.name, params=[ ],
                ret=Type('base::ProcessId'),
                const=1))
            otherpid.addstmts([
                StmtReturn(ExprCall(
                    ExprVar('base::GetProcId'),
                    args=[ p.otherProcessVar() ])),
            ])

            dumpvar = ExprVar('aDump')
            getdump = MethodDefn(MethodDecl(
                'TakeMinidump',
                params=[ Decl(Type('nsILocalFile', ptrptr=1), dumpvar.name) ],
                ret=Type.BOOL,
                const=1))
            getdump.addstmts([
                CppDirective('ifdef', 'MOZ_CRASHREPORTER'),
                StmtReturn(ExprCall(
                    ExprVar('XRE_TakeMinidumpForChild'),
                    args=[ ExprCall(otherpidvar), dumpvar ])),
                CppDirective('else'),
                StmtReturn.FALSE,
                CppDirective('endif')
            ])
            self.cls.addstmts([ otherpid, Whitespace.NL,
                                getdump, Whitespace.NL ])

        if (ptype.isToplevel() and self.side is 'parent'
            and ptype.talksRpc()):
            # offer BlockChild() and UnblockChild().
            # See ipc/glue/RPCChannel.h
            blockchild = MethodDefn(MethodDecl(
                'BlockChild', ret=Type.BOOL))
            blockchild.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.channelVar(), '.', 'BlockChild'))))

            unblockchild = MethodDefn(MethodDecl(
                'UnblockChild', ret=Type.BOOL))
            unblockchild.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.channelVar(), '.', 'UnblockChild'))))

            self.cls.addstmts([ blockchild, unblockchild, Whitespace.NL ])

        ## private methods
        self.cls.addstmt(Label.PRIVATE)

        ## FatalError()       
        msgvar = ExprVar('msg')
        fatalerror = MethodDefn(MethodDecl(
            'FatalError',
            params=[ Decl(Type('char', const=1, ptrconst=1), msgvar.name) ],
            const=1, virtual=1))
        fatalerror.addstmts([
            Whitespace('// Virtual method to prevent inlining.\n', indent=1),
            Whitespace('// This give us better error reporting.\n', indent=1),
            Whitespace('// See bug 589371\n\n', indent=1),
            _printErrorMessage('IPDL error:'),
            _printErrorMessage(msgvar),
            Whitespace.NL
        ])
        actorname = _actorName(p.name, self.side)
        if self.side is 'parent':
            # if the error happens on the parent side, the parent
            # kills off the child
            fatalerror.addstmts([
                _printErrorMessage(
                    '['+ actorname +'] killing child side as a result'),
                Whitespace.NL
            ])

            ifkill = StmtIf(ExprNot(
                _killProcess(ExprCall(p.otherProcessMethod()))))
            ifkill.addifstmt(
                _printErrorMessage("  may have failed to kill child!"))
            fatalerror.addstmt(ifkill)
        else:
            # and if it happens on the child side, the child commits
            # seppuko
            fatalerror.addstmt(
                _runtimeAbort('['+ actorname +'] abort()ing as a result'))
        self.cls.addstmts([ fatalerror, Whitespace.NL ])

        ## DestroySubtree(bool normal)
        whyvar = ExprVar('why')
        subtreewhyvar = ExprVar('subtreewhy')
        kidsvar = ExprVar('kids')
        ivar = ExprVar('i')
        ithkid = ExprIndex(kidsvar, ivar)

        destroysubtree = MethodDefn(MethodDecl(
            destroysubtreevar.name,
            params=[ Decl(_DestroyReason.Type(), whyvar.name) ]))

        if ptype.isManaged():
            destroysubtree.addstmt(
                Whitespace('// Unregister from our manager.\n', indent=1))
            destroysubtree.addstmts(self.unregisterActor())
            destroysubtree.addstmt(Whitespace.NL)

        if ptype.isManager():
            # only declare this for managers to avoid unused var warnings
            destroysubtree.addstmts([
                StmtDecl(
                    Decl(_DestroyReason.Type(), subtreewhyvar.name),
                    init=ExprConditional(
                        ExprBinary(
                            ExprBinary(whyvar, '==',
                                       _DestroyReason.Deletion),
                            '||',
                            ExprBinary(whyvar, '==',
                                       _DestroyReason.FailedConstructor)),
                        _DestroyReason.AncestorDeletion, whyvar)),
                Whitespace.NL
            ])

        for managed in ptype.manages:
            foreachdestroy = StmtFor(
                init=Param(Type.UINT32, ivar.name, ExprLiteral.ZERO),
                cond=ExprBinary(ivar, '<', _callCxxArrayLength(kidsvar)),
                update=ExprPrefixUnop(ivar, '++'))
            foreachdestroy.addstmt(StmtExpr(ExprCall(
                ExprSelect(ithkid, '->', destroysubtreevar.name),
                args=[ subtreewhyvar ])))

            block = StmtBlock()
            block.addstmts([
                Whitespace(
                    '// Recursively shutting down %s kids\n'% (managed.name()),
                    indent=1),
                StmtDecl(
                    Decl(p.managedVarType(managed, self.side), kidsvar.name),
                    init=p.managedVar(managed, self.side)),
                foreachdestroy,
            ])
            destroysubtree.addstmt(block)

        if len(ptype.manages):
            destroysubtree.addstmt(Whitespace.NL)
        destroysubtree.addstmts([ Whitespace('// Finally, destroy "us".\n',
                                             indent=1),
                                  StmtExpr(ExprCall(_destroyMethod(),
                                                    args=[ whyvar ]))
                                ])

        self.cls.addstmts([ destroysubtree, Whitespace.NL ])

        ## DeallocSubtree()
        deallocsubtree = MethodDefn(MethodDecl(deallocsubtreevar.name))
        for managed in ptype.manages:
            foreachrecurse = StmtFor(
                init=Param(Type.UINT32, ivar.name, ExprLiteral.ZERO),
                cond=ExprBinary(ivar, '<', _callCxxArrayLength(kidsvar)),
                update=ExprPrefixUnop(ivar, '++'))
            foreachrecurse.addstmt(StmtExpr(ExprCall(
                ExprSelect(ithkid, '->', deallocsubtreevar.name))))

            foreachdealloc = StmtFor(
                init=Param(Type.UINT32, ivar.name, ExprLiteral.ZERO),
                cond=ExprBinary(ivar, '<', _callCxxArrayLength(kidsvar)),
                update=ExprPrefixUnop(ivar, '++'))
            foreachdealloc.addstmts([
                StmtExpr(ExprCall(_deallocMethod(managed),
                                  args=[ ithkid ]))
            ])

            block = StmtBlock()
            block.addstmts([
                Whitespace(
                    '// Recursively deleting %s kids\n'% (managed.name()),
                    indent=1),
                StmtDecl(
                    Decl(p.managedVarType(managed, self.side, ref=1),
                         kidsvar.name),
                    init=p.managedVar(managed, self.side)),
                foreachrecurse,
                Whitespace.NL,
                # no need to copy |kids| here; we're the ones deleting
                # stragglers, no outside C++ is being invoked (except
                # Dealloc(subactor))
                foreachdealloc,
                StmtExpr(_callCxxArrayClear(p.managedVar(managed, self.side))),

            ])
            deallocsubtree.addstmt(block)
        # don't delete outselves: either the manager will do it, or
        # we're toplevel
        self.cls.addstmts([ deallocsubtree, Whitespace.NL ])

        if ptype.isToplevel():
            ## DeallocShmem():
            #    for (cit = map.begin(); cit != map.end(); ++cit)
            #      Dealloc(cit->second)
            #    map.Clear()
            deallocshmem = MethodDefn(MethodDecl(deallocshmemvar.name))

            citvar = ExprVar('cit')
            begin = ExprCall(ExprSelect(p.shmemMapVar(), '.', 'begin'))
            end = ExprCall(ExprSelect(p.shmemMapVar(), '.', 'end'))
            shmem = ExprSelect(citvar, '->', 'second')
            foreachdealloc = StmtFor(
                Param(p.shmemIteratorType(), citvar.name, begin),
                ExprBinary(citvar, '!=', end),
                ExprPrefixUnop(citvar, '++'))
            foreachdealloc.addstmt(StmtExpr(_shmemDealloc(shmem)))

            deallocshmem.addstmts([
                foreachdealloc,
                StmtExpr(ExprCall(ExprSelect(p.shmemMapVar(), '.', 'Clear')))
            ])
            self.cls.addstmts([ deallocshmem, Whitespace.NL ])

        self.implementPickling()

        ## private members
        self.cls.addstmt(StmtDecl(Decl(p.channelType(), 'mChannel')))
        if ptype.isToplevel():
            self.cls.addstmts([
                StmtDecl(Decl(Type('IDMap', T=Type('ChannelListener')),
                              p.actorMapVar().name)),
                StmtDecl(Decl(_actorIdType(), p.lastActorIdVar().name)),
                StmtDecl(Decl(Type('ProcessHandle'),
                              p.otherProcessVar().name))
            ])
        elif ptype.isManaged():
            self.cls.addstmts([
                StmtDecl(Decl(_actorIdType(), p.idVar().name)),
                StmtDecl(Decl(p.managerInterfaceType(ptr=1),
                              p.managerVar().name))
            ])
        if p.decl.type.isToplevel():
            self.cls.addstmts([
                StmtDecl(Decl(p.shmemMapType(), p.shmemMapVar().name)),
                StmtDecl(Decl(_shmemIdType(), p.lastShmemIdVar().name))
            ])

        self.cls.addstmt(StmtDecl(Decl(Type('State'), p.stateVar().name)))

        for managed in ptype.manages:
            self.cls.addstmts([
                Whitespace('// Sorted by pointer value\n', indent=1),
                StmtDecl(Decl(
                    p.managedVarType(managed, self.side),
                    p.managedVar(managed, self.side).name)) ])

    def implementManagerIface(self):
        p = self.protocol
        routedvar = ExprVar('aRouted')
        idvar = ExprVar('aId')
        shmemvar = ExprVar('aShmem')
        rawvar = ExprVar('segment')
        sizevar = ExprVar('aSize')
        typevar = ExprVar('type')
        unsafevar = ExprVar('unsafe')
        listenertype = Type('ChannelListener', ptr=1)

        register = MethodDefn(MethodDecl(
            p.registerMethod().name,
            params=[ Decl(listenertype, routedvar.name) ],
            ret=_actorIdType(), virtual=1))
        registerid = MethodDefn(MethodDecl(
            p.registerIDMethod().name,
            params=[ Decl(listenertype, routedvar.name),
                     Decl(_actorIdType(), idvar.name) ],
            ret=_actorIdType(),
            virtual=1))
        lookup = MethodDefn(MethodDecl(
            p.lookupIDMethod().name,
            params=[ Decl(_actorIdType(), idvar.name) ],
            ret=listenertype, virtual=1))
        unregister = MethodDefn(MethodDecl(
            p.unregisterMethod().name,
            params=[ Decl(_actorIdType(), idvar.name) ],
            virtual=1))

        createshmem = MethodDefn(MethodDecl(
            p.createSharedMemory().name,
            ret=_rawShmemType(ptr=1),
            params=[ Decl(Type.SIZE, sizevar.name),
                     Decl(_shmemTypeType(), typevar.name),
                     Decl(Type.BOOL, unsafevar.name),
                     Decl(_shmemIdType(ptr=1), idvar.name) ],
            virtual=1))
        adoptshmem = MethodDefn(MethodDecl(
            p.adoptSharedMemory().name,
            ret=Type.BOOL,
            params=[ Decl(_rawShmemType(ptr=1), rawvar.name),
                     Decl(_shmemIdType(ptr=1), idvar.name) ],
            virtual=1))
        lookupshmem = MethodDefn(MethodDecl(
            p.lookupSharedMemory().name,
            ret=_rawShmemType(ptr=1),
            params=[ Decl(_shmemIdType(), idvar.name) ],
            virtual=1))
        destroyshmem = MethodDefn(MethodDecl(
            p.destroySharedMemory().name,
            ret=Type.BOOL,
            params=[ Decl(_shmemType(ref=1), shmemvar.name) ],
            virtual=1))
        istracking = MethodDefn(MethodDecl(
            p.isTrackingSharedMemory().name,
            ret=Type.BOOL,
            params=[ Decl(_rawShmemType(ptr=1), rawvar.name) ],
            virtual=1))

        otherprocess = MethodDefn(MethodDecl(
            p.otherProcessMethod().name,
            ret=Type('ProcessHandle'),
            const=1,
            virtual=1))

        if p.decl.type.isToplevel():
            tmpvar = ExprVar('tmp')
            
            register.addstmts([
                StmtDecl(Decl(_actorIdType(), tmpvar.name),
                         p.nextActorIdExpr(self.side)),
                StmtExpr(ExprCall(
                    ExprSelect(p.actorMapVar(), '.', 'AddWithID'),
                    [ routedvar, tmpvar ])),
                StmtReturn(tmpvar)
            ])
            registerid.addstmts([
                StmtExpr(
                    ExprCall(ExprSelect(p.actorMapVar(), '.', 'AddWithID'),
                             [ routedvar, idvar ])),
                StmtReturn(idvar)
            ])
            lookup.addstmt(StmtReturn(
                ExprCall(ExprSelect(p.actorMapVar(), '.', 'Lookup'),
                         [ idvar ])))
            unregister.addstmt(StmtReturn(
                ExprCall(ExprSelect(p.actorMapVar(), '.', 'Remove'),
                         [ idvar ])))

            # SharedMemory* CreateSharedMemory(size, type, bool, id_t*):
            #   nsAutoPtr<SharedMemory> seg(Shmem::Alloc(size, type, unsafe));
            #   if (!shmem)
            #     return false
            #   Shmem s(seg, [nextshmemid]);
            #   Message descriptor;
            #   if (!s->ShareTo(subprocess, mId, descriptor) ||
            #       !Send(descriptor))
            #     return null;
            #   mShmemMap.Add(seg, id);
            #   return shmem.forget();
            createshmem.addstmt(StmtDecl(
                Decl(_autoptr(_rawShmemType()), rawvar.name),
                initargs=[ _shmemAlloc(sizevar, typevar, unsafevar) ]))
            failif = StmtIf(ExprNot(rawvar))
            failif.addifstmt(StmtReturn(ExprLiteral.NULL))
            createshmem.addstmt(failif)

            descriptorvar = ExprVar('descriptor')
            createshmem.addstmts([
                StmtDecl(
                    Decl(_shmemType(), shmemvar.name),
                    initargs=[ _shmemBackstagePass(),
                               _autoptrGet(rawvar),
                               p.nextShmemIdExpr(self.side) ]),
                StmtDecl(Decl(Type('Message', ptr=1), descriptorvar.name),
                         init=_shmemShareTo(shmemvar,
                                            ExprCall(p.otherProcessMethod()),
                                            p.routingId()))
            ])
            failif = StmtIf(ExprNot(descriptorvar))
            failif.addifstmt(StmtReturn.FALSE)
            createshmem.addstmt(failif)

            failif = StmtIf(ExprNot(ExprCall(
                ExprSelect(p.channelVar(), p.channelSel(), 'Send'),
                args=[ descriptorvar ])))
            createshmem.addstmt(failif)

            createshmem.addstmts([
                StmtExpr(ExprAssn(ExprDeref(idvar), _shmemId(shmemvar))),
                StmtExpr(ExprCall(
                    ExprSelect(p.shmemMapVar(), '.', 'AddWithID'),
                    args=[ rawvar, ExprDeref(idvar) ])),
                StmtReturn(_autoptrForget(rawvar))
            ])

            # SharedMemory* AdoptSharedMemory(SharedMemory*, id_t*):
            #   Shmem s(seg, [nextshmemid]);
            #   Message descriptor;
            #   if (!s->ShareTo(subprocess, mId, descriptor) ||
            #       !Send(descriptor))
            #     return false;
            #   mShmemMap.Add(seg, id);
            #   seg->AddRef();
            #   return true;

            # XXX this is close to the same code as above, could be
            # refactored
            descriptorvar = ExprVar('descriptor')
            adoptshmem.addstmts([
                StmtDecl(
                    Decl(_shmemType(), shmemvar.name),
                    initargs=[ _shmemBackstagePass(),
                               rawvar,
                               p.nextShmemIdExpr(self.side) ]),
                StmtDecl(Decl(Type('Message', ptr=1), descriptorvar.name),
                         init=_shmemShareTo(shmemvar,
                                            ExprCall(p.otherProcessMethod()),
                                            p.routingId()))
            ])
            failif = StmtIf(ExprNot(descriptorvar))
            failif.addifstmt(StmtReturn.FALSE)
            adoptshmem.addstmt(failif)

            failif = StmtIf(ExprNot(ExprCall(
                ExprSelect(p.channelVar(), p.channelSel(), 'Send'),
                args=[ descriptorvar ])))
            adoptshmem.addstmt(failif)

            adoptshmem.addstmts([
                StmtExpr(ExprAssn(ExprDeref(idvar), _shmemId(shmemvar))),
                StmtExpr(ExprCall(
                    ExprSelect(p.shmemMapVar(), '.', 'AddWithID'),
                    args=[ rawvar, ExprDeref(idvar) ])),
                StmtExpr(ExprCall(ExprSelect(rawvar, '->', 'AddRef'))),
                StmtReturn.TRUE
            ])

            # SharedMemory* Lookup(id)
            lookupshmem.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.shmemMapVar(), '.', 'Lookup'),
                args=[ idvar ])))

            # bool IsTrackingSharedMemory(mem)
            istracking.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.shmemMapVar(), '.', 'HasData'),
                args=[ rawvar ])))

            # bool DestroySharedMemory(shmem):
            #   id = shmem.Id()
            #   SharedMemory* rawmem = Lookup(id)
            #   if (!rawmem)
            #     return false;
            #   Message descriptor = UnShare(subprocess, mId, descriptor)
            #   mShmemMap.Remove(id)
            #   Shmem::Dealloc(rawmem)
            #   return descriptor && Send(descriptor)
            destroyshmem.addstmts([
                StmtDecl(Decl(_shmemIdType(), idvar.name),
                         init=_shmemId(shmemvar)),
                StmtDecl(Decl(_rawShmemType(ptr=1), rawvar.name),
                         init=_lookupShmem(idvar))
            ])

            failif = StmtIf(ExprNot(rawvar))
            failif.addifstmt(StmtReturn.FALSE)
            destroyshmem.addstmts([
                failif,
                StmtDecl(Decl(Type('Message', ptr=1), descriptorvar.name),
                         init=_shmemUnshareFrom(
                             shmemvar,
                             ExprCall(p.otherProcessMethod()),
                             p.routingId())),
                Whitespace.NL,
                StmtExpr(p.removeShmemId(idvar)),
                StmtExpr(_shmemDealloc(rawvar)),
                Whitespace.NL,
                StmtReturn(ExprBinary(
                    descriptorvar, '&&',
                    ExprCall(
                        ExprSelect(p.channelVar(), p.channelSel(), 'Send'),
                        args=[ descriptorvar ])))
            ])


            # "private" message that passes shmem mappings from one process
            # to the other
            if p.subtreeUsesShmem():
                self.asyncSwitch.addcase(
                    CaseLabel('SHMEM_CREATED_MESSAGE_TYPE'),
                    self.genShmemCreatedHandler())
                self.asyncSwitch.addcase(
                    CaseLabel('SHMEM_DESTROYED_MESSAGE_TYPE'),
                    self.genShmemDestroyedHandler())
            else:
                abort = StmtBlock()
                abort.addstmts([
                    _runtimeAbort('this protocol tree does not use shmem'),
                    StmtReturn(_Result.NotKnown)
                ])
                self.asyncSwitch.addcase(
                    CaseLabel('SHMEM_CREATED_MESSAGE_TYPE'), abort)
                self.asyncSwitch.addcase(
                    CaseLabel('SHMEM_DESTROYED_MESSAGE_TYPE'), abort)
            
            otherprocess.addstmt(StmtReturn(p.otherProcessVar()))
        else:
            # delegate registration to manager
            register.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.managerVar(), '->', p.registerMethod().name),
                [ routedvar ])))
            registerid.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.managerVar(), '->', p.registerIDMethod().name),
                [ routedvar, idvar ])))
            lookup.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.managerVar(), '->', p.lookupIDMethod().name),
                [ idvar ])))
            unregister.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.managerVar(), '->', p.unregisterMethod().name),
                [ idvar ])))
            createshmem.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.managerVar(), '->', p.createSharedMemory().name),
                [ sizevar, typevar, unsafevar, idvar ])))
            adoptshmem.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.managerVar(), '->', p.adoptSharedMemory().name),
                [ rawvar, idvar ])))
            lookupshmem.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.managerVar(), '->', p.lookupSharedMemory().name),
                [ idvar ])))
            istracking.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.managerVar(), '->',
                           p.isTrackingSharedMemory().name),
                [ rawvar ])))
            destroyshmem.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.managerVar(), '->', p.destroySharedMemory().name),
                [ shmemvar ])))
            otherprocess.addstmt(StmtReturn(ExprCall(
                ExprSelect(p.managerVar(), '->',
                           p.otherProcessMethod().name))))

        # all protocols share the "same" RemoveManagee() implementation
        pvar = ExprVar('aProtocolId')
        listenervar = ExprVar('aListener')
        removemanagee = MethodDefn(MethodDecl(
            p.removeManageeMethod().name,
            params=[ Decl(_protocolIdType(), pvar.name),
                     Decl(listenertype, listenervar.name) ],
            virtual=1))

        if not len(p.managesStmts):
            removemanagee.addstmts([ _runtimeAbort('unreached'), StmtReturn() ])
        else:
            switchontype = StmtSwitch(pvar)
            for managee in p.managesStmts:
                case = StmtBlock()
                actorvar = ExprVar('actor')
                manageeipdltype = managee.decl.type
                manageecxxtype = _cxxBareType(ipdl.type.ActorType(manageeipdltype),
                                              self.side)
                manageearray = p.managedVar(manageeipdltype, self.side)

                case.addstmts([
                    StmtDecl(Decl(manageecxxtype, actorvar.name),
                             ExprCast(listenervar, manageecxxtype, static=1)),
                    _abortIfFalse(
                        _cxxArrayHasElementSorted(manageearray, actorvar),
                        "actor not managed by this!"),
                    Whitespace.NL,
                    StmtExpr(_callCxxArrayRemoveSorted(manageearray, actorvar)),
                    StmtExpr(ExprCall(_deallocMethod(manageeipdltype),
                                      args=[ actorvar ])),
                    StmtReturn()
                ])
                switchontype.addcase(CaseLabel(_protocolId(manageeipdltype).name),
                                     case)
            default = StmtBlock()
            default.addstmts([ _runtimeAbort('unreached'), StmtReturn() ])
            switchontype.addcase(DefaultLabel(), default)
            removemanagee.addstmt(switchontype)

        return [ register,
                 registerid,
                 lookup,
                 unregister,
                 removemanagee,
                 createshmem,
                 adoptshmem,
                 lookupshmem,
                 istracking,
                 destroyshmem,
                 otherprocess,
                 Whitespace.NL ]

    def makeShmemIface(self):
        p = self.protocol
        idvar = ExprVar('aId')
        sizevar = ExprVar('aSize')
        typevar = ExprVar('aType')
        memvar = ExprVar('aMem')
        outmemvar = ExprVar('aOutMem')
        rawvar = ExprVar('rawmem')

        def allocShmemMethod(name, unsafe):
            # bool Alloc*Shmem(size_t size, Type type, Shmem* outmem):
            #   id_t id;
            #   nsAutoPtr<SharedMemory> mem(CreateSharedMemory(&id));
            #   if (!mem)
            #     return false;
            #   *outmem = Shmem(mem, id)
            #   return true;
            method = MethodDefn(MethodDecl(
                name,
                params=[ Decl(Type.SIZE, sizevar.name),
                         Decl(_shmemTypeType(), typevar.name),
                         Decl(_shmemType(ptr=1), memvar.name) ],
                ret=Type.BOOL))

            ifallocfails = StmtIf(ExprNot(rawvar))
            ifallocfails.addifstmt(StmtReturn.FALSE)

            if unsafe:
                unsafe = ExprLiteral.TRUE
            else:
                unsafe = ExprLiteral.FALSE
            method.addstmts([
                StmtDecl(Decl(_shmemIdType(), idvar.name)),
                StmtDecl(Decl(_autoptr(_rawShmemType()), rawvar.name),
                         initargs=[ ExprCall(p.createSharedMemory(),
                                         args=[ sizevar,
                                                typevar,
                                                unsafe,
                                                ExprAddrOf(idvar) ]) ]),
                ifallocfails,
                Whitespace.NL,
                StmtExpr(ExprAssn(
                    ExprDeref(memvar), _shmemCtor(_autoptrForget(rawvar), idvar))),
                StmtReturn.TRUE
            ])
            return method

        # bool AllocShmem(size_t size, Type type, Shmem* outmem):
        allocShmem = allocShmemMethod('AllocShmem', False)

        # bool AllocUnsafeShmem(size_t size, Type type, Shmem* outmem):
        allocUnsafeShmem = allocShmemMethod('AllocUnsafeShmem', True)

        # bool AdoptShmem(const Shmem& mem, Shmem* outmem):
        #   SharedMemory* raw = mem.mSegment;
        #   if (!raw || IsTrackingSharedMemory(raw))
        #     RUNTIMEABORT()
        #   id_t id
        #   if (!AdoptSharedMemory(raw, &id))
        #     return false
        #   *outmem = Shmem(raw, id);
        #   return true;
        adoptShmem = MethodDefn(MethodDecl(
            'AdoptShmem',
            params=[ Decl(_shmemType(const=1, ref=1), memvar.name),
                     Decl(_shmemType(ptr=1), outmemvar.name) ],
            ret=Type.BOOL))

        adoptShmem.addstmt(StmtDecl(Decl(_rawShmemType(ptr=1), rawvar.name),
                                    init=_shmemSegment(memvar)))
        ifbad = StmtIf(ExprBinary(
            ExprNot(rawvar), '||',
            ExprCall(ExprVar('IsTrackingSharedMemory'), args=[ rawvar ])))
        ifbad.addifstmt(_runtimeAbort('bad Shmem'))
        adoptShmem.addstmt(ifbad)

        ifadoptfails = StmtIf(ExprNot(ExprCall(
            p.adoptSharedMemory(), args=[ rawvar, ExprAddrOf(idvar) ])))
        ifadoptfails.addifstmt(StmtReturn.FALSE)

        adoptShmem.addstmts([
            Whitespace.NL,
            StmtDecl(Decl(_shmemIdType(), idvar.name)),
            ifadoptfails,
            Whitespace.NL,
            StmtExpr(ExprAssn(ExprDeref(outmemvar),
                              _shmemCtor(rawvar, idvar))),
            StmtReturn.TRUE
        ])

        # bool DeallocShmem(Shmem& mem):
        #   bool ok = DestroySharedMemory(mem);
        #   mem.forget();
        #   return ok;
        deallocShmem = MethodDefn(MethodDecl(
            'DeallocShmem',
            params=[ Decl(_shmemType(ref=1), memvar.name) ],
            ret=Type.BOOL))
        okvar = ExprVar('ok')

        deallocShmem.addstmts([
            StmtDecl(Decl(Type.BOOL, okvar.name),
                     init=ExprCall(p.destroySharedMemory(),
                                   args=[ memvar ])),
            StmtExpr(_shmemForget(memvar)),
            StmtReturn(okvar)
        ])

        return [ Whitespace('// Methods for managing shmem\n', indent=1),
                 allocShmem,
                 Whitespace.NL,
                 allocUnsafeShmem,
                 Whitespace.NL,
                 adoptShmem,
                 Whitespace.NL,
                 deallocShmem,
                 Whitespace.NL ]

    def genShmemCreatedHandler(self):
        p = self.protocol
        assert p.decl.type.isToplevel()
        
        case = StmtBlock()                                          

        rawvar = ExprVar('rawmem')
        idvar = ExprVar('id')
        case.addstmts([
            StmtDecl(Decl(_shmemIdType(), idvar.name)),
            StmtDecl(Decl(_autoptr(_rawShmemType()), rawvar.name),
                     initargs=[ _shmemOpenExisting(self.msgvar,
                                                   ExprAddrOf(idvar)) ])
        ])
        failif = StmtIf(ExprNot(rawvar))
        failif.addifstmt(StmtReturn(_Result.PayloadError))

        case.addstmts([
            failif,
            StmtExpr(ExprCall(
                ExprSelect(p.shmemMapVar(), '.', 'AddWithID'),
                args=[ _autoptrForget(rawvar), idvar ])),
            Whitespace.NL,
            StmtReturn(_Result.Processed)
        ])

        return case

    def genShmemDestroyedHandler(self):
        p = self.protocol
        assert p.decl.type.isToplevel()
        
        case = StmtBlock()                                          

        rawvar = ExprVar('rawmem')
        idvar = ExprVar('id')
        itervar = ExprVar('iter')
        case.addstmts([
            StmtDecl(Decl(_shmemIdType(), idvar.name)),
            StmtDecl(Decl(Type.VOIDPTR, itervar.name), init=ExprLiteral.NULL)
        ])

        failif = StmtIf(ExprNot(
            ExprCall(ExprVar('IPC::ReadParam'),
                     args=[ ExprAddrOf(self.msgvar), ExprAddrOf(itervar),
                            ExprAddrOf(idvar) ])))
        failif.addifstmt(StmtReturn(_Result.PayloadError))

        case.addstmts([
            failif,
            StmtExpr(ExprCall(ExprSelect(self.msgvar, '.', 'EndRead'),
                              args=[ itervar ])),
            Whitespace.NL,
            StmtDecl(Decl(_rawShmemType(ptr=1), rawvar.name),
                     init=ExprCall(p.lookupSharedMemory(), args=[ idvar ]))
        ])

        failif = StmtIf(ExprNot(rawvar))
        failif.addifstmt(StmtReturn(_Result.ValuError))

        case.addstmts([
            failif,
            StmtExpr(p.removeShmemId(idvar)),
            StmtExpr(_shmemDealloc(rawvar)),
            StmtReturn(_Result.Processed)
        ])

        return case


    ##-------------------------------------------------------------------------
    ## The next few functions are the crux of the IPDL code generator.
    ## They generate code for all the nasty work of message
    ## serialization/deserialization and dispatching handlers for
    ## received messages.
    ##
    def implementPickling(self):
        # pickling of "normal", non-IPDL types
        self.implementGenericPickling()

        # pickling for IPDL types
        specialtypes = set()
        class findSpecialTypes(TypeVisitor):
            def visitActorType(self, a):  specialtypes.add(a)
            def visitShmemType(self, s):  specialtypes.add(s)
            def visitStructType(self, s):
                specialtypes.add(s)
                return TypeVisitor.visitStructType(self, s)
            def visitUnionType(self, u):
                specialtypes.add(u)
                return TypeVisitor.visitUnionType(self, u)
            def visitArrayType(self, a):
                if a.basetype.isIPDL():
                    specialtypes.add(a)
                    return a.basetype.accept(self)

        for md in self.protocol.messageDecls:
            for param in md.params:
                mtype = md.decl.type
                # special case for top-level __delete__(), which isn't
                # understood yet
                if mtype.isDtor() and mtype.constructedType().isToplevel():
                    continue
                param.ipdltype.accept(findSpecialTypes())
            for ret in md.returns:
                ret.ipdltype.accept(findSpecialTypes())

        for t in specialtypes:
            if t.isActor():    self.implementActorPickling(t)
            elif t.isArray():  self.implementSpecialArrayPickling(t)
            elif t.isShmem():  self.implementShmemPickling(t)
            elif t.isStruct(): self.implementStructPickling(t)
            elif t.isUnion():  self.implementUnionPickling(t)
            else:
                assert 0 and 'unknown special type'

    def implementGenericPickling(self):
        var = self.var
        msgvar = self.msgvar
        itervar = self.itervar

        write = MethodDefn(self.writeMethodDecl(
            Type('T', const=1, ref=1), var, template=Type('T')))
        write.addstmt(StmtExpr(ExprCall(ExprVar('IPC::WriteParam'),
                                        args=[ msgvar, var ])))

        read = MethodDefn(self.readMethodDecl(
            Type('T', ptr=1), var, template=Type('T')))
        read.addstmt(StmtReturn(ExprCall(ExprVar('IPC::ReadParam'),
                                         args=[ msgvar, itervar, var ])))

        self.cls.addstmts([ write, Whitespace.NL, read, Whitespace.NL ])


    def implementActorPickling(self, actortype):
        # Note that we pickle based on *protocol* type and *not* actor
        # type.  The actor type includes a |nullable| qualifier, but
        # this method is not specialized based on nullability.  The
        # |actortype| nullability is ignored in this method.
        var = self.var
        idvar = ExprVar('id')
        intype = _cxxConstRefType(actortype, self.side)
        cxxtype = _cxxBareType(actortype, self.side)
        outtype = _cxxPtrToType(actortype, self.side)

        ## Write([const] PFoo* var)
        write = MethodDefn(self.writeMethodDecl(intype, var))
        nullablevar = ExprVar('__nullable')
        write.decl.params.append(Decl(Type.BOOL, nullablevar.name))
        # id_t id;
        # if (!var)
        #   if(!nullable)
        #     abort()
        #   id = NULL_ID
        write.addstmt(StmtDecl(Decl(_actorIdType(), idvar.name)))

        ifnull = StmtIf(ExprNot(var))
        ifnotnullable = StmtIf(ExprNot(nullablevar))
        ifnotnullable.addifstmt(
            _runtimeAbort("NULL actor value passed to non-nullable param"))
        ifnull.addifstmt(ifnotnullable)
        ifnull.addifstmt(StmtExpr(ExprAssn(idvar, _NULL_ACTOR_ID)))
        # else
        #   id = var->mId
        #   if (id == FREED_ID)
        #     abort()
        # Write(msg, id)
        ifnull.addelsestmt(StmtExpr(ExprAssn(idvar, _actorId(var))))
        iffreed = StmtIf(ExprBinary(_FREED_ACTOR_ID, '==', idvar))
        # this is always a hard-abort, because it means that some C++
        # code has a live pointer to a freed actor, so we're playing
        # Russian roulette with invalid memory
        iffreed.addifstmt(_runtimeAbort("actor has been |delete|d"))
        ifnull.addelsestmt(iffreed)

        write.addstmts([
            ifnull,
            Whitespace.NL,
            StmtExpr(self.write(None, idvar, self.msgvar))
        ])

        ## Read(PFoo** var)
        read = MethodDefn(self.readMethodDecl(outtype, var))
        read.decl.params.append(Decl(Type.BOOL, nullablevar.name))

        # if (!Read(id, msg))
        #   return false
        # if (FREED_ID == id
        #     || NULL_ID == id && !nullable)
        #   return false
        read.addstmts([
            StmtDecl(Decl(_actorIdType(), idvar.name)),
            self.checkedRead(None, ExprAddrOf(idvar),
                             self.msgvar, self.itervar, errfnRead),
        ])

        ifbadid = StmtIf(ExprBinary(
            ExprBinary(_FREED_ACTOR_ID, '==', idvar),
            '||',
            ExprBinary(ExprBinary(_NULL_ACTOR_ID, '==', idvar),
                       '&&',
                       ExprNot(nullablevar))))
        ifbadid.addifstmt(StmtReturn.FALSE)
        read.addstmts([ ifbadid, Whitespace.NL ])
        
        # if (NULL_ID == id)
        #   *var = null
        # else
        #   *var = Lookup(id)
        #   if (!*var)
        #     return false
        outactor = ExprDeref(var)
        ifnull = StmtIf(ExprBinary(_NULL_ACTOR_ID, '==', idvar))
        ifnull.addifstmt(StmtExpr(ExprAssn(outactor, ExprLiteral.NULL)))

        ifnull.addelsestmt(StmtExpr(ExprAssn(
            outactor,
            ExprCast(_lookupListener(idvar), cxxtype, static=1))))

        ifnotfound = StmtIf(ExprNot(outactor))
        ifnotfound.addifstmt(StmtReturn.FALSE)
        ifnull.addelsestmt(ifnotfound)

        read.addstmts([
            ifnull,
            StmtReturn.TRUE
        ])

        self.cls.addstmts([ write, Whitespace.NL, read, Whitespace.NL ])


    def implementSpecialArrayPickling(self, arraytype):
        var = self.var
        msgvar = self.msgvar
        itervar = self.itervar
        lenvar = ExprVar('length')
        ivar = ExprVar('i')
        eltipdltype = arraytype.basetype
        intype = _cxxConstRefType(arraytype, self.side)
        outtype = _cxxPtrToType(arraytype, self.side)

        write = MethodDefn(self.writeMethodDecl(intype, var))
        forwrite = StmtFor(init=ExprAssn(Decl(Type.UINT32, ivar.name),
                                         ExprLiteral.ZERO),
                           cond=ExprBinary(ivar, '<', lenvar),
                           update=ExprPrefixUnop(ivar, '++'))
        forwrite.addstmt(StmtExpr(
            self.write(eltipdltype, ExprIndex(var, ivar), msgvar)))
        write.addstmts([
            StmtDecl(Decl(Type.UINT32, lenvar.name),
                     init=_callCxxArrayLength(var)),
            StmtExpr(self.write(None, lenvar, msgvar)),
            Whitespace.NL,
            forwrite
        ])

        read = MethodDefn(self.readMethodDecl(outtype, var))
        avar = ExprVar('a')
        forread = StmtFor(init=ExprAssn(Decl(Type.UINT32, ivar.name),
                                        ExprLiteral.ZERO),
                          cond=ExprBinary(ivar, '<', lenvar),
                          update=ExprPrefixUnop(ivar, '++'))
        forread.addstmt(
            self.checkedRead(eltipdltype, ExprAddrOf(ExprIndex(avar, ivar)),
                             msgvar, itervar, errfnRead))
        read.addstmts([
            StmtDecl(Decl(_cxxRefType(arraytype, self.side), avar.name),
                     init=ExprDeref(var)),
            StmtDecl(Decl(Type.UINT32, lenvar.name)),
            self.checkedRead(None, ExprAddrOf(lenvar),
                             msgvar, itervar, errfnRead),
            Whitespace.NL,
            StmtExpr(_callCxxArraySetLength(var, lenvar, '->')),
            forread,
            StmtReturn.TRUE
        ])

        self.cls.addstmts([ write, Whitespace.NL, read, Whitespace.NL ])


    def implementShmemPickling(self, shmemtype):
        msgvar = self.msgvar
        itervar = self.itervar
        var = self.var
        tmpvar = ExprVar('tmp')
        idvar = ExprVar('shmemid')
        rawvar = ExprVar('rawmem')
        baretype = _cxxBareType(shmemtype, self.side)
        intype = _cxxConstRefType(shmemtype, self.side)
        outtype = _cxxPtrToType(shmemtype, self.side)

        write = MethodDefn(self.writeMethodDecl(intype, var))
        write.addstmts([
            StmtExpr(ExprCall(ExprVar('IPC::WriteParam'),
                              args=[ msgvar, var ])),
            StmtExpr(_shmemRevokeRights(var)),
            StmtExpr(_shmemForget(var))
        ])

        read = MethodDefn(self.readMethodDecl(outtype, var))
        ifread = StmtIf(ExprNot(ExprCall(ExprVar('IPC::ReadParam'),
                                         args=[ msgvar, itervar,
                                                ExprAddrOf(tmpvar) ])))
        ifread.addifstmt(StmtReturn.FALSE)

        iffound = StmtIf(rawvar)
        iffound.addifstmt(StmtExpr(ExprAssn(
            ExprDeref(var), _shmemCtor(rawvar, idvar))))
        iffound.addifstmt(StmtReturn.TRUE)

        read.addstmts([
            StmtDecl(Decl(_shmemType(), tmpvar.name)),
            ifread,
            Whitespace.NL,
            StmtDecl(Decl(_shmemIdType(), idvar.name),
                     init=_shmemId(tmpvar)),
            StmtDecl(Decl(_rawShmemType(ptr=1), rawvar.name),
                     init=_lookupShmem(idvar)),
            iffound,
            StmtReturn.FALSE
        ])

        self.cls.addstmts([ write, Whitespace.NL, read, Whitespace.NL ])


    def implementStructPickling(self, structtype):
        msgvar = self.msgvar
        itervar = self.itervar
        var = self.var
        intype = _cxxConstRefType(structtype, self.side)
        outtype = _cxxPtrToType(structtype, self.side)
        sd = _typeToAST[structtype]

        write = MethodDefn(self.writeMethodDecl(intype, var))
        read = MethodDefn(self.readMethodDecl(outtype, var))        

        def get(sel, f):
            return ExprCall(f.getMethod(thisexpr=var, sel=sel))

        for f in sd.fields:
            writefield = StmtExpr(self.write(f.ipdltype, get('.', f), msgvar))
            readfield = self.checkedRead(f.ipdltype,
                                         ExprAddrOf(get('->', f)),
                                         msgvar, itervar,
                                         errfn=errfnRead)
            if f.special and f.side != self.side:
                writefield = Whitespace(
                    "// skipping actor field that's meaningless on this side\n", indent=1)
                readfield = Whitespace(
                    "// skipping actor field that's meaningless on this side\n", indent=1)
            write.addstmt(writefield)
            read.addstmt(readfield)

        read.addstmt(StmtReturn.TRUE)

        self.cls.addstmts([ write, Whitespace.NL, read, Whitespace.NL ])


    def implementUnionPickling(self, uniontype):
        msgvar = self.msgvar
        itervar = self.itervar
        var = self.var
        intype = _cxxConstRefType(uniontype, self.side)
        outtype = _cxxPtrToType(uniontype, self.side)
        ud = _typeToAST[uniontype]

        typename = '__type'
        uniontdef = Typedef(_cxxBareType(uniontype, typename), typename)

        typevar = ExprVar('type')
        writeswitch = StmtSwitch(ud.callType(var))
        readswitch = StmtSwitch(typevar)

        for c in ud.components:
            ct = c.ipdltype
            isactor = (ct.isIPDL() and ct.isActor())
            caselabel = CaseLabel(typename +'::'+ c.enum())

            writecase = StmtBlock()
            if c.special and c.side != self.side:
                writecase.addstmt(_runtimeAbort('wrong side!'))
            else:
                wexpr = ExprCall(ExprSelect(var, '.', c.getTypeName()))
                writecase.addstmt(StmtExpr(self.write(ct, wexpr, msgvar)))

            writecase.addstmt(StmtReturn())
            writeswitch.addcase(caselabel, writecase)

            readcase = StmtBlock()
            if c.special and c.side == self.side:
                # the type comes across flipped from what the actor
                # will be on this side; i.e. child->parent messages
                # have type PFooChild when received on the parent side
                # XXX: better error message
                readcase.addstmt(StmtReturn.FALSE)
            else:
                if c.special:
                    c = c.other       # see above
                tmpvar = ExprVar('tmp')
                ct = c.bareType()
                readcase.addstmts([
                    StmtDecl(Decl(ct, tmpvar.name), init=c.defaultValue()),
                    StmtExpr(ExprAssn(ExprDeref(var), tmpvar)),
                    StmtReturn(self.read(
                        c.ipdltype,
                        ExprAddrOf(ExprCall(ExprSelect(var, '->',
                                                       c.getTypeName()))),
                        msgvar, itervar))
                ])

            readswitch.addcase(caselabel, readcase)

        unknowntype = 'unknown union type'
        writeswitch.addcase(DefaultLabel(),
                            StmtBlock([ _runtimeAbort(unknowntype),
                                        StmtReturn() ]))
        readswitch.addcase(DefaultLabel(), StmtBlock(errfnRead(unknowntype)))

        write = MethodDefn(self.writeMethodDecl(intype, var))
        write.addstmts([
            uniontdef,
            StmtExpr(self.write(
                None, ExprCall(Type.INT, args=[ ud.callType(var) ]), msgvar)),
            Whitespace.NL,
            writeswitch
        ])

        read = MethodDefn(self.readMethodDecl(outtype, var))
        read.addstmts([
            uniontdef,
            StmtDecl(Decl(Type.INT, typevar.name)),
            self.checkedRead(
                None, ExprAddrOf(typevar), msgvar, itervar, errfnRead),
            Whitespace.NL,
            readswitch,
        ])

        self.cls.addstmts([ write, Whitespace.NL, read, Whitespace.NL ])


    def writeMethodDecl(self, intype, var, template=None):
        return MethodDecl(
            'Write',
            params=[ Decl(intype, var.name),
                     Decl(Type('Message', ptr=1), self.msgvar.name) ],
            T=template)

    def readMethodDecl(self, outtype, var, template=None):
        return MethodDecl(
            'Read',
            params=[ Decl(outtype, var.name),
                     Decl(Type('Message', ptr=1, const=1),
                          self.msgvar.name),
                     Decl(Type('void', ptrptr=1), self.itervar.name)],
            warn_unused=not template,
            T=template,
            ret=Type.BOOL)

    def maybeAddNullabilityArg(self, ipdltype, call):
        if ipdltype and ipdltype.isIPDL() and ipdltype.isActor():
            if ipdltype.nullable:
                call.args.append(ExprLiteral.TRUE)
            else:
                call.args.append(ExprLiteral.FALSE)
        return call

    def write(self, ipdltype, expr, to, this=None):
        write = ExprVar('Write')
        if this:  write = ExprSelect(this, '->', write.name)
        return self.maybeAddNullabilityArg(ipdltype,
                                           ExprCall(write, args=[ expr, to ]))

    def read(self, ipdltype, expr, from_, iterexpr, this=None):
        read = ExprVar('Read')
        if this:  read = ExprSelect(this, '->', read.name)
        return self.maybeAddNullabilityArg(
            ipdltype, ExprCall(read, args=[ expr, from_, iterexpr ]))


    def visitMessageDecl(self, md):
        isctor = md.decl.type.isCtor()
        isdtor = md.decl.type.isDtor()
        sems = md.decl.type.sendSemantics
        sendmethod = None
        helpermethod = None
        recvlbl, recvcase = None, None

        def addRecvCase(lbl, case):
            if sems is ipdl.ast.ASYNC:
                self.asyncSwitch.addcase(lbl, case)
            elif sems is ipdl.ast.SYNC:
                self.syncSwitch.addcase(lbl, case)
            elif sems is ipdl.ast.RPC:
                self.rpcSwitch.addcase(lbl, case)
            else: assert 0

        if self.sendsMessage(md):
            isasync = (sems is ipdl.ast.ASYNC)

            if isctor:
                self.cls.addstmts([ self.genHelperCtor(md), Whitespace.NL ])

            if isctor and isasync:
                sendmethod, (recvlbl, recvcase) = self.genAsyncCtor(md)
            elif isctor:
                sendmethod = self.genBlockingCtorMethod(md)
            elif isdtor and isasync:
                sendmethod, (recvlbl, recvcase) = self.genAsyncDtor(md)
            elif isdtor:
                sendmethod = self.genBlockingDtorMethod(md)
            elif isasync:
                sendmethod = self.genAsyncSendMethod(md)
            else:
                sendmethod = self.genBlockingSendMethod(md)

        # XXX figure out what to do here
        if isdtor and md.decl.type.constructedType().isToplevel():
            sendmethod = None
                
        if sendmethod is not None:
            self.cls.addstmts([ sendmethod, Whitespace.NL ])
        if recvcase is not None:
            addRecvCase(recvlbl, recvcase)
            recvlbl, recvcase = None, None

        if self.receivesMessage(md):
            if isctor:
                recvlbl, recvcase = self.genCtorRecvCase(md)
            elif isdtor:
                recvlbl, recvcase = self.genDtorRecvCase(md)
            else:
                recvlbl, recvcase = self.genRecvCase(md)

            # XXX figure out what to do here
            if isdtor and md.decl.type.constructedType().isToplevel():
                return

            addRecvCase(recvlbl, recvcase)


    def genAsyncCtor(self, md):
        actor = md.actorDecl()
        method = MethodDefn(self.makeSendMethodDecl(md))
        method.addstmts(self.ctorPrologue(md) + [ Whitespace.NL ])

        msgvar, stmts = self.makeMessage(md, errfnSendCtor)
        sendok, sendstmts = self.sendAsync(md, msgvar)
        method.addstmts(
            stmts
            + sendstmts
            + self.failCtorIf(md, ExprNot(sendok))
            + [ StmtReturn(actor.var()) ])

        lbl = CaseLabel(md.pqReplyId())
        case = StmtBlock()
        case.addstmt(StmtReturn(_Result.Processed))
        # TODO not really sure what to do with async ctor "replies" yet.
        # destroy actor if there was an error?  tricky ...

        return method, (lbl, case)


    def genBlockingCtorMethod(self, md):
        actor = md.actorDecl()
        method = MethodDefn(self.makeSendMethodDecl(md))
        method.addstmts(self.ctorPrologue(md) + [ Whitespace.NL ])

        msgvar, stmts = self.makeMessage(md, errfnSendCtor)

        replyvar = self.replyvar
        sendok, sendstmts = self.sendBlocking(md, msgvar, replyvar)
        method.addstmts(
            stmts
            + [ Whitespace.NL,
                StmtDecl(Decl(Type('Message'), replyvar.name)) ]
            + sendstmts
            + self.failCtorIf(md, ExprNot(sendok)))

        def errfnCleanupCtor(msg):
            return self.failCtorIf(md, ExprLiteral.TRUE)
        stmts = self.deserializeReply(
            md, ExprAddrOf(replyvar), self.side, errfnCleanupCtor)
        method.addstmts(stmts + [ StmtReturn(actor.var()) ])

        return method


    def ctorPrologue(self, md, errfn=ExprLiteral.NULL, idexpr=None):
        actordecl = md.actorDecl()
        actorvar = actordecl.var()
        actorproto = actordecl.ipdltype.protocol

        if idexpr is None:
            idexpr = ExprCall(self.protocol.registerMethod(),
                              args=[ actorvar ])
        else:
            idexpr = ExprCall(self.protocol.registerIDMethod(),
                              args=[ actorvar, idexpr ])

        return [
            self.failIfNullActor(actorvar, errfn),
            StmtExpr(ExprAssn(_actorId(actorvar), idexpr)),
            StmtExpr(ExprAssn(_actorManager(actorvar), ExprVar.THIS)),
            StmtExpr(ExprAssn(_actorChannel(actorvar),
                              self.protocol.channelForSubactor())),
            StmtExpr(_callCxxArrayInsertSorted(
                self.protocol.managedVar(md.decl.type.constructedType(),
                                         self.side),
                actorvar)),
            StmtExpr(ExprAssn(_actorState(actorvar),
                              _startState(actorproto, fq=1)))
        ]

    def failCtorIf(self, md, cond):
        actorvar = md.actorDecl().var()
        type = md.decl.type.constructedType()
        failif = StmtIf(cond)
        failif.addifstmts(self.destroyActor(md, actorvar,
                                            why=_DestroyReason.FailedConstructor)
                          + [ StmtReturn(ExprLiteral.NULL) ])
        return [ failif ]

    def genHelperCtor(self, md):
        helperdecl = self.makeSendMethodDecl(md)
        helperdecl.params = helperdecl.params[1:]
        helper = MethodDefn(helperdecl)

        callctor = self.callAllocActor(md, retsems='out')
        helper.addstmt(StmtReturn(ExprCall(
            ExprVar(helperdecl.name), args=[ callctor ] + callctor.args)))
        return helper


    def genAsyncDtor(self, md):
        actor = md.actorDecl()
        actorvar = actor.var()
        method = MethodDefn(self.makeDtorMethodDecl(md))

        method.addstmts(self.dtorPrologue(actor.var()))
        method.addstmts(self.dtorPrologue(actorvar))

        msgvar, stmts = self.makeMessage(md, errfnSendDtor, actorvar)
        sendok, sendstmts = self.sendAsync(md, msgvar, actorvar)
        method.addstmts(
            stmts
            + sendstmts
            + [ Whitespace.NL ]
            + self.dtorEpilogue(md, actor.var())
            + [ StmtReturn(sendok) ])

        lbl = CaseLabel(md.pqReplyId())
        case = StmtBlock()
        case.addstmt(StmtReturn(_Result.Processed))
        # TODO if the dtor is "inherently racy", keep the actor alive
        # until the other side acks

        return method, (lbl, case)


    def genBlockingDtorMethod(self, md):
        actor = md.actorDecl()
        actorvar = actor.var()
        method = MethodDefn(self.makeDtorMethodDecl(md))

        method.addstmts(self.dtorPrologue(actorvar))

        msgvar, stmts = self.makeMessage(md, errfnSendDtor, actorvar)

        replyvar = self.replyvar
        sendok, sendstmts = self.sendBlocking(md, msgvar, replyvar, actorvar)
        method.addstmts(
            stmts
            + [ Whitespace.NL,
                StmtDecl(Decl(Type('Message'), replyvar.name)) ]
            + sendstmts)

        destmts = self.deserializeReply(
            md, ExprAddrOf(replyvar), self.side, errfnSend)
        ifsendok = StmtIf(ExprLiteral.FALSE)
        ifsendok.addifstmts(destmts)
        ifsendok.addifstmts([ Whitespace.NL,
                              StmtExpr(ExprAssn(sendok, ExprLiteral.FALSE, '&=')) ])

        method.addstmts(
            [ ifsendok ]
            + self.dtorEpilogue(md, actor.var())
            + [ Whitespace.NL, StmtReturn(sendok) ])

        return method

    def destroyActor(self, md, actorexpr, why=_DestroyReason.Deletion):
        if md.decl.type.isCtor():
            destroyedType = md.decl.type.constructedType()
        else:
            destroyedType = self.protocol.decl.type
        return ([ StmtExpr(self.callActorDestroy(actorexpr, why)),
                  StmtExpr(self.callDeallocSubtree(md, actorexpr)),
                  StmtExpr(self.callRemoveActor(
                      actorexpr,
                      manager=self.protocol.managerVar(actorexpr),
                      ipdltype=destroyedType))
                ])

    def dtorPrologue(self, actorexpr):
        return [ self.failIfNullActor(actorexpr), Whitespace.NL ]

    def dtorEpilogue(self, md, actorexpr):
        return self.destroyActor(md, actorexpr)

    def genAsyncSendMethod(self, md):
        method = MethodDefn(self.makeSendMethodDecl(md))
        msgvar, stmts = self.makeMessage(md, errfnSend)
        sendok, sendstmts = self.sendAsync(md, msgvar)
        method.addstmts(stmts
                        +[ Whitespace.NL ]
                        + sendstmts
                        +[ StmtReturn(sendok) ])
        return method


    def genBlockingSendMethod(self, md, fromActor=None):
        method = MethodDefn(self.makeSendMethodDecl(md))

        msgvar, serstmts = self.makeMessage(md, errfnSend, fromActor)
        replyvar = self.replyvar

        sendok, sendstmts = self.sendBlocking(md, msgvar, replyvar)
        failif = StmtIf(ExprNot(sendok))
        failif.addifstmt(StmtReturn.FALSE)

        desstmts = self.deserializeReply(
            md, ExprAddrOf(replyvar), self.side, errfnSend)

        method.addstmts(
            serstmts
            + [ Whitespace.NL,
                StmtDecl(Decl(Type('Message'), replyvar.name)) ]
            + sendstmts
            + [ failif ]
            + desstmts
            + [ Whitespace.NL,
                StmtReturn.TRUE ])

        return method


    def genCtorRecvCase(self, md):
        lbl = CaseLabel(md.pqMsgId())
        case = StmtBlock()
        actorvar = md.actorDecl().var()
        actorhandle = self.handlevar

        stmts = self.deserializeMessage(md, self.side, errfnRecv)

        idvar, saveIdStmts = self.saveActorId(md)
        case.addstmts(
            stmts
            + self.transition(md, 'in')
            + [ StmtDecl(Decl(r.bareType(self.side), r.var().name))
                for r in md.returns ]
            # alloc the actor, register it under the foreign ID
            + [ StmtExpr(ExprAssn(
                actorvar,
                self.callAllocActor(md, retsems='in'))) ]
            + self.ctorPrologue(md, errfn=_Result.ValuError,
                                idexpr=_actorHId(actorhandle))
            + [ Whitespace.NL ]
            + saveIdStmts
            + self.invokeRecvHandler(md)
            + self.makeReply(md, errfnRecv, idvar)
            + [ Whitespace.NL,
                StmtReturn(_Result.Processed) ])

        return lbl, case


    def genDtorRecvCase(self, md):
        lbl = CaseLabel(md.pqMsgId())
        case = StmtBlock()

        stmts = self.deserializeMessage(md, self.side, errfnRecv)

        idvar, saveIdStmts = self.saveActorId(md)
        case.addstmts(
            stmts
            + self.transition(md, 'in')
            + [ StmtDecl(Decl(r.bareType(self.side), r.var().name))
                for r in md.returns ]
            + self.invokeRecvHandler(md, implicit=0)
            + [ Whitespace.NL ]
            + saveIdStmts
            + self.dtorEpilogue(md, md.actorDecl().var())
            + [ Whitespace.NL ]
            + self.makeReply(md, errfnRecv, routingId=idvar)
            + [ Whitespace.NL,
                StmtReturn(_Result.Processed) ])
        
        return lbl, case


    def genRecvCase(self, md):
        lbl = CaseLabel(md.pqMsgId())
        case = StmtBlock()

        stmts = self.deserializeMessage(md, self.side, errfn=errfnRecv)

        idvar, saveIdStmts = self.saveActorId(md)
        case.addstmts(
            stmts
            + self.transition(md, 'in')
            + [ StmtDecl(Decl(r.bareType(self.side), r.var().name))
                for r in md.returns ]
            + saveIdStmts
            + self.invokeRecvHandler(md)
            + [ Whitespace.NL ]
            + self.makeReply(md, errfnRecv, routingId=idvar)
            + [ StmtReturn(_Result.Processed) ])

        return lbl, case


    # helper methods

    def failIfNullActor(self, actorExpr, retOnNull=ExprLiteral.FALSE):
        failif = StmtIf(ExprNot(actorExpr))
        failif.addifstmt(StmtReturn(retOnNull))
        return failif

    def unregisterActor(self, actorexpr=None):
        return [ StmtExpr(ExprCall(self.protocol.unregisterMethod(actorexpr),
                                   args=[ _actorId(actorexpr) ])),
                 StmtExpr(ExprAssn(_actorId(actorexpr), _FREED_ACTOR_ID)) ]

    def makeMessage(self, md, errfn, fromActor=None):
        msgvar = self.msgvar
        routingId = self.protocol.routingId(fromActor)
        this = None
        if md.decl.type.isDtor():  this = md.actorDecl().var()

        stmts = ([ StmtDecl(Decl(Type(md.pqMsgClass(), ptr=1), msgvar.name),
                            init=ExprNew(Type(md.pqMsgClass()))) ]
                 + [ Whitespace.NL ]
                 + [ StmtExpr(self.write(p.ipdltype, p.var(), msgvar, this))
                     for p in md.params ]
                 + [ Whitespace.NL ]
                 + self.setMessageFlags(md, msgvar, reply=0,
                                        routingId=routingId))
        return msgvar, stmts


    def makeReply(self, md, errfn, routingId):
        # TODO special cases for async ctor/dtor replies
        if not md.decl.type.hasReply():
            return [ ]

        replyvar = self.replyvar
        return (
            [ StmtExpr(ExprAssn(
                replyvar, ExprNew(Type(md.pqReplyClass()), args=[ ]))),
              Whitespace.NL ]
            + [ StmtExpr(self.write(r.ipdltype, r.var(), replyvar))
                for r in md.returns ]
            + self.setMessageFlags(md, replyvar, reply=1, routingId=routingId)
            + [ self.logMessage(md, md.replyCast(replyvar), 'Sending reply ') ])


    def setMessageFlags(self, md, var, reply, routingId=None):
        if routingId is None:
            routingId = self.protocol.routingId()
        
        stmts = [ StmtExpr(ExprCall(
            ExprSelect(var, '->', 'set_routing_id'),
            args=[ routingId ])) ]

        if md.decl.type.isSync():
            stmts.append(StmtExpr(ExprCall(
                ExprSelect(var, '->', 'set_sync'))))
        elif md.decl.type.isRpc():
            stmts.append(StmtExpr(ExprCall(
                ExprSelect(var, '->', 'set_rpc'))))

        if reply:
            stmts.append(StmtExpr(ExprCall(
                ExprSelect(var, '->', 'set_reply'))))

        return stmts + [ Whitespace.NL ]


    def deserializeMessage(self, md, side, errfn):
        msgvar = self.msgvar
        itervar = self.itervar
        msgexpr = ExprAddrOf(msgvar)
        isctor = md.decl.type.isCtor()
        stmts = ([
            # this is kind of naughty, but the only two other options
            # are forwarding the message name (yuck) or making the
            # IPDL|*Channel abstraction leak more
            StmtExpr(ExprCall(
                ExprSelect(
                    ExprCast(msgvar, Type('Message', ref=1), const=1),
                    '.', 'set_name'),
                args=[ ExprLiteral.String(md.prettyMsgName(self.protocol.name
                                                           +'::')) ])),
            self.logMessage(md, md.msgCast(msgexpr), 'Received '),
            Whitespace.NL
        ])

        if 0 == len(md.params):
            return stmts

        start, decls, reads = 0, [], []
        if isctor:
            # return the raw actor handle so that its ID can be used
            # to construct the "real" actor
            handlevar = self.handlevar
            decls = [ StmtDecl(Decl(Type('ActorHandle'), handlevar.name)) ]
            reads = [ self.checkedRead(None, ExprAddrOf(handlevar), msgexpr,
                                       ExprAddrOf(self.itervar),
                                       errfn) ]
            start = 1

        stmts.extend((
            [ StmtDecl(Decl(Type.VOIDPTR, self.itervar.name),
                     init=ExprLiteral.NULL) ]
            + decls + [ StmtDecl(Decl(p.bareType(side), p.var().name))
                      for p in md.params ]
            + [ Whitespace.NL ]
            + reads + [ self.checkedRead(p.ipdltype, ExprAddrOf(p.var()),
                                         msgexpr, ExprAddrOf(itervar),
                                         errfn)
                        for p in md.params[start:] ]
            + [ self.endRead(msgvar, itervar) ]))

        return stmts


    def deserializeReply(self, md, replyexpr, side, errfn):
        stmts = [ Whitespace.NL,
                   self.logMessage(md, md.replyCast(replyexpr),
                                   'Received reply ') ]
        if 0 == len(md.returns):
            return stmts

        itervar = self.itervar
        stmts.extend(
            [ Whitespace.NL,
              StmtDecl(Decl(Type.VOIDPTR, itervar.name),
                       init=ExprLiteral.NULL) ]
            + [ self.checkedRead(r.ipdltype, r.var(),
                                 ExprAddrOf(self.replyvar),
                                 ExprAddrOf(self.itervar),
                                 errfn)
                for r in md.returns ]
            + [ self.endRead(self.replyvar, itervar) ])

        return stmts


    def sendAsync(self, md, msgexpr, actor=None):
        sendok = ExprVar('__sendok')
        return (
            sendok,
            ([ Whitespace.NL,
               self.logMessage(md, msgexpr, 'Sending ') ]
            + self.transition(md, 'out', actor)
            + [ Whitespace.NL,
                StmtDecl(Decl(Type.BOOL, sendok.name),
                         init=ExprCall(
                             ExprSelect(self.protocol.channelVar(actor),
                                        self.protocol.channelSel(), 'Send'),
                             args=[ msgexpr ]))
            ])
        )

    def sendBlocking(self, md, msgexpr, replyexpr, actor=None):
        sendok = ExprVar('__sendok')
        return (
            sendok,
            ([ Whitespace.NL,
               self.logMessage(md, msgexpr, 'Sending ') ]
            + self.transition(md, 'out', actor)
            + [ Whitespace.NL,
                StmtDecl(
                    Decl(Type.BOOL, sendok.name),
                    init=ExprCall(ExprSelect(self.protocol.channelVar(actor),
                                             self.protocol.channelSel(),
                                             _sendPrefix(md.decl.type)),
                                  args=[ msgexpr, ExprAddrOf(replyexpr) ]))
            ])
        )

    def callAllocActor(self, md, retsems):
        return ExprCall(
            _allocMethod(md.decl.type.constructedType()),
            args=md.makeCxxArgs(params=1, retsems=retsems, retcallsems='out',
                                implicit=0))

    def callActorDestroy(self, actorexpr, why=_DestroyReason.Deletion):
        return ExprCall(ExprSelect(actorexpr, '->', 'DestroySubtree'),
                        args=[ why ])

    def callRemoveActor(self, actorexpr, manager=None, ipdltype=None):
        if ipdltype is None: ipdltype = self.protocol.decl.type

        if not ipdltype.isManaged():
            return Whitespace('// unmanaged protocol')

        removefunc = self.protocol.removeManageeMethod()
        if manager is not None:
            removefunc = ExprSelect(manager, '->', removefunc.name)

        return ExprCall(removefunc,
                        args=[ _protocolId(ipdltype),
                               actorexpr ])

    def callDeallocSubtree(self, md, actorexpr):
        return ExprCall(ExprSelect(actorexpr, '->', 'DeallocSubtree'))

    def invokeRecvHandler(self, md, implicit=1):
        failif = StmtIf(ExprNot(
            ExprCall(md.recvMethod(),
                     args=md.makeCxxArgs(params=1,
                                         retsems='in', retcallsems='out',
                                         implicit=implicit))))
        failif.addifstmt(StmtReturn(_Result.ProcessingError))
        return [ failif ]

    def makeDtorMethodDecl(self, md):
        decl = self.makeSendMethodDecl(md)
        decl.static = 1
        return decl

    def makeSendMethodDecl(self, md):
        implicit = md.decl.type.hasImplicitActorParam()
        decl = MethodDecl(
            md.sendMethod().name,
            params=md.makeCxxParams(paramsems='in', returnsems='out',
                                    side=self.side, implicit=implicit),
            warn_unused=(self.side == 'parent'),
            ret=Type.BOOL)
        if md.decl.type.isCtor():
            decl.ret = md.actorDecl().bareType(self.side)
        return decl

    def logMessage(self, md, msgptr, pfx):
        actorname = _actorName(self.protocol.name, self.side)
        return _ifLogging([
            StmtExpr(ExprCall(
                ExprSelect(msgptr, '->', 'Log'),
                args=[ ExprLiteral.String('['+ actorname +'] '+ pfx),
                       ExprVar('stderr') ])) ])

    def saveActorId(self, md):
        idvar = ExprVar('__id')
        if md.decl.type.hasReply():
            # only save the ID if we're actually going to use it, to
            # avoid unused-variable warnings
            saveIdStmts = [ StmtDecl(Decl(_actorIdType(), idvar.name),
                                     self.protocol.routingId()) ]
        else:
            saveIdStmts = [ ]
        return idvar, saveIdStmts

    def transition(self, md, direction, actor=None):
        if actor is not None:  stateexpr = _actorState(actor)
        else:                  stateexpr = self.protocol.stateVar()
        
        if (self.side is 'parent' and direction is 'out'
            or self.side is 'child' and direction is 'in'):
            action = ExprVar('Trigger::Send')
        elif (self.side is 'parent' and direction is 'in'
            or self.side is 'child' and direction is 'out'):
            action = ExprVar('Trigger::Recv')
        else: assert 0 and 'unknown combo %s/%s'% (self.side, direction)

        ifbad = StmtIf(ExprNot(
            ExprCall(
                ExprVar(self.protocol.name +'::Transition'),
                args=[ stateexpr,
                       ExprCall(ExprVar('Trigger'),
                                args=[ action, ExprVar(md.pqMsgId()) ]),
                       ExprAddrOf(stateexpr) ])))
        ifbad.addifstmts(_badTransition())
        return [ ifbad ]

    def checkedRead(self, ipdltype, expr, msgexpr, iterexpr, errfn):
        ifbad = StmtIf(ExprNot(self.read(ipdltype, expr, msgexpr, iterexpr)))
        ifbad.addifstmts(errfn('error deserializing (better message TODO)'))
        return ifbad

    def endRead(self, msgexpr, iterexpr):
        return StmtExpr(ExprCall(ExprSelect(msgexpr, '.', 'EndRead'),
                                 args=[ iterexpr ]))

class _GenerateProtocolParentCode(_GenerateProtocolActorCode):
    def __init__(self):
        _GenerateProtocolActorCode.__init__(self, 'parent')

    def sendsMessage(self, md):
        return not md.decl.type.isIn()

    def receivesMessage(self, md):
        return md.decl.type.isInout() or md.decl.type.isIn()

class _GenerateProtocolChildCode(_GenerateProtocolActorCode):
    def __init__(self):
        _GenerateProtocolActorCode.__init__(self, 'child')

    def sendsMessage(self, md):
        return not md.decl.type.isOut()

    def receivesMessage(self, md):
        return md.decl.type.isInout() or md.decl.type.isOut()


##-----------------------------------------------------------------------------
## Utility passes
##

def _splitClassDeclDefn(cls, inlinedefns=0):
    """Destructively split |cls| methods into declarations and
definitions (if |not methodDecl.force_inline|).  Return classDecl,
methodDefns."""
    defns = Block()

    for i, stmt in enumerate(cls.stmts):
        if isinstance(stmt, MethodDefn) and not stmt.decl.force_inline:
            decl, defn = _splitMethodDefn(stmt, cls.name, inlinedefns)
            cls.stmts[i] = StmtDecl(decl)
            defns.addstmts([ defn, Whitespace.NL ])

    return cls, defns

def _splitMethodDefn(md, clsname, inlinedefn):
    saveddecl = deepcopy(md.decl)
    md.decl.name = (clsname +'::'+ md.decl.name)
    md.decl.virtual = 0
    md.decl.static = 0
    md.decl.warn_unused = 0
    md.decl.inline = inlinedefn
    for param in md.decl.params:
        if isinstance(param, Param):
            param.default = None
    return saveddecl, md


# XXX this is tantalizingly similar to _splitClassDeclDefn, but just
# different enough that I don't see the need to define
# _GenerateSkeleton in terms of that
class _GenerateSkeletonImpl(Visitor):
    def __init__(self, name, namespaces):
        self.name = name
        self.cls = None
        self.namespaces = namespaces
        self.methodimpls = Block()

    def fromclass(self, cls):
        cls.accept(self)

        nsclass = _putInNamespaces(self.cls, self.namespaces)
        nsmethodimpls = _putInNamespaces(self.methodimpls, self.namespaces)

        return [
            Whitespace('''
//-----------------------------------------------------------------------------
// Skeleton implementation of abstract actor class

'''),
            Whitespace('// Header file contents\n'),
            nsclass,
            Whitespace.NL,
            Whitespace('\n// C++ file contents\n'),
            nsmethodimpls
        ]


    def visitClass(self, cls):
        self.cls = Class(self.name, inherits=[ Inherit(Type(cls.name)) ])
        Visitor.visitClass(self, cls)

    def visitMethodDecl(self, md):
        if not md.pure:
            return
        decl = deepcopy(md)
        decl.pure = 0
        impl = MethodDefn(MethodDecl(self.implname(md.name),
                                             params=md.params,
                                             ret=md.ret))
        if md.ret.ptr:
            impl.addstmt(StmtReturn(ExprLiteral.ZERO))
        else:
            impl.addstmt(StmtReturn(ExprVar('false')))

        self.cls.addstmts([ StmtDecl(decl), Whitespace.NL ])
        self.addmethodimpl(impl)

    def visitConstructorDecl(self, cd):
        self.cls.addstmt(StmtDecl(ConstructorDecl(self.name)))
        ctor = ConstructorDefn(ConstructorDecl(self.implname(self.name)))
        ctor.addstmt(StmtExpr(ExprCall(ExprVar( 'MOZ_COUNT_CTOR'),
                                               [ ExprVar(self.name) ])))
        self.addmethodimpl(ctor)
        
    def visitDestructorDecl(self, dd):
        self.cls.addstmt(
            StmtDecl(DestructorDecl(self.name, virtual=1)))
        # FIXME/cjones: hack!
        dtor = DestructorDefn(ConstructorDecl(self.implname('~' +self.name)))
        dtor.addstmt(StmtExpr(ExprCall(ExprVar( 'MOZ_COUNT_DTOR'),
                                               [ ExprVar(self.name) ])))
        self.addmethodimpl(dtor)

    def addmethodimpl(self, impl):
        self.methodimpls.addstmts([ impl, Whitespace.NL ])

    def implname(self, method):
        return self.name +'::'+ method
