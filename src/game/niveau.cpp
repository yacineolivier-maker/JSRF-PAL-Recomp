// Niveaux : chaque demande de chargement (sub_00036720, l'identifiant (zone << 16) | sous-niveau, Stg%1d%1d) est
// journalisée. Tournée des niveaux sans jouer jusqu'à eux :
//   XBE_NIVEAU=<zone><sous-niveau> (ex. 11 pour Stg11) remplace chaque demande du niveau XBE_NIVEAU_DE
//   (10 par défaut : la démo de l'écran titre, qui montre un vrai niveau avec un personnage).
#include <cstdlib>

#include "core/log.h"
#include "kernel/kernel.h"
#include "xdk/hle.h"

namespace {
uint32_t StageId(int v) { return uint32_t(v / 10) << 16 | uint32_t(v % 10); }
}  // namespace

X86_EXTERN_FUNC(__imp__sub_00036720);
X86_FUNC(sub_00036720) {
    uint32_t id = kernel::R32(ctx.esp + 4);
    if (const char* n = std::getenv("XBE_NIVEAU")) {
        const char* from = std::getenv("XBE_NIVEAU_DE");
        if (id == StageId(from ? std::atoi(from) : 10)) {
            uint32_t want = StageId(std::atoi(n));
            kernel::W32(ctx.esp + 4, want);
            LOGI("niveau : Stg%u%u demandé, Stg%u%u chargé à la place (XBE_NIVEAU)", (id >> 16) % 10, (id & 0xFFFF) % 10,
                 (want >> 16) % 10, (want & 0xFFFF) % 10);
            id = want;
        }
    }
    LOGI("niveau : chargement de Stg%u%u (%08X) depuis %08X", (id >> 16) % 10, (id & 0xFFFF) % 10, id, kernel::R32(ctx.esp));
    __imp__sub_00036720(ctx, base);
}
