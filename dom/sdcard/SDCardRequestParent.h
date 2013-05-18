#pragma once

#include "mozilla/dom/sdcard/PSDCardRequestParent.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class SDCardRequestParent :
    public PSDCardRequestParent
{
    SDCardRequestParent();
    virtual ~SDCardRequestParent();
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
