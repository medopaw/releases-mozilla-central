#pragma once

#include "mozilla/dom/sdcard/PSDCardRequestChild.h"

namespace mozilla {
namespace dom {
namespace sdcard {

class SDCardRequestChild :
    public PSDCardRequestChild
{
public:
    SDCardRequestChild();
    virtual ~SDCardRequestChild();
};

} // namespace sdcard
} // namespace dom
} // namespace mozilla
