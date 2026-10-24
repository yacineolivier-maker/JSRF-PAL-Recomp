#include "app/app.h"

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
    .options = {},
    .load = nullptr,
    .save = nullptr,
    .init = nullptr,
    .loaded = nullptr,
    .window = nullptr,
    .message = nullptr,
    .ui = nullptr,
    .pad = nullptr,
    .rumble = nullptr,
};
