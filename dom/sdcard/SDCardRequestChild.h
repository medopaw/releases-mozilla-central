#pragma once

#include "mozilla/dom/sdcard/PSDCardRequestChild.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class SDCardRequestChild :
    public PSDCardRequestChild
{
    SDCardRequestChild();
    virtual ~SDCardRequestChild();
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
