#pragma once
// ============================================================
//  WelcomeSystem.h  –  Binary-digit WELCOME splash screen
//  Dear ImGui (OpenGL3 / GLFW backend)
//  v3 – clean letters, strict no-overlap, very dark background
// ============================================================

#include "imgui.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace std;

// ─── tuneable constants ──────────────────────────────────────
static constexpr float DIGIT_SPACING = 11.0f;   // px between digit centres
static constexpr float STROKE_W      = 18.0f;   // perpendicular half-thickness (px)
static constexpr float ASSEMBLE_DUR  = 4.0f;    // seconds – fly-in
static constexpr float HOLD_DUR      = 6.0f;    // seconds – solid hold
static constexpr float EXIT_DUR      = 2.5f;    // seconds – exit
static constexpr float LETTER_H      = 190.0f;  // cap-height (px)
static constexpr float LETTER_GAP    = 50.0f;   // gap between letter bounding boxes
// ─────────────────────────────────────────────────────────────

static inline float EaseOutCubic(float t) {
    float u = 1.0f - t;
    return 1.0f - u * u * u;
}

struct BinaryDigit {
    ImVec2 target, origin, pos, exitVel;
    char   ch;
    float  alpha, phaseOffset;
    bool   disperses, isStray;
};

enum class Phase { Assemble, Hold, Exit, Done };

// =============================================================
class WelcomeSystem {
public:
    WelcomeSystem()  { srand((unsigned)time(nullptr)); }
    ~WelcomeSystem() = default;

    void Init(ImVec2 screenSize) {
        m_screen = screenSize;
        m_time = m_phaseT = 0.0f;
        m_phase = Phase::Assemble;
        m_digits.clear();
        m_letterPoints.clear();
        BuildLetterPaths();
        SpawnDigits();
        m_initialised = true;
    }

    // Call every frame. Returns false when fully done.
    bool Render(float dt) {
        if (!m_initialised) Init(ImGui::GetIO().DisplaySize);

        m_time += dt;

        if      (m_phase == Phase::Assemble && m_time >= ASSEMBLE_DUR) {
            m_phase = Phase::Hold;  m_phaseT = 0.0f;
        } else if (m_phase == Phase::Hold    && m_time >= ASSEMBLE_DUR + HOLD_DUR) {
            m_phase = Phase::Exit;  m_phaseT = 0.0f;
            AssignExitBehaviour();
        } else if (m_phase == Phase::Exit    && m_phaseT >= EXIT_DUR) {
            m_phase = Phase::Done;
        }
        if (m_phase != Phase::Assemble) m_phaseT += dt;

        DrawBackground();

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        for (auto& d : m_digits) {
            UpdateDigit(d);
            if (d.alpha > 0.001f)
                dl->AddText(d.pos, DigitColor(d), &d.ch, &d.ch + 1);
        }
        return (m_phase != Phase::Done);
    }

    bool IsDone() const { return m_phase == Phase::Done; }
    void Reset()        { m_initialised = false; }

private:
    ImVec2              m_screen      { 1920, 1080 };
    float               m_time = 0, m_phaseT = 0;
    Phase               m_phase       = Phase::Assemble;
    bool                m_initialised = false;
    vector<BinaryDigit> m_digits;
    vector<ImVec2>      m_letterPoints;

    static float Rnd01()  { return (float)rand() / (float)RAND_MAX; }
    static float RndSym() { return Rnd01() * 2.0f - 1.0f; }
    char  RndBit()        { return (rand() & 1) ? '1' : '0'; }

    // ── Extremely dark navy background ───────────────────────
    void DrawBackground() {
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        ImVec2 s = m_screen;
        ImU32 top = IM_COL32(2, 4, 14, 255);   // deep dark navy
        ImU32 bot = IM_COL32(0, 0,  3, 255);   // near-black
        dl->AddRectFilledMultiColor({0,0}, s, top, top, bot, bot);
    }

    // ── Digit colour: clean blue-white ───────────────────────
    ImU32 DigitColor(const BinaryDigit& d) {
        float a = d.alpha;
        if (d.isStray) a *= 0.35f;
        if (m_phase == Phase::Hold)
            a *= 0.87f + 0.13f * sinf(m_time * 4.2f + d.phaseOffset);
        a = max(0.0f, min(1.0f, a));
        return IM_COL32(155, 205, 255, (int)(a * 255));
    }

    void UpdateDigit(BinaryDigit& d) {
        if (m_phase == Phase::Assemble) {
            float delay  = d.phaseOffset * 0.55f;
            float localT = max(0.0f, (m_time - delay) / max(0.01f, ASSEMBLE_DUR - delay));
            float ease   = EaseOutCubic(min(1.0f, localT));
            d.pos.x = d.origin.x + (d.target.x - d.origin.x) * ease;
            d.pos.y = d.origin.y + (d.target.y - d.origin.y) * ease;
            d.alpha = ease;
        } else if (m_phase == Phase::Hold) {
            d.pos   = d.target;
            d.alpha = 1.0f;
        } else if (m_phase == Phase::Exit) {
            float t = min(1.0f, m_phaseT / EXIT_DUR);
            if (d.disperses) {
                d.pos.x += d.exitVel.x * 0.016f;
                d.pos.y += d.exitVel.y * 0.016f;
                d.alpha  = 1.0f - t;
            } else {
                d.alpha = max(0.0f, 1.0f - m_phaseT / 0.18f);
            }
        } else {
            d.alpha = 0.0f;
        }
    }

    void AssignExitBehaviour() {
        for (auto& d : m_digits) {
            d.disperses = (rand() & 1) != 0;
            if (d.disperses) {
                float angle = Rnd01() * 6.2832f;
                float speed = 90.0f + Rnd01() * 210.0f;
                d.exitVel = { cosf(angle)*speed, sinf(angle)*speed };
            }
        }
    }

    // =========================================================
    //  LETTER PATH GEOMETRY
    //
    //  Key design decisions that prevent overlap:
    //  1. Every letter has a fixed bounding-box width.
    //     cx advances by exactly (width + LETTER_GAP).
    //  2. C and O use a reduced inner radius so the rightmost
    //     digit column never reaches the next letter's left edge.
    //  3. All raw points are snapped to a uniform grid (cell = sp)
    //     then de-duplicated, giving perfectly uniform density.
    // =========================================================
    void BuildLetterPaths() {
        const float H  = LETTER_H;
        const float SW = STROKE_W;
        const float sp = DIGIT_SPACING;

        vector<ImVec2> raw;
        raw.reserve(12000);

        // ── SampleLine: thick line segment ────────────────────
        auto SampleLine = [&](ImVec2 a, ImVec2 b) {
            float dx  = b.x - a.x, dy = b.y - a.y;
            float len = sqrtf(dx*dx + dy*dy);
            if (len < 0.5f) return;
            float tx = dx/len, ty = dy/len;  // along
            float nx = -ty,    ny =  tx;      // perp

            int nAlong = max(1, (int)ceilf(len / sp));
            int nPerp  = (int)floorf(SW / sp);

            for (int i = 0; i <= nAlong; i++) {
                float s  = (float)i / nAlong * len;
                float bx = a.x + tx*s, by = a.y + ty*s;
                raw.push_back({bx, by});
                for (int r = 1; r <= nPerp; r++) {
                    float off = (float)r * sp;
                    raw.push_back({bx + nx*off, by + ny*off});
                    raw.push_back({bx - nx*off, by - ny*off});
                }
            }
        };

        // ── SampleArc: thick arc ring ─────────────────────────
        auto SampleArc = [&](ImVec2 ctr, float r, float a0, float a1) {
            float arc   = a1 - a0;
            int   steps = max(12, (int)ceilf(fabsf(arc) * r / sp));
            int   nPerp = (int)floorf(SW / sp);

            for (int i = 0; i <= steps; i++) {
                float angle = a0 + arc * (float)i / steps;
                float ca = cosf(angle), sa = sinf(angle);
                raw.push_back({ctr.x + ca*r, ctr.y + sa*r});
                for (int rr = 1; rr <= nPerp; rr++) {
                    float off = (float)rr * sp;
                    raw.push_back({ctr.x + ca*(r+off), ctr.y + sa*(r+off)});
                    raw.push_back({ctr.x + ca*(r-off), ctr.y + sa*(r-off)});
                }
            }
        };

        float cx = 0; // running left-edge cursor

        // ══════ W  (bounding width = 160) ══════════════════════
        {
            const float W = 160.0f;
            float x0=cx, x1=cx+W*.25f, x2=cx+W*.5f, x3=cx+W*.75f, x4=cx+W;
            float yt=0, yb=H, ym=H*0.56f;
            SampleLine({x0,yt},{x1,yb});
            SampleLine({x1,yb},{x2,ym});
            SampleLine({x2,ym},{x3,yb});
            SampleLine({x3,yb},{x4,yt});
            cx += W + LETTER_GAP;
        }

        // ══════ E  (bounding width = 95) ═══════════════════════
        {
            const float W = 95.0f;
            SampleLine({cx,   0},{cx,     H});      // spine
            SampleLine({cx,   0},{cx+W,   0});      // top
            SampleLine({cx, H*.5f},{cx+W*.78f, H*.5f}); // mid
            SampleLine({cx,   H},{cx+W,   H});      // bottom
            cx += W + LETTER_GAP;
        }

        // ══════ L  (bounding width = 90) ═══════════════════════
        {
            const float W = 90.0f;
            SampleLine({cx,0},{cx,H});
            SampleLine({cx,H},{cx+W,H});
            cx += W + LETTER_GAP;
        }

        // ══════ C  (bounding width = 2*(r+SW)) ═════════════════
        // r chosen so total width = about 130 px
        {
            const float r    = 55.0f;
            const float gap  = 0.68f; // radians opening each side (~39°)
            const float bbW  = 2.0f*(r + SW);
            ImVec2 ctr = { cx + r + SW, H*0.5f };
            SampleArc(ctr, r, gap, (float)(2.0*3.14159265) - gap);
            cx += bbW + LETTER_GAP;
        }

        // ══════ O  (bounding width = 2*(r+SW)) ═════════════════
        {
            const float r   = 55.0f;
            const float bbW = 2.0f*(r + SW);
            ImVec2 ctr = { cx + r + SW, H*0.5f };
            SampleArc(ctr, r, 0.0f, (float)(2.0*3.14159265));
            cx += bbW + LETTER_GAP;
        }

        // ══════ M  (bounding width = 155) ══════════════════════
        {
            const float W   = 155.0f;
            float ymid = H * 0.50f;
            SampleLine({cx,   0},{cx,     H});       // left vertical
            SampleLine({cx,   0},{cx+W*.5f,ymid});   // left diagonal
            SampleLine({cx+W*.5f,ymid},{cx+W,0});    // right diagonal
            SampleLine({cx+W, 0},{cx+W,   H});       // right vertical
            cx += W + LETTER_GAP;
        }

        // ══════ E  (second, same as first) ═════════════════════
        {
            const float W = 95.0f;
            SampleLine({cx,   0},{cx,     H});
            SampleLine({cx,   0},{cx+W,   0});
            SampleLine({cx, H*.5f},{cx+W*.78f, H*.5f});
            SampleLine({cx,   H},{cx+W,   H});
            cx += W + LETTER_GAP;
        }

        // ── Grid-snap deduplication ───────────────────────────
        // Snap every raw point to the nearest grid cell (size = sp*0.9),
        // then discard duplicate cells. This is O(n log n) and gives
        // perfectly uniform, non-overlapping digit positions.
        float cell = sp * 0.92f;
        using IVec2 = pair<int,int>;
        vector<pair<IVec2, ImVec2>> snapped;
        snapped.reserve(raw.size());
        for (auto& p : raw) {
            int gx = (int)roundf(p.x / cell);
            int gy = (int)roundf(p.y / cell);
            snapped.push_back({{gx,gy}, {(float)gx*cell, (float)gy*cell}});
        }
        sort(snapped.begin(), snapped.end(),
             [](auto& a, auto& b){ return a.first < b.first; });
        snapped.erase(unique(snapped.begin(), snapped.end(),
             [](auto& a, auto& b){ return a.first == b.first; }),
             snapped.end());

        // ── Centre on screen ──────────────────────────────────
        float totalW = cx - LETTER_GAP; // last letter already advanced cx
        float offX   = (m_screen.x - totalW) * 0.5f;
        float offY   = (m_screen.y - H)       * 0.5f;

        m_letterPoints.reserve(snapped.size());
        for (auto& [key, p] : snapped)
            m_letterPoints.push_back({p.x + offX, p.y + offY});
    }

    // ── Spawn one particle per letter point + stray cloud ────
    void SpawnDigits() {
        m_digits.reserve(m_letterPoints.size() + 90);

        auto RandEdge = [&]() -> ImVec2 {
            switch (rand() % 4) {
                case 0: return {Rnd01()*m_screen.x, -30.0f};
                case 1: return {Rnd01()*m_screen.x, m_screen.y + 30.0f};
                case 2: return {-30.0f, Rnd01()*m_screen.y};
                default:return {m_screen.x + 30.0f, Rnd01()*m_screen.y};
            }
        };

        for (auto& tgt : m_letterPoints) {
            BinaryDigit d{};
            d.target = tgt;  d.origin = RandEdge();  d.pos = d.origin;
            d.ch = RndBit();  d.phaseOffset = Rnd01();
            m_digits.push_back(d);
        }

        // Sparse floating stray digits
        float cx = m_screen.x * 0.5f, cy = m_screen.y * 0.5f;
        for (int i = 0; i < 75; i++) {
            BinaryDigit d{};
            d.target = {cx + RndSym()*m_screen.x*0.45f,
                        cy + RndSym()*(LETTER_H*1.15f)};
            d.origin = RandEdge();  d.pos = d.origin;
            d.ch = RndBit();  d.phaseOffset = Rnd01();
            d.isStray = true;
            m_digits.push_back(d);
        }
    }
};