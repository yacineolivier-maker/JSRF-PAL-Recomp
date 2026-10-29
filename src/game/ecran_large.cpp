// Écran large « Hor+ » : le jeu calcule sa projection pour du 4:3 (sub_00154920 : m11 = s * 640 / 480, m00 copié de
// 0x234F58). Sur une sortie 16:9 (ou plus large), m00 est réduit dans le même rapport le temps du calcul : même champ
// vertical, plus de décor sur les côtés. Option ecran_large (settings.toml), active par défaut.
#include <cstring>
#include <unordered_map>

#include "app/app.h"
#include "gpu/backend.h"
#include "kernel/kernel.h"
#include "xdk/hle.h"

namespace game {
bool g_ecran_large = true;
}

namespace {
// largeur de la sortie sur hauteur, 4:3 au moins ; 0 quand l'écran large est coupé. L'image est présentée dans ce
// rapport : toute la largeur d'un écran 21:9 avec l'écran large, un cadre 4:3 avec bandes noires sans lui
float WideAspect() {
    if (!game::g_ecran_large || !app::Widescreen()) {
        gpu::g_hud_x_scale = 1.0f;
        gpu::g_present_aspect = 4.0f / 3.0f;
        return 0;
    }
    const app::Mode out = app::Output();
    float wide = out.height ? float(out.width) / float(out.height) : 16.0f / 9.0f;
    wide = wide < 4.0f / 3.0f ? 4.0f / 3.0f : wide;
    // l'interface 2D garde ses proportions : ramenée au cadre 4:3 du centre
    gpu::g_hud_x_scale = (4.0f / 3.0f) / wide;
    gpu::g_present_aspect = wide;
    return wide;
}
}  // namespace

// La focale en pixels de la caméra (+0x50, 320 pour 640 de large, recopiée en 0x26B7A8 et -focale * +0x60 en
// 0x26B7B0 par sub_00154A40) sert au frustum (sub_001544F0) et au test d'écran des objets : réduite dans le même
// rapport que m00, le décor ajouté sur les côtés n'est plus écarté avant d'être dessiné, et les repères 2D tombent
// juste. Appliqué à l'entrée de sub_001544F0 ; une focale déjà réduite (même valeur que la dernière écrite) reste.
X86_EXTERN_FUNC(__imp__sub_001544F0);
X86_FUNC(sub_001544F0) {
    float wide = WideAspect();
    uint32_t cam = ctx.ecx;
    if (wide && cam) {
        static std::unordered_map<uint32_t, uint32_t> written;
        uint32_t bits = kernel::R32(cam + 0x50);
        auto it = written.find(cam);
        if (it == written.end() || it->second != bits) {
            float k = (4.0f / 3.0f) / wide, f;
            std::memcpy(&f, &bits, 4);
            auto scale = [&](uint32_t va) {
                uint32_t b = kernel::R32(va);
                float v;
                std::memcpy(&v, &b, 4);
                v *= k;
                std::memcpy(&b, &v, 4);
                kernel::W32(va, b);
            };
            if (kernel::R32(0x26B7A8) == bits) {
                scale(0x26B7A8);
                scale(0x26B7B0);
            }
            scale(cam + 0x50);
            written[cam] = kernel::R32(cam + 0x50);
        }
    }
    __imp__sub_001544F0(ctx, base);
}

X86_EXTERN_FUNC(__imp__sub_00154920);
X86_FUNC(sub_00154920) {
    constexpr uint32_t kM00 = 0x234F58;
    float wide = WideAspect();
    if (!wide) {
        __imp__sub_00154920(ctx, base);
        return;
    }
    uint32_t bits = kernel::R32(kM00);
    float m00;
    std::memcpy(&m00, &bits, 4);
    float scaled = m00 * (4.0f / 3.0f) / wide;
    uint32_t sbits;
    std::memcpy(&sbits, &scaled, 4);
    kernel::W32(kM00, sbits);
    __imp__sub_00154920(ctx, base);
    kernel::W32(kM00, bits);
}
