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
extern bool g_bloom, g_etalonnage;
extern uint32_t g_bloom_force, g_bloom_seuil, g_vibrance, g_contraste;
extern bool g_reflets, g_rayons;
extern uint32_t g_reflets_force, g_rayons_force, g_particules;
void InstallRendu();
bool RenduKey(uint32_t vk);
// options.cpp : panneau F1
bool OptionsKey(uint32_t vk);
void OptionsUi(double fps);

// rendu à 175 % de la hauteur de l'écran par défaut (150 dans le SDK), avant la lecture de settings.toml : la scène
// de JSRF est légère, le suréchantillonnage en plus affine ses contours cel-shadés
[[maybe_unused]] const bool kRenduParDefaut = (app::Get().render_percent = 175, true);

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
                {"bloom", &game::g_bloom, nullptr},
                {"bloom_force", nullptr, &game::g_bloom_force},
                {"bloom_seuil", nullptr, &game::g_bloom_seuil},
                {"etalonnage", &game::g_etalonnage, nullptr},
                {"vibrance", nullptr, &game::g_vibrance},
                {"contraste", nullptr, &game::g_contraste},
                {"reflets_objectif", &game::g_reflets, nullptr},
                {"reflets_force", nullptr, &game::g_reflets_force},
                {"rayons_lumiere", &game::g_rayons, nullptr},
                {"rayons_force", nullptr, &game::g_rayons_force},
                {"particules_eclat", nullptr, &game::g_particules},
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
