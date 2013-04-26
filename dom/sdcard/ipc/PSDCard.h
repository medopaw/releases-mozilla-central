//
// Automatically generated by ipdlc.
// Edit at your own risk
//

#ifndef PSDCard_h
#define PSDCard_h

#include "mozilla/Attributes.h"
#include "base/basictypes.h"
#include "prtime.h"
#include "nscore.h"
#include "IPCMessageStart.h"
#include "ipc/IPCMessageUtils.h"
#include "nsAutoPtr.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "nsIFile.h"
#include "mozilla/ipc/ProtocolUtils.h"
#include "GeckoProfiler.h"

//-----------------------------------------------------------------------------
// Code common to PSDCardChild and PSDCardParent
//
namespace mozilla {
namespace dom {
namespace sdcard {
namespace PSDCard {

enum State {
    __Dead,
    __Null,
    __Error,
    __Dying,
    __Start = __Null
};

enum MessageType {
    PSDCardStart = PSDCardMsgStart << 16,
    PSDCardPreStart = (PSDCardMsgStart << 16) - 1,
    Msg_OnVoidResult__ID,
    Msg_OnError__ID,
    Msg___delete____ID,
    Reply___delete____ID,
    Msg_Remove__ID,
    PSDCardEnd
};

bool
Transition(
        State from,
        mozilla::ipc::Trigger trigger,
        State* next);

class Msg_OnVoidResult :
    public IPC::Message
{
private:
    typedef mozilla::ipc::ActorHandle ActorHandle;
    typedef mozilla::ipc::FileDescriptor FileDescriptor;
    typedef mozilla::ipc::Shmem Shmem;

public:
    enum {
        ID = Msg_OnVoidResult__ID
    };
    Msg_OnVoidResult() :
        IPC::Message(MSG_ROUTING_NONE, ID, PRIORITY_NORMAL, COMPRESSION_NONE, "PSDCard::Msg_OnVoidResult")
    {
    }

    void
    Log(
            const std::string& __pfx,
            FILE* __outf) const
    {
        std::string __logmsg;
        StringAppendF((&(__logmsg)), "[time:%" PRId64 "][%d]", PR_Now(), base::GetCurrentProcId());
        (__logmsg).append(__pfx);
        (__logmsg).append("Msg_OnVoidResult(");

        (__logmsg).append("[TODO])\n");
        fputs((__logmsg).c_str(), __outf);
    }
};

class Msg_OnError :
    public IPC::Message
{
private:
    typedef mozilla::ipc::ActorHandle ActorHandle;
    typedef mozilla::ipc::FileDescriptor FileDescriptor;
    typedef mozilla::ipc::Shmem Shmem;

public:
    enum {
        ID = Msg_OnError__ID
    };
    Msg_OnError() :
        IPC::Message(MSG_ROUTING_NONE, ID, PRIORITY_NORMAL, COMPRESSION_NONE, "PSDCard::Msg_OnError")
    {
    }

    void
    Log(
            const std::string& __pfx,
            FILE* __outf) const
    {
        std::string __logmsg;
        StringAppendF((&(__logmsg)), "[time:%" PRId64 "][%d]", PR_Now(), base::GetCurrentProcId());
        (__logmsg).append(__pfx);
        (__logmsg).append("Msg_OnError(");

        (__logmsg).append("[TODO])\n");
        fputs((__logmsg).c_str(), __outf);
    }
};

class Msg___delete__ :
    public IPC::Message
{
private:
    typedef mozilla::ipc::ActorHandle ActorHandle;
    typedef mozilla::ipc::FileDescriptor FileDescriptor;
    typedef mozilla::ipc::Shmem Shmem;

public:
    enum {
        ID = Msg___delete____ID
    };
    Msg___delete__() :
        IPC::Message(MSG_ROUTING_NONE, ID, PRIORITY_NORMAL, COMPRESSION_NONE, "PSDCard::Msg___delete__")
    {
    }

    void
    Log(
            const std::string& __pfx,
            FILE* __outf) const
    {
        std::string __logmsg;
        StringAppendF((&(__logmsg)), "[time:%" PRId64 "][%d]", PR_Now(), base::GetCurrentProcId());
        (__logmsg).append(__pfx);
        (__logmsg).append("Msg___delete__(");

        (__logmsg).append("[TODO])\n");
        fputs((__logmsg).c_str(), __outf);
    }
};

class Reply___delete__ :
    public IPC::Message
{
private:
    typedef mozilla::ipc::ActorHandle ActorHandle;
    typedef mozilla::ipc::FileDescriptor FileDescriptor;
    typedef mozilla::ipc::Shmem Shmem;

public:
    enum {
        ID = Reply___delete____ID
    };
    Reply___delete__() :
        IPC::Message(MSG_ROUTING_NONE, ID, PRIORITY_NORMAL, COMPRESSION_NONE, "PSDCard::Reply___delete__")
    {
    }

    void
    Log(
            const std::string& __pfx,
            FILE* __outf) const
    {
        std::string __logmsg;
        StringAppendF((&(__logmsg)), "[time:%" PRId64 "][%d]", PR_Now(), base::GetCurrentProcId());
        (__logmsg).append(__pfx);
        (__logmsg).append("Reply___delete__(");

        (__logmsg).append("[TODO])\n");
        fputs((__logmsg).c_str(), __outf);
    }
};

class Msg_Remove :
    public IPC::Message
{
private:
    typedef mozilla::ipc::ActorHandle ActorHandle;
    typedef mozilla::ipc::FileDescriptor FileDescriptor;
    typedef mozilla::ipc::Shmem Shmem;

public:
    enum {
        ID = Msg_Remove__ID
    };
    Msg_Remove() :
        IPC::Message(MSG_ROUTING_NONE, ID, PRIORITY_NORMAL, COMPRESSION_NONE, "PSDCard::Msg_Remove")
    {
    }

    void
    Log(
            const std::string& __pfx,
            FILE* __outf) const
    {
        std::string __logmsg;
        StringAppendF((&(__logmsg)), "[time:%" PRId64 "][%d]", PR_Now(), base::GetCurrentProcId());
        (__logmsg).append(__pfx);
        (__logmsg).append("Msg_Remove(");

        (__logmsg).append("[TODO])\n");
        fputs((__logmsg).c_str(), __outf);
    }
};



} // namespace PSDCard
} // namespace sdcard
} // namespace dom
} // namespace mozilla

#endif // ifndef PSDCard_h
