#pragma once
// Shaders du rendu modernisé (rendu.cpp) : occlusion ambiante GTAO tirée de la profondeur de la scène, puis SMAA sur
// l'image présentée. Chaque passe a son propre point d'entrée vs, comme Context::Pass le compile.

namespace game {

// b1 : proj (1 / échelle xy, centre xy), zp (distance = x / (profondeur - y)), vp (vue en pixels de la cible),
// opt (rayon en unités du monde, plus grand écart en pixels, pas du flou xy), fade (début, fin de l'atténuation avec
// la distance, force, 0)
inline constexpr char kAoHlsl[] = R"(
cbuffer A : register(b1) { float4 proj; float4 zp; float4 vp; float4 opt; float4 fade; };
Texture2D<float> depth : register(t0);
Texture2D<float2> ao : register(t1);
Texture2D color : register(t2);
float Raw(int2 p) { return depth.Load(int3(p, 0)); }
float4 vs(uint id : SV_VertexID) : SV_Position {
    float2 t = float2((id << 1) & 2, id & 2);
    return float4(t * float2(2, -2) + float2(-1, 1), 0, 1);
}
bool Inside(float2 px) { return all(px >= vp.xy) && all(px < vp.xy + vp.zw); }
// position d'un pixel dans l'espace de la caméra, z = distance devant elle
float3 View(float2 px) {
    int2 p = clamp(int2(px), int2(vp.xy), int2(vp.xy + vp.zw) - 1);
    float z = zp.x / (Raw(p) - zp.y);
    float2 ndc = (p + 0.5 - vp.xy) / vp.zw * float2(2, -2) + float2(-1, 1);
    return float3((ndc - proj.zw) * proj.xy * z, z);
}
// la normale prise du voisin le plus proche de chaque côté : pas de pli aux bords de profondeur
float3 Normal(float2 pos, float3 c) {
    float3 l = View(pos - float2(1, 0));
    float3 r = View(pos + float2(1, 0));
    float3 u = View(pos - float2(0, 1));
    float3 d = View(pos + float2(0, 1));
    float3 dx = abs(r.z - c.z) < abs(c.z - l.z) ? r - c : c - l;
    float3 dy = abs(d.z - c.z) < abs(c.z - u.z) ? d - c : c - u;
    return normalize(cross(dx, dy));
}
static const float kDither[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
static const float kHalfPi = 1.5707963;
// GTAO (Jimenez 2016), préréglage haut de XeGTAO : 3 tranches, 3 pas de chaque côté. Le ciel et le lointain (plus d'un
// pixel couvert par le rayon) restent éclairés
float2 gtao(float4 pos : SV_Position) : SV_Target {
    if (!Inside(pos.xy)) return float2(1, 0);
    float3 c = View(pos.xy);
    if (!(c.z > 0) || c.z > fade.y) return float2(1, c.z);
    float2 rad = opt.x * 0.5 * vp.zw / (abs(proj.xy) * c.z);
    float span = max(rad.x, rad.y);
    if (span < 1) return float2(1, c.z);
    float3 n = Normal(pos.xy, c);
    float3 v = normalize(-c);
    float k = min(1, opt.y / span);
    float R = opt.x * k;
    int2 cell = int2(pos.xy) & 3;
    float slice_noise = kDither[cell.y * 4 + cell.x] / 16;
    float step_noise = kDither[cell.x * 4 + cell.y] / 16;
    float vis = 0;
    [unroll] for (int s = 0; s < 3; s++) {
        float phi = (s + slice_noise) * (kHalfPi * 2 / 3);
        float3 dir = float3(cos(phi), sin(phi), 0);
        float2 omega = float2(dir.x, -dir.y) * rad * k;
        float3 ortho = dir - dot(dir, v) * v;
        float3 axis = normalize(cross(ortho, v));
        float3 pn = n - axis * dot(n, axis);
        float pn_len = max(length(pn), 1e-4);
        float cos_n = saturate(dot(pn, v) / pn_len);
        float an = sign(dot(ortho, pn)) * acos(cos_n);
        float2 low = float2(cos(an + kHalfPi), cos(an - kHalfPi));
        float2 hc = low;
        [unroll] for (int j = 0; j < 3; j++) {
            float f = (j + step_noise) / 3;
            float2 off = omega * (f * f + 1.3 / max(length(omega), 1e-3));
            float3 d0 = View(pos.xy + off) - c;
            float3 d1 = View(pos.xy - off) - c;
            float2 len = max(float2(length(d0), length(d1)), 1e-4);
            float2 w = saturate((R - len) / (R * 0.615));
            float2 sc = float2(dot(d0, v), dot(d1, v)) / len;
            hc = max(hc, lerp(low, sc, w));
        }
        float h0 = an + clamp(-acos(hc.y) - an, -kHalfPi, kHalfPi);
        float h1 = an + clamp(acos(hc.x) - an, -kHalfPi, kHalfPi);
        float sin_n = sin(an);
        float arc0 = (cos_n + 2 * h0 * sin_n - cos(2 * h0 - an)) / 4;
        float arc1 = (cos_n + 2 * h1 * sin_n - cos(2 * h1 - an)) / 4;
        vis += pn_len * (arc0 + arc1);
    }
    return float2(pow(saturate(vis / 3), 2.2), c.z);
}
// 5 prises le long de opt.zw lissent le tramage 4x4, sans traverser les bords de profondeur
float Smooth(float2 pos, out float z) {
    int2 p = int2(pos);
    int2 lo = int2(vp.xy);
    int2 hi = int2(vp.xy + vp.zw) - 1;
    z = ao.Load(int3(clamp(p, lo, hi), 0)).y;
    float sum = 0;
    float n = 0;
    [unroll] for (int i = -2; i <= 2; i++) {
        float2 s = ao.Load(int3(clamp(p + i * int2(opt.zw), lo, hi), 0));
        float w = (abs(i) == 2 ? 0.5 : 1) * saturate(2 - abs(s.y - z) / max(z * 0.02, 1e-3));
        sum += s.x * w;
        n += w;
    }
    return n > 0 ? sum / n : 1;
}
float2 blur(float4 pos : SV_Position) : SV_Target {
    float z;
    float a = Smooth(pos.xy, z);
    return float2(a, z);
}
// la couleur de la scène assombrie, l'occlusion atténuée au loin (le brouillard du jeu y domine)
float4 apply(float4 pos : SV_Position) : SV_Target {
    float4 c = color.Load(int3(pos.xy, 0));
    if (!Inside(pos.xy)) return c;
    float z;
    float a = Smooth(pos.xy, z);
    float far = saturate((z - fade.x) / max(fade.y - fade.x, 1));
    a = lerp(1, a, fade.z * (1 - far));
    return float4(c.rgb * a, c.a);
}
)";

// ombres douces : flou gaussien séparable de la texture d'ombre que le jeu dessine (silhouettes sur fond blanc).
// b1 : pas en uv (x, y), rayon en texels de la cible
inline constexpr char kSoftHlsl[] = R"(
cbuffer S : register(b1) { float4 step; };
Texture2D img : register(t0);
SamplerState ls : register(s0);
struct O { float4 p : SV_Position; float2 uv : TEXCOORD0; };
O vs(uint id : SV_VertexID) {
    O o;
    o.uv = float2((id << 1) & 2, id & 2);
    o.p = float4(o.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return o;
}
// 9 prises linéaires couvrant 17 texels, poids gaussiens (sigma = rayon / 2)
float4 soft(O i) : SV_Target {
    float4 c = 0;
    float n = 0;
    [unroll] for (int k = -4; k <= 4; k++) {
        float x = k * 2.0 * step.z / 8.0;
        float w = exp(-0.5 * (x * x) / max(step.z * step.z / 4, 1e-3));
        c += img.SampleLevel(ls, i.uv + step.xy * x, 0) * w;
        n += w;
    }
    return c / n;
}
)";

// netteté adaptative au contraste (CAS d'AMD, version simple) : chaque pixel renforcé selon le contraste de son voisinage,
// sans halo là où il est déjà fort. b1 : force dans [0, 1]
inline constexpr char kCasHlsl[] = R"(
cbuffer C : register(b1) { float4 k; };
Texture2D img : register(t0);
float4 vs(uint id : SV_VertexID) : SV_Position {
    float2 t = float2((id << 1) & 2, id & 2);
    return float4(t * float2(2, -2) + float2(-1, 1), 0, 1);
}
float3 At(int2 p, int2 hi) { return img.Load(int3(clamp(p, int2(0, 0), hi), 0)).rgb; }
float4 cas(float4 pos : SV_Position) : SV_Target {
    uint w, h;
    img.GetDimensions(w, h);
    int2 hi = int2(w, h) - 1;
    int2 p = int2(pos.xy);
    float4 e4 = img.Load(int3(p, 0));
    float3 a = At(p + int2(-1, -1), hi), b = At(p + int2(0, -1), hi), c = At(p + int2(1, -1), hi);
    float3 d = At(p + int2(-1, 0), hi), e = e4.rgb, f = At(p + int2(1, 0), hi);
    float3 g = At(p + int2(-1, 1), hi), i = At(p + int2(0, 1), hi), j = At(p + int2(1, 1), hi);
    float3 mn = min(min(min(d, e), min(f, b)), i);
    mn += min(mn, min(min(a, c), min(g, j)));
    float3 mx = max(max(max(d, e), max(f, b)), i);
    mx += max(mx, max(max(a, c), max(g, j)));
    float3 amp = sqrt(saturate(min(mn, 2 - mx) / max(mx, 1e-4)));
    float3 wt = amp * (-1 / lerp(8, 5, saturate(k.x)));
    float3 o = saturate((b * wt + d * wt + f * wt + i * wt + e) / (1 + 4 * wt));
    return float4(o, e4.a);
}
)";

inline constexpr char kSmaaConfig[] = R"(
cbuffer P : register(b1) { float4 metrics; };
#define SMAA_RT_METRICS metrics
#define SMAA_CUSTOM_SL
#define SMAA_THRESHOLD 0.05
#define SMAA_MAX_SEARCH_STEPS 16
#define SMAA_MAX_SEARCH_STEPS_DIAG 8
#define SMAA_CORNER_ROUNDING 25
SamplerState LinearSampler : register(s0);
SamplerState PointSampler : register(s1);
#define SMAATexture2D(tex) Texture2D tex
#define SMAATexturePass2D(tex) tex
#define SMAASampleLevelZero(tex, coord) tex.SampleLevel(LinearSampler, coord, 0)
#define SMAASampleLevelZeroPoint(tex, coord) tex.SampleLevel(PointSampler, coord, 0)
#define SMAASampleLevelZeroOffset(tex, coord, offset) tex.SampleLevel(LinearSampler, coord, 0, offset)
#define SMAASample(tex, coord) tex.Sample(LinearSampler, coord)
#define SMAASamplePoint(tex, coord) tex.Sample(PointSampler, coord)
#define SMAASampleOffset(tex, coord, offset) tex.Sample(LinearSampler, coord, offset)
#define SMAAGather(tex, coord) tex.Gather(LinearSampler, coord, 0)
#define SMAA_FLATTEN [flatten]
#define SMAA_BRANCH [branch]
)";

// après kSmaaHlsl : les bords d'après la luminance, les poids de mélange, puis l'image mélangée
inline constexpr char kSmaaPasses[] = R"(
Texture2D tex0 : register(t0);
Texture2D tex1 : register(t1);
Texture2D tex2 : register(t2);
struct SmaaEdgeV { float4 p : SV_Position; float2 uv : TEXCOORD0; float4 o[3] : TEXCOORD1; };
struct SmaaWeightV { float4 p : SV_Position; float2 uv : TEXCOORD0; float2 pix : TEXCOORD1; float4 o[3] : TEXCOORD2; };
struct SmaaBlendV { float4 p : SV_Position; float2 uv : TEXCOORD0; float4 o : TEXCOORD1; };
float4 corner(uint id, out float2 uv) {
    uv = float2((id << 1) & 2, id & 2);
    return float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
}
SmaaEdgeV vs_edges(uint id : SV_VertexID) {
    SmaaEdgeV o;
    o.p = corner(id, o.uv);
    SMAAEdgeDetectionVS(o.uv, o.o);
    return o;
}
SmaaWeightV vs_weights(uint id : SV_VertexID) {
    SmaaWeightV o;
    o.p = corner(id, o.uv);
    SMAABlendingWeightCalculationVS(o.uv, o.pix, o.o);
    return o;
}
SmaaBlendV vs_blend(uint id : SV_VertexID) {
    SmaaBlendV o;
    o.p = corner(id, o.uv);
    SMAANeighborhoodBlendingVS(o.uv, o.o);
    return o;
}
float4 smaa_edges(SmaaEdgeV i) : SV_Target { return float4(SMAALumaEdgeDetectionPS(i.uv, i.o, tex0), 0, 0); }
float4 smaa_weights(SmaaWeightV i) : SV_Target { return SMAABlendingWeightCalculationPS(i.uv, i.pix, i.o, tex0, tex1, tex2, 0); }
float4 smaa_blend(SmaaBlendV i) : SV_Target { return SMAANeighborhoodBlendingPS(i.uv, i.o, tex0, tex1); }
)";

// le point d'entrée vs que Context::Pass compile, par passe
inline constexpr char kSmaaEdgesVs[] = "SmaaEdgeV vs(uint id : SV_VertexID) { return vs_edges(id); }\n";
inline constexpr char kSmaaWeightsVs[] = "SmaaWeightV vs(uint id : SV_VertexID) { return vs_weights(id); }\n";
inline constexpr char kSmaaBlendVs[] = "SmaaBlendV vs(uint id : SV_VertexID) { return vs_blend(id); }\n";

}  // namespace game

// Lumière et couleurs (idées du portage Wind Waker) : bloom sur la luminance (le ciel et les couleurs saturées ne
// brillent pas), flou au quart de la résolution, ajouté en lumière linéaire avec les reflets d'objectif et les rayons
// de lumière tirés du même flou ; puis vibrance et courbe en S. Le noir et
// le blanc restent en place : le dessin cel-shadé garde ses aplats.
inline constexpr char kGlowHlsl[] = R"(
cbuffer C : register(b1) { float4 k0; float4 k1; };
Texture2D img : register(t0);
Texture2D glow : register(t1);
Texture2D extra : register(t2);
SamplerState LinearSampler : register(s0);
float4 vs(uint id : SV_VertexID) : SV_Position {
    float2 t = float2((id << 1) & 2, id & 2);
    return float4(t * float2(2, -2) + float2(-1, 1), 0, 1);
}
float3 Lin(float3 c) { return pow(saturate(c), 2.2); }
float Luma(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)); }
// k0 : taille de sortie, seuil, genou ; k1.x : seuil des rayons. 4 lectures bilinéaires couvrent les 4x4 pixels de
// l'image ; l'alpha garde ce qui dépasse le seuil des rayons (le ciel, les lumières), flou avec le reste
float4 glow_bright(float4 pos : SV_Position) : SV_Target {
    float2 uv = pos.xy / k0.xy, d = 0.25 / k0.xy;
    float3 s = 0;
    s += Lin(img.SampleLevel(LinearSampler, uv + float2(-d.x, -d.y), 0).rgb);
    s += Lin(img.SampleLevel(LinearSampler, uv + float2(d.x, -d.y), 0).rgb);
    s += Lin(img.SampleLevel(LinearSampler, uv + float2(-d.x, d.y), 0).rgb);
    s += Lin(img.SampleLevel(LinearSampler, uv + float2(d.x, d.y), 0).rgb);
    s *= 0.25;
    float l = Luma(pow(s, 1 / 2.2));  // le seuil se règle sur l'image vue
    float knee = max(k0.w, 1e-4);
    float soft = clamp(l - k0.z + knee, 0, 2 * knee);
    soft = soft * soft / (4 * knee);
    float w = max(soft, l - k0.z) / max(l, 1e-4);
    return float4(s * w, max(Luma(s) - k1.x, 0));
}
// k0 : taille, k1.xy : pas en uv (un axe)
float4 glow_blur(float4 pos : SV_Position) : SV_Target {
    float2 uv = pos.xy / k0.xy;
    static const float wt[5] = {0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216};
    float4 s = img.SampleLevel(LinearSampler, uv, 0) * wt[0];
    [unroll] for (int i = 1; i < 5; i++) {
        float2 o = k1.xy * (i * 1.5);
        s += img.SampleLevel(LinearSampler, uv + o, 0) * wt[i];
        s += img.SampleLevel(LinearSampler, uv - o, 0) * wt[i];
    }
    return s;
}
// la lumière d'ensemble pour les rayons : barycentre des zones claires de l'alpha du flou (le haut de l'image, où est
// le ciel, compte plus), leur part de l'image et leur concentration, sur une grille de 32x18. Une cible de 1x1, mêlée à celle des
// images d'avant : les rayons ne sautent pas d'une image à l'autre
float4 glow_sun(float4 pos : SV_Position) : SV_Target {
    float4 acc = 0;
    [loop] for (int y = 0; y < 18; y++) {
        [loop] for (int x = 0; x < 32; x++) {
            float2 uv = (float2(x, y) + 0.5) / float2(32, 18);
            float w = img.SampleLevel(LinearSampler, uv, 0).a * (1.5 - uv.y);
            acc += float4(uv * w, dot(uv, uv) * w, w);
        }
    }
    if (acc.w < 1e-4) return float4(0.5, 0.3, 0, 0);
    float2 c = acc.xy / acc.w;
    float spread = max(acc.z / acc.w - dot(c, c), 0);  // 0,17 quand tout l'écran brille
    return float4(c, acc.w / (32 * 18), saturate(1 - spread * 4));
}
// k0 : taille du quart, force des reflets d'objectif, force des rayons ; k1.x : seuil des reflets ; glow : glow_sun
float3 Flare(float2 uv, float2 dir) {
    if (any(uv < 0) || any(uv > 1)) return 0;
    // frange colorée : rouge et bleu pris de part et d'autre le long de l'axe
    float2 d = dir * 0.006;
    float3 c = float3(img.SampleLevel(LinearSampler, uv + d, 0).r, img.SampleLevel(LinearSampler, uv, 0).g,
                      img.SampleLevel(LinearSampler, uv - d, 0).b);
    return max(c - k1.x, 0);
}
float4 glow_extra(float4 pos : SV_Position) : SV_Target {
    float2 uv = pos.xy / k0.xy;
    float3 o = 0;
    if (k0.z > 0) {
        // reflets d'objectif (pseudo lens flare de John Chapman) : images fantômes des zones claires renvoyées par le
        // centre de l'image, plus faibles au bord, et un anneau de halo
        float2 tc = 1 - uv;
        float2 axis = 0.5 - tc;
        float len = length(axis);
        float2 dir = len > 1e-4 ? axis / len : float2(0, 0);
        float2 gv = axis * 0.42;
        static const float3 tint[4] = {float3(1, 0.8, 0.55), float3(0.6, 0.85, 1), float3(0.85, 1, 0.7), float3(1, 0.6, 0.9)};
        float3 f = 0;
        [unroll] for (int i = 0; i < 4; i++) {
            float2 g = tc + gv * i;
            float w = pow(1 - saturate(length(0.5 - g) / 0.7071), 5);
            f += Flare(g, dir) * w * tint[i];
        }
        float2 h = tc + dir * 0.42;
        float hw = pow(1 - saturate(length(0.5 - h) / 0.7071), 5);
        f += Flare(h, dir) * hw * float3(0.9, 0.95, 1);
        o += f * (k0.z * 1.5);
    }
    if (k0.w > 0) {
        // rayons de lumière (god rays) : flou radial de l'alpha (les zones claires) vers le barycentre de la lumière,
        // d'autant plus marqués qu'elle est concentrée, tramé pour ne pas faire de marches
        float4 sun = glow.Load(int3(0, 0, 0));
        float amt = k0.w * saturate(sun.z * 20) * sun.w;
        if (amt > 1e-3) {
            float2 d = (uv - sun.xy) * (0.9 / 24);
            float n = frac(52.9829189 * frac(dot(pos.xy, float2(0.06711056, 0.00583715))));
            float2 s = uv - d * n;
            float r = 0;
            float decay = 1;
            [loop] for (int i = 0; i < 24; i++) {
                r += img.SampleLevel(LinearSampler, s, 0).a * decay;
                s -= d;
                decay *= 0.94;
            }
            float fall = 1 - saturate(length((uv - sun.xy) * float2(k0.x / k0.y, 1)) * 0.8);
            o += float3(1, 0.97, 0.9) * (r * amt * 0.25 * fall);
        }
    }
    return float4(o, 1);
}
// k0 : taille, force du bloom, vibrance ; k1.x : contraste, k1.y : 1 avec les reflets et rayons (extra)
float4 glow_mix(float4 pos : SV_Position) : SV_Target {
    float2 uv = pos.xy / k0.xy;
    float4 c4 = img.Load(int3(pos.xy, 0));
    float3 c = c4.rgb;
    if (k0.z > 0 || k1.y > 0) {
        float3 l = Lin(c);
        if (k0.z > 0) l += glow.SampleLevel(LinearSampler, uv, 0).rgb * k0.z;
        if (k1.y > 0) l += extra.SampleLevel(LinearSampler, uv, 0).rgb * k1.y;
        c = pow(saturate(l), 1 / 2.2);
    }
    float y = Luma(c);
    float sat = max(c.r, max(c.g, c.b)) - min(c.r, min(c.g, c.b));
    c = lerp(float3(y, y, y), c, 1 + k0.w * (1 - sat));
    c = saturate(c);
    c = lerp(c, c * c * (3 - 2 * c), k1.x);
    return float4(c, c4.a);
}
)";
