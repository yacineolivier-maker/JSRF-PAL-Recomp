// Diagnostics du jeu, actifs seulement avec XBE_WATCH_ADDR=<adresse de données> : les enregistrements, créations et
// destructions de ressources D3D (XDK 4134) qui touchent cette adresse, avec l'adresse de retour dans le jeu.
#include <cstdlib>

#include "core/log.h"
#include "kernel/kernel.h"
#include "xdk/hle.h"

namespace {
uint32_t Watch() {
    static const uint32_t w = std::getenv("XBE_WATCH_ADDR") ? uint32_t(std::strtoul(std::getenv("XBE_WATCH_ADDR"), nullptr, 16)) : 0;
    return w & 0x0FFFFFFF;
}
bool Near(uint32_t data) { return Watch() && (data & 0x0FFFFFFF) - Watch() + 0x10000 < 0x20000; }
}  // namespace

// D3DResource_Register(resource, base) : Data += base
X86_EXTERN_FUNC(__imp__sub_001934A0);
X86_FUNC(sub_001934A0) {
    uint32_t res = kernel::R32(ctx.esp + 4), b = kernel::R32(ctx.esp + 8), ret = kernel::R32(ctx.esp);
    uint32_t before = kernel::R32(res + 4);
    __imp__sub_001934A0(ctx, base);
    if (Near(kernel::R32(res + 4)))
        LOGI("watch: Register(%08X, base %08X) data %08X -> %08X common %08X format %08X from %08X", res, b, before,
             kernel::R32(res + 4), kernel::R32(res), kernel::R32(res + 12), ret);
}

// destruction d'une ressource dont plus rien ne tient (appelée par SetTexture et Release du 4134)
X86_EXTERN_FUNC(__imp__sub_001932F0);
X86_FUNC(sub_001932F0) {
    uint32_t res = kernel::R32(ctx.esp + 4), ret = kernel::R32(ctx.esp);
    if (Near(kernel::R32(res + 4)))
        LOGI("watch: destroy %08X data %08X common %08X from %08X", res, kernel::R32(res + 4), kernel::R32(res), ret);
    __imp__sub_001932F0(ctx, base);
}

// D3DResource_Release
X86_EXTERN_FUNC(__imp__sub_00193450);
X86_FUNC(sub_00193450) {
    uint32_t res = kernel::R32(ctx.esp + 4), ret = kernel::R32(ctx.esp);
    if (Near(kernel::R32(res + 4)))
        LOGI("watch: Release %08X data %08X common %08X from %08X", res, kernel::R32(res + 4), kernel::R32(res), ret);
    __imp__sub_00193450(ctx, base);
}

// D3DDevice_CreateTexture(width, height, levels, usage, format, pool, out)
X86_EXTERN_FUNC(__imp__sub_0018D6B0);
X86_FUNC(sub_0018D6B0) {
    uint32_t w = kernel::R32(ctx.esp + 4), h = kernel::R32(ctx.esp + 8), fmt = kernel::R32(ctx.esp + 0x14);
    uint32_t out = kernel::R32(ctx.esp + 0x1C), ret = kernel::R32(ctx.esp);
    __imp__sub_0018D6B0(ctx, base);
    uint32_t t = out ? kernel::R32(out) : 0;
    if (Watch() && t)
        LOGI("watch: CreateTexture %ux%u format %X = %08X data %08X from %08X", w, h, fmt, t, kernel::R32(t + 4), ret);
}
