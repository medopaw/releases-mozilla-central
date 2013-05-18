#include "SDCardRequestChild.h"

namespace mozilla {
namespace dom {
namespace sdcard {

SDCardRequestChild::SDCardRequestChild()
{
    MOZ_COUNT_CTOR(SDCardRequestChild);
}

SDCardRequestChild::~SDCardRequestChild()
{
    MOZ_COUNT_DTOR(SDCardRequestChild);
}

} // namespace sdcard
} // namespace dom
} // namespace mozilla
