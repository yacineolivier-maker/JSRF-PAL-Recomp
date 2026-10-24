#include "app/app.h"
#include "core/input.h"

namespace game {
void StartHangWatch();                     // hang_watch.cpp (repris de Conker)
void AutoPad(int port, input::Pad& pad);  // autopad.cpp (repris de Conker)
extern bool g_ecran_large;                // ecran_large.cpp
// clavier_souris.cpp (repris de Conker)
void ClavierCharger(const void* table);
void ClavierEnregistrer(void* table);
void ClavierFenetre(void* hwnd);
bool ClavierMessage(uint32_t msg, uintptr_t wp, intptr_t lp, intptr_t* result);
void ClavierSouris(int port, input::Pad& pad);
// rendu.cpp (repris de Conker : SMAA et netteté ; ombres douces et occlusion coupées par défaut)
extern bool g_smaa, g_occlusion, g_ombres_douces;
extern uint32_t g_occlusion_rayon, g_occlusion_force, g_nettete, g_ombres_taille;
void InstallRendu();
bool RenduKey(uint32_t vk);
// options.cpp : panneau F1
bool OptionsKey(uint32_t vk);
void OptionsUi(double fps);

void Pad(int port, input::Pad& pad) {
    AutoPad(port, pad);
    ClavierSouris(port, pad);
}

void Init() {
    StartHangWatch();
    InstallRendu();
}

bool Message(void*, uint32_t msg, uintptr_t wp, intptr_t lp, intptr_t* result) {
    if (ClavierMessage(msg, wp, lp, result)) return true;
    constexpr uint32_t kKeyDown = 0x0100;  // WM_KEYDOWN
    if (msg != kKeyDown || !(RenduKey(uint32_t(wp)) || OptionsKey(uint32_t(wp)))) return false;
    *result = 0;
    return true;
}
}

// ce que le runtime demande au jeu ; les chaînes sont obligatoires, les hooks peuvent rester nuls
const app::GameInfo app::kGame = {
    .title = L"Jet Set Radio Future",
    .short_title = L"JSRF",
    .name = L"JSRF",
    .log = "jsrf",
    .settings = "settings.toml",
    .splash = L"splash",
    .missing = L"Placez le jeu dans le dossier assets : l'image disque (.iso) ou default.xbe avec ses dossiers.",
    .images = {{"default.xbe", "JSRF"}},
    .options = {{"ecran_large", &game::g_ecran_large, nullptr},
                {"smaa", &game::g_smaa, nullptr},
                {"nettete", nullptr, &game::g_nettete},
                {"ombres_douces", &game::g_ombres_douces, nullptr},
                {"ombres_taille", nullptr, &game::g_ombres_taille},
                {"occlusion_ambiante", &game::g_occlusion, nullptr},
                {"occlusion_rayon", nullptr, &game::g_occlusion_rayon},
                {"occlusion_force", nullptr, &game::g_occlusion_force}},
    .load = game::ClavierCharger,
    .save = game::ClavierEnregistrer,
    .init = game::Init,
    .loaded = nullptr,
    .window = game::ClavierFenetre,
    .message = game::Message,
    .ui = game::OptionsUi,
    .pad = game::Pad,
    .rumble = nullptr,
};
