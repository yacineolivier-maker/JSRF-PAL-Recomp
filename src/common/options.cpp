// Panneau d'options en jeu (F1) : les réglages du rendu modernisé, de l'affichage et du clavier-souris, appliqués aussitôt,
// enregistrés dans settings.toml par le bouton du panneau. Souris et clavier ; la manette reste au jeu.
#include <imgui.h>

#include <cstdint>
#include <iterator>

#include "app/app.h"

namespace game {

// rendu.cpp
extern bool g_smaa, g_occlusion, g_ombres_douces;
extern uint32_t g_occlusion_rayon, g_occlusion_force, g_nettete, g_ombres_taille;
extern bool g_bloom, g_etalonnage;
extern uint32_t g_bloom_force, g_bloom_seuil, g_vibrance, g_contraste;
extern bool g_reflets, g_rayons;
extern uint32_t g_reflets_force, g_rayons_force, g_particules;
// ecran_large.cpp
extern bool g_ecran_large;
// clavier_souris.cpp
extern bool g_clavier_souris, g_souris_inverser_y;
extern uint32_t g_souris_sensibilite;

namespace {

bool g_open = false;
const char* g_saved = nullptr;  // message après l'enregistrement
double g_saved_until = 0;

// une valeur entière bornée, réglée par une glissière
void Slider(const char* label, uint32_t& v, int lo, int hi, const char* fmt) {
    int x = int(v);
    if (ImGui::SliderInt(label, &x, lo, hi, fmt, ImGuiSliderFlags_AlwaysClamp)) v = uint32_t(x);
}

// le texte du panneau dans la langue du jeu : français, sinon anglais
bool g_fr = true;
const char* T(const char* fr, const char* en) { return g_fr ? fr : en; }

// un choix parmi des valeurs fixes
void Choice(const char* label, uint32_t& v, const uint32_t* values, const char* const* names, int n) {
    int at = 0;
    for (int i = 0; i < n; i++)
        if (values[i] == v) at = i;
    if (ImGui::Combo(label, &at, names, n)) v = values[at];
}

}  // namespace

bool OptionsOpen() { return g_open; }

bool OptionsKey(uint32_t vk) {
    constexpr uint32_t kF1 = 0x70;  // VK_F1
    if (vk != kF1) return false;
    g_open = !g_open;
    return true;
}

void OptionsUi(double fps) {
    if (!g_open) return;
    app::Settings& s = app::Get();
    g_fr = s.language == "francais";
    ImGui::SetNextWindowPos(ImVec2(48, 48), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(T("Options graphiques (F1)###options", "Graphics options (F1)###options"), &g_open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }
    ImGui::SetWindowFontScale(1.6f);
    ImGui::Text(T("%.0f images par seconde", "%.0f frames per second"), fps);
    ImGui::Separator();

    ImGui::TextUnformatted(T("Lumière et couleurs", "Light and colour"));
    ImGui::Checkbox(T("Halo lumineux (bloom)", "Bloom"), &g_bloom);
    if (g_bloom) {
        Slider(T("Intensité du halo", "Bloom strength"), g_bloom_force, 0, 100, "%d %%");
        Slider(T("Seuil du halo", "Bloom threshold"), g_bloom_seuil, 40, 100, "%d %%");
    }
    ImGui::Checkbox(T("Reflets d'objectif (lens flare)", "Lens flare"), &g_reflets);
    if (g_reflets) Slider(T("Intensité des reflets", "Lens flare strength"), g_reflets_force, 0, 100, "%d %%");
    ImGui::Checkbox(T("Rayons de lumière", "Light shafts"), &g_rayons);
    if (g_rayons) Slider(T("Intensité des rayons", "Light shaft strength"), g_rayons_force, 0, 100, "%d %%");
    Slider(T("Éclat des particules", "Particle glow"), g_particules, 0, 100, "+%d %%");
    ImGui::Checkbox(T("Étalonnage des couleurs", "Colour grading"), &g_etalonnage);
    if (g_etalonnage) {
        Slider(T("Vibrance", "Vibrance"), g_vibrance, 0, 60, "%d %%");
        Slider(T("Contraste", "Contrast"), g_contraste, 0, 50, "%d %%");
    }
    ImGui::Separator();

    ImGui::TextUnformatted(T("Ombres et lumière (expérimental)", "Shadows and lighting (experimental)"));
    ImGui::Checkbox(T("Ombres douces", "Soft shadows"), &g_ombres_douces);
    // prise à la création des textures d'ombre : au prochain lancement
    static const uint32_t kShadowSizes[] = {0, 512, 1024, 2048};
    const char* const kShadowSizeNames[] = {T("Celle du jeu", "Game default"), "512", "1024", "2048"};
    Choice(T("Taille des ombres (relancer)", "Shadow size (restart)"), g_ombres_taille, kShadowSizes, kShadowSizeNames, int(std::size(kShadowSizes)));
    ImGui::Checkbox(T("Occlusion ambiante (GTAO)", "Ambient occlusion (GTAO)"), &g_occlusion);
    if (g_occlusion) {
        Slider(T("Portée", "Radius"), g_occlusion_rayon, 5, 200, "%d");
        Slider(T("Intensité", "Strength"), g_occlusion_force, 0, 200, "%d %%");
    }
    ImGui::Separator();

    ImGui::TextUnformatted(T("Image", "Picture"));
    ImGui::Checkbox(T("Anticrénelage SMAA", "SMAA anti-aliasing"), &g_smaa);
    Slider(T("Netteté", "Sharpening"), g_nettete, 0, 100, "%d %%");
    static const uint32_t kAniso[] = {0, 2, 4, 8, 16};
    const char* const kAnisoNames[] = {T("Au choix du jeu", "Game default"), "2x", "4x", "8x", "16x"};
    Choice(T("Filtrage anisotrope", "Anisotropic filtering"), s.aniso, kAniso, kAnisoNames, int(std::size(kAniso)));
    Slider(T("Résolution du rendu", "Render resolution"), s.render_percent, 50, 200, "%d %%");
    ImGui::Separator();

    ImGui::TextUnformatted(T("Affichage", "Display"));
    ImGui::Checkbox(T("Écran large (16:9 et plus, champ élargi)", "Widescreen (16:9 and wider, wider view)"), &g_ecran_large);
    static const char* const kLangues[] = {"anglais", "francais", "allemand", "espagnol", "italien"};
    static const char* const kLanguesNoms[] = {"English", "Français", "Deutsch", "Español", "Italiano"};
    int langue = 1;
    for (int i = 0; i < 5; i++)
        if (s.language == kLangues[i]) langue = i;
    if (ImGui::Combo(T("Langue du jeu (relancer)", "Game language (restart)"), &langue, kLanguesNoms, 5)) s.language = kLangues[langue];
    ImGui::Checkbox(T("Synchronisation verticale", "Vertical sync"), &s.vsync);
    static const uint32_t kCaps[] = {30, 60, 120, 144, app::kFpsCapRefresh, 0};
    const char* const kCapNames[] = {"30", "60", "120", "144", T("Écran", "Display"), T("Sans limite", "Unlimited")};  // Écran : juste sous sa fréquence
    Choice(T("Images par seconde au plus", "Frame rate cap"), s.fps_cap, kCaps, kCapNames, int(std::size(kCaps)));
    ImGui::Separator();

    ImGui::TextUnformatted(T("Commandes", "Controls"));
    ImGui::Checkbox(T("Clavier et souris (manette 1)", "Keyboard and mouse (controller 1)"), &g_clavier_souris);
    if (g_clavier_souris) {
        Slider(T("Sensibilité de la souris", "Mouse sensitivity"), g_souris_sensibilite, 10, 400, "%d %%");
        ImGui::Checkbox(T("Inverser la souris verticalement", "Invert mouse Y"), &g_souris_inverser_y);
        ImGui::TextDisabled("%s", T("Touches : table [clavier_souris] de settings.toml", "Keys: [clavier_souris] table in settings.toml"));
    }
    ImGui::Separator();

    if (ImGui::Button(T("Enregistrer", "Save"))) {
        app::Save();
        g_saved = T("Réglages enregistrés dans settings.toml", "Settings saved to settings.toml");
        g_saved_until = ImGui::GetTime() + 3;
    }
    ImGui::SameLine();
    if (ImGui::Button(T("Fermer", "Close"))) g_open = false;
    if (g_saved && ImGui::GetTime() < g_saved_until) ImGui::TextUnformatted(g_saved);
    ImGui::TextDisabled("%s", T("F7 SMAA, F8 occlusion, F9 ombres douces", "F7 SMAA, F8 occlusion, F9 soft shadows"));
    ImGui::End();
}

}  // namespace game
