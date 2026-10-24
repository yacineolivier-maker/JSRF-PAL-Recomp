#include "app/app.h"
#include "core/input.h"

namespace game {
void StartHangWatch();                     // hang_watch.cpp (repris de Conker)
void AutoPad(int port, input::Pad& pad);  // autopad.cpp (repris de Conker)
extern bool g_ecran_large;                // ecran_large.cpp
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
    .options = {{"ecran_large", &game::g_ecran_large, nullptr}},
    .load = nullptr,
    .save = nullptr,
    .init = game::StartHangWatch,
    .loaded = nullptr,
    .window = nullptr,
    .message = nullptr,
    .ui = nullptr,
    .pad = game::AutoPad,
    .rumble = nullptr,
};
