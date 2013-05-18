#include "SDCardRequestParent.h"

namespace mozilla {
namespace dom {
namespace sdcard {

SDCardRequestParent::SDCardRequestParent()
{
    MOZ_COUNT_CTOR(SDCardRequestParent);
}

SDCardRequestParent::~SDCardRequestParent()
{
    MOZ_COUNT_DTOR(SDCardRequestParent);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
