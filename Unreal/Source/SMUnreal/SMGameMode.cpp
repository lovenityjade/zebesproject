#include "SMGameMode.h"
#include "SMHUD.h"
ASMGameMode::ASMGameMode() {
    HUDClass = ASMHUD::StaticClass();
    DefaultPawnClass = nullptr;
}
