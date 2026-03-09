#pragma once
// ╔══════════════════════════════════════════════════════════════════╗
//  AlgoForgesUI.h
//  Strategic Command Center — AlgoForges
//  Pure Dear ImGui frontend, OOP, C++17
//  Aesthetic: Dark Forge / War-Room / Circuit-Board
//  Palette:   Deep black bg · Molten amber accent · Electric cyan data
//             Crimson warnings · Jade green success
//  Layout:    3-column command bridge with animated panels
// ╚══════════════════════════════════════════════════════════════════╝

#include "imgui.h"
#include "imgui_internal.h"
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <algorithm>

using namespace std;

// ═══════════════════════════════════════════════════════════════════
//  COLOUR PALETTE
// ═══════════════════════════════════════════════════════════════════
namespace AF {
    // Backgrounds
    constexpr ImU32 BG_VOID      = IM_COL32( 4,  5,  8, 255); // deepest black
    constexpr ImU32 BG_PANEL     = IM_COL32( 9, 11, 16, 255); // panel bg
    constexpr ImU32 BG_PANEL2    = IM_COL32(13, 16, 22, 255); // lighter panel
    constexpr ImU32 BG_INPUT     = IM_COL32( 7,  9, 14, 255); // input area

    // Amber / forge fire
    constexpr ImU32 AMBER        = IM_COL32(255, 160,  30, 255);
    constexpr ImU32 AMBER_DIM    = IM_COL32(180, 100,  15, 160);
    constexpr ImU32 AMBER_GLOW   = IM_COL32(255, 180,  60,  80);
    constexpr ImU32 AMBER_DARK   = IM_COL32( 80,  40,   5, 255);

    // Cyan / data stream
    constexpr ImU32 CYAN         = IM_COL32(  0, 220, 220, 255);
    constexpr ImU32 CYAN_DIM     = IM_COL32(  0, 140, 140, 160);
    constexpr ImU32 CYAN_GLOW    = IM_COL32(  0, 200, 200,  60);

    // Status colours
    constexpr ImU32 GREEN        = IM_COL32( 50, 220, 100, 255);
    constexpr ImU32 GREEN_DIM    = IM_COL32( 30, 130,  60, 160);
    constexpr ImU32 RED          = IM_COL32(220,  55,  55, 255);
    constexpr ImU32 RED_DIM      = IM_COL32(140,  30,  30, 160);
    constexpr ImU32 YELLOW       = IM_COL32(240, 200,  40, 255);
    constexpr ImU32 PURPLE       = IM_COL32(160,  80, 255, 255);

    // Text
    constexpr ImU32 TXT_BRIGHT   = IM_COL32(235, 235, 240, 255);
    constexpr ImU32 TXT_MID      = IM_COL32(160, 165, 175, 255);
    constexpr ImU32 TXT_DIM      = IM_COL32( 85,  90, 100, 255);
    constexpr ImU32 TXT_AMBER    = IM_COL32(255, 165,  40, 255);
    constexpr ImU32 TXT_CYAN     = IM_COL32(  0, 210, 210, 255);

    // Border
    constexpr ImU32 BORDER       = IM_COL32( 35,  40,  52, 255);
    constexpr ImU32 BORDER_GLOW  = IM_COL32( 60,  70,  90, 255);

    // Helpers
    static ImU32 Alpha(ImU32 col, float a) {
        ImVec4 c = ImGui::ColorConvertU32ToFloat4(col);
        c.w *= a; return ImGui::ColorConvertFloat4ToU32(c);
    }
    static ImVec4 ToV4(ImU32 col) { return ImGui::ColorConvertU32ToFloat4(col); }
    static float  Pulse(float t, float freq=1.f){ return (sinf(t*freq*6.2832f)+1.f)*.5f; }
    static float  EaseOut(float t){ float u=1-t; return 1-u*u*u; }
}

// ═══════════════════════════════════════════════════════════════════
//  ANIMATED SCAN LINE HELPER
// ═══════════════════════════════════════════════════════════════════
struct ScanLine {
    float y = 0, speed = 0.18f;
    void Update(float dt, float h){ y = fmodf(y + speed * h * dt, h); }
    void Draw(ImDrawList* dl, ImVec2 origin, float w, float h, ImU32 col){
        float gy = origin.y + y;
        dl->AddRectFilled({origin.x, gy-1.f},{origin.x+w, gy+1.f}, col);
    }
};

// ═══════════════════════════════════════════════════════════════════
//  COMPLEXITY RADAR  (hexagonal spider chart)
// ═══════════════════════════════════════════════════════════════════
class ComplexityRadar {
public:
    struct Axis { string name; float value; ImU32 col; }; // value 0-1

    void SetAxes(vector<Axis> axes){ m_axes = move(axes); }

    void Draw(ImDrawList* dl, ImVec2 centre, float radius, float time) {
        if(m_axes.empty()) return;
        int N = (int)m_axes.size();
        float step = 6.2832f / N;

        // Draw grid rings
        for(int ring = 1; ring <= 4; ring++){
            float r = radius * ring / 4.f;
            vector<ImVec2> pts;
            for(int i=0;i<N;i++){
                float a = i*step - 1.5708f;
                pts.push_back({centre.x+cosf(a)*r, centre.y+sinf(a)*r});
            }
            for(int i=0;i<N;i++)
                dl->AddLine(pts[i], pts[(i+1)%N], AF::Alpha(AF::BORDER,0.6f), 1.f);
        }
        // Spokes
        for(int i=0;i<N;i++){
            float a = i*step - 1.5708f;
            dl->AddLine(centre,
                {centre.x+cosf(a)*radius, centre.y+sinf(a)*radius},
                AF::Alpha(AF::BORDER,0.5f), 1.f);
        }

        // Animated data polygon (amber fill)
        float pulse = 0.88f + 0.12f*sinf(time*2.f);
        vector<ImVec2> data;
        for(int i=0;i<N;i++){
            float a = i*step - 1.5708f;
            float r = radius * m_axes[i].value * pulse;
            data.push_back({centre.x+cosf(a)*r, centre.y+sinf(a)*r});
        }
        // Fill
        for(int i=0;i<N;i++)
            dl->AddTriangleFilled(centre, data[i], data[(i+1)%N],
                AF::Alpha(AF::AMBER, 0.18f));
        // Outline
        for(int i=0;i<N;i++)
            dl->AddLine(data[i], data[(i+1)%N], AF::Alpha(AF::AMBER, 0.85f), 1.8f);
        // Nodes
        for(int i=0;i<N;i++){
            dl->AddCircleFilled(data[i], 4.f, m_axes[i].col);
            dl->AddCircle(data[i], 4.f, AF::AMBER, 8, 1.f);
        }
        // Labels
        for(int i=0;i<N;i++){
            float a = i*step - 1.5708f;
            float lr = radius + 20.f;
            ImVec2 lp = {centre.x+cosf(a)*lr, centre.y+sinf(a)*lr};
            ImVec2 tsz = ImGui::CalcTextSize(m_axes[i].name.c_str());
            dl->AddText({lp.x-tsz.x*.5f, lp.y-tsz.y*.5f},
                        AF::TXT_MID, m_axes[i].name.c_str());
        }
    }
private:
    vector<Axis> m_axes;
};

// ═══════════════════════════════════════════════════════════════════
//  UNIQUENESS SCORE RING
// ═══════════════════════════════════════════════════════════════════
class UniquenessRing {
public:
    float score = 0.73f; // 0-1
    float animT = 0.f;

    void Draw(ImDrawList* dl, ImVec2 centre, float radius, float time){
        animT = min(1.f, animT + 0.016f);
        float displayed = AF::EaseOut(animT) * score;

        // Background ring
        dl->AddCircle(centre, radius, AF::Alpha(AF::BORDER,0.8f), 64, 6.f);

        // Coloured arc
        float arcEnd = -1.5708f + displayed * 6.2832f;
        float arcStart = -1.5708f;
        int segs = 80;
        ImU32 col = (score > 0.75f) ? AF::GREEN : (score > 0.5f) ? AF::AMBER : AF::RED;
        for(int i=0;i<segs;i++){
            float a0 = arcStart + (arcEnd-arcStart)*(float)i/segs;
            float a1 = arcStart + (arcEnd-arcStart)*(float)(i+1)/segs;
            dl->AddLine(
                {centre.x+cosf(a0)*radius, centre.y+sinf(a0)*radius},
                {centre.x+cosf(a1)*radius, centre.y+sinf(a1)*radius},
                AF::Alpha(col, 0.9f), 7.f);
        }

        // Glow dot at tip
        float pulseTip = 0.7f+0.3f*sinf(time*4.f);
        dl->AddCircleFilled(
            {centre.x+cosf(arcEnd)*radius, centre.y+sinf(arcEnd)*radius},
            6.f*pulseTip, col, 12);

        // Centre text
        char buf[16]; snprintf(buf,sizeof(buf),"%.0f%%", displayed*100.f);
        ImVec2 tsz = ImGui::CalcTextSize(buf);
        // Scale up for the score number
        ImGui::SetWindowFontScale(1.8f);
        tsz = ImGui::CalcTextSize(buf);
        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                    {centre.x-tsz.x*.5f, centre.y-tsz.y*.5f}, col, buf);
        ImGui::SetWindowFontScale(1.f);
        // Label
        ImVec2 lsz = ImGui::CalcTextSize("UNIQUENESS");
        dl->AddText({centre.x-lsz.x*.5f, centre.y+tsz.y*.2f},
                    AF::TXT_DIM, "UNIQUENESS");
    }
};

// ═══════════════════════════════════════════════════════════════════
//  SIMILARITY BAR  (LeetCode match)
// ═══════════════════════════════════════════════════════════════════
struct SimilarityEntry {
    string  title;
    string  number;
    float   similarity; // 0-1
    string  difficulty; // "Easy" "Medium" "Hard"
};

// ═══════════════════════════════════════════════════════════════════
//  EDGE CASE CARD
// ═══════════════════════════════════════════════════════════════════
struct EdgeCase {
    string  description;
    string  example;
    bool    critical;
    bool    present;   // true = found in problem, false = not found
};


// ═══════════════════════════════════════════════════════════════════
//  QUESTION TYPE PROFICIENCY
// ═══════════════════════════════════════════════════════════════════
struct QuestionTag {
    string  name;         // e.g. "Dynamic Programming"
    string  shortCode;    // e.g. "DP"
    int     solved;       // number solved
    int     total;        // total attempted
    float   accuracy;     // 0-1
    ImU32   col;          // accent colour
};

struct DifficultyBreakdown {
    int easySolved   = 42,  easyTotal   = 50;
    int mediumSolved = 28,  mediumTotal = 60;
    int hardSolved   = 8,   hardTotal   = 30;
};

// ═══════════════════════════════════════════════════════════════════
//  PANEL HELPER  – draws an angular, bordered, titled panel
// ═══════════════════════════════════════════════════════════════════
class Panel {
public:
    // Draw the panel shell. Returns inner content top-left and size.
    static void Begin(ImDrawList* dl,
                      ImVec2 pos, ImVec2 size,
                      const char* title,
                      ImU32 accentCol,
                      float time,
                      bool  scanline = true,
                      ScanLine* scan = nullptr)
    {
        float x=pos.x,y=pos.y,w=size.x,h=size.y;

        // Outer glow
        dl->AddRectFilled({x-2,y-2},{x+w+2,y+h+2},
                          AF::Alpha(accentCol, 0.08f), 4.f);
        // Panel background
        dl->AddRectFilled({x,y},{x+w,y+h}, AF::BG_PANEL, 3.f);

        // Top accent bar (full width, 2px)
        dl->AddRectFilled({x,y},{x+w,y+2}, accentCol);

        // Top-left corner cut (angular bracket feel)
        dl->AddLine({x,y+14},{x,y}, accentCol, 2.f);
        dl->AddLine({x,y},{x+14,y}, accentCol, 2.f);
        // Top-right corner cut
        dl->AddLine({x+w-14,y},{x+w,y}, accentCol, 2.f);
        dl->AddLine({x+w,y},{x+w,y+14}, accentCol, 2.f);
        // Bottom-left
        dl->AddLine({x,y+h-14},{x,y+h}, AF::Alpha(accentCol,0.4f), 1.f);
        dl->AddLine({x,y+h},{x+14,y+h}, AF::Alpha(accentCol,0.4f), 1.f);
        // Bottom-right
        dl->AddLine({x+w-14,y+h},{x+w,y+h}, AF::Alpha(accentCol,0.4f), 1.f);
        dl->AddLine({x+w,y+h-14},{x+w,y+h}, AF::Alpha(accentCol,0.4f), 1.f);

        // Full border (dim)
        dl->AddRect({x,y},{x+w,y+h}, AF::BORDER, 3.f, 0, 1.f);

        // Scanline
        if(scanline && scan){
            float glow = 0.12f + 0.06f*sinf(time*2.f);
            scan->Draw(dl, pos, w, h, AF::Alpha(accentCol, glow));
        }

        // Title bar
        dl->AddRectFilled({x,y},{x+w,y+26}, AF::Alpha(accentCol, 0.12f));
        dl->AddLine({x,y+26},{x+w,y+26}, AF::Alpha(accentCol, 0.35f), 1.f);

        // Title text
        float pulse = 0.92f + 0.08f*sinf(time*1.5f);
        dl->AddText({x+12,y+6}, AF::Alpha(accentCol,(int)(pulse*255)/255.f),
                    title);

        // Live indicator dot
        float dotP = 0.6f+0.4f*sinf(time*3.f);
        dl->AddCircleFilled({x+w-16, y+13}, 4.f,
                            AF::Alpha(accentCol, dotP), 8);
    }
};

// ═══════════════════════════════════════════════════════════════════
//  MAIN AlgoForges UI CLASS
// ═══════════════════════════════════════════════════════════════════
class AlgoForgesUI {
public:
    explicit AlgoForgesUI(GLFWwindow* window) : m_window(window) { Init(); }

    void Init(){
        m_time = 0;
        memset(m_problemBuf, 0, sizeof(m_problemBuf));
        memset(m_tabBuf,     0, sizeof(m_tabBuf));

        // Tab 0: solution.cpp  — read-only, shows optimized solution
        strncpy(m_tabBuf[0],
            "// solution.cpp  —  AI-generated optimized solution\n"
            "// (Read-only: edit from the Optimized Approach panel)\n"
            "#include <bits/stdc++.h>\nusing namespace std;\n\n"
            "class Solution {\npublic:\n"
            "    vector<int> twoSum(vector<int>& nums, int target) {\n"
            "        unordered_map<int,int> seen;\n"
            "        for (int i = 0; i < (int)nums.size(); i++) {\n"
            "            int need = target - nums[i];\n"
            "            if (seen.count(need)) return {seen[need], i};\n"
            "            seen[nums[i]] = i;\n"
            "        }\n"
            "        return {};\n"
            "    }\n};",
            sizeof(m_tabBuf[0])-1);

        // Tab 1: brute.cpp  — read-only, shows brute force
        strncpy(m_tabBuf[1],
            "// brute.cpp  —  Brute-force reference (Read-only)\n"
            "#include <bits/stdc++.h>\nusing namespace std;\n\n"
            "class Solution {\npublic:\n"
            "    vector<int> twoSum(vector<int>& nums, int target) {\n"
            "        int n = nums.size();\n"
            "        for (int i = 0; i < n; i++)\n"
            "          for (int j = i+1; j < n; j++)\n"
            "            if (nums[i]+nums[j] == target)\n"
            "              return {i, j};\n"
            "        return {};\n"
            "    }\n};",
            sizeof(m_tabBuf[1])-1);

        // Tab 2: test.cpp  — editable, user writes test cases
        strncpy(m_tabBuf[2],
            "// test.cpp  —  Your custom test cases (Editable)\n"
            "// Write your own inputs below and press [>] RUN\n\n"
            "int main() {\n"
            "    Solution s;\n"
            "    // Test 1\n"
            "    vector<int> nums1 = {2, 7, 11, 15};\n"
            "    auto r1 = s.twoSum(nums1, 9);\n"
            "    assert(r1 == vector<int>({0,1}));\n\n"
            "    // Add your tests here...\n"
            "    return 0;\n"
            "}",
            sizeof(m_tabBuf[2])-1);

        // Tab 3: notes.md  — fully editable markdown notes
        strncpy(m_tabBuf[3],
            "# Problem Notes\n\n"
            "## Key Insight\n"
            "Use a hash map to store complement -> index.\n"
            "Single pass O(n) instead of nested O(n^2).\n\n"
            "## Gotchas\n"
            "- Same element cannot be used twice\n"
            "- Guaranteed exactly one solution\n\n"
            "## Related Patterns\n"
            "- Two Pointers (sorted array variant)\n"
            "- Sliding Window\n\n"
            "## TODO\n"
            "- [ ] Handle edge: empty array\n"
            "- [ ] Benchmark against brute force\n",
            sizeof(m_tabBuf[3])-1);

        m_scanTool.speed = 0.15f;
        m_generatedTests = {
            {"[2,7,11,15], t=9", "[0,1]",  "Normal",  "READY",  AF::GREEN  },
            {"[3,2,4], t=6",     "[1,2]",  "Normal",  "READY",  AF::GREEN  },
            {"[], t=0",          "[]",     "Edge",    "READY",  AF::AMBER  },
            {"[0,0], t=0",       "[0,1]",  "Edge",    "READY",  AF::AMBER  },
            {"[-1,-2,-3], t=-3", "[-1,-2]","Negative","READY",  AF::CYAN   },
            {"[1000000,2], t=1000002","[0,1]","Stress","READY", AF::PURPLE },
        };
        m_debugOutput = {};
        m_cmpDiffs = {};

        // Seed placeholder data
        m_similarities = {
            {"Two Sum",              "#1",   0.87f, "Easy"  },
            {"Subarray Sum Equals K","#560", 0.74f, "Medium"},
            {"Max Points on a Line", "#149", 0.61f, "Hard"  },
            {"Merge Intervals",      "#56",  0.55f, "Medium"},
        };

        m_edgeCases = {
            {"Empty input array",      "arr = []",          true,  true  },
            {"All elements identical", "arr = [5,5,5,5]",   false, false },
            {"Integer overflow",       "n = 2^31 - 1",       true,  true  },
            {"Single element",         "arr = [42]",         false, false },
            {"Negative numbers",       "arr = [-1,-2,-3]",   true,  true  },
        };

        m_radar.SetAxes({
            {"Time",    0.85f, AF::AMBER },
            {"Space",   0.60f, AF::CYAN  },
            {"Optimal", 0.78f, AF::GREEN },
            {"Edges",   0.45f, AF::RED   },
            {"Clarity", 0.90f, AF::PURPLE},
            {"Scale",   0.70f, AF::YELLOW},
        });

        m_uniqueness.score  = 0.73f;
        m_uniqueness.animT  = 0.f;

        m_bruteForce =
            "// BRUTE FORCE  O(n²) / O(1)\n"
            "for (int i = 0; i < n; i++)\n"
            "  for (int j = i+1; j < n; j++)\n"
            "    if (check(arr[i], arr[j]))\n"
            "      result.push_back({i, j});\n"
            "return result;";

        m_optimized =
            "// OPTIMIZED  O(n log n) / O(n)\n"
            "unordered_map<int,int> seen;\n"
            "for (int i = 0; i < n; i++) {\n"
            "  int need = target - arr[i];\n"
            "  if (seen.count(need))\n"
            "    return {seen[need], i};\n"
            "  seen[arr[i]] = i;\n"
            "}\n"
            "return {};";

        // Init scan lines with different speeds
        m_scanProblem.speed   = 0.12f;
        m_scanBrute.speed     = 0.16f;
        m_scanOpt.speed       = 0.20f;
        m_scanSim.speed       = 0.14f;
        m_scanEdge.speed      = 0.18f;
        m_scanRadar.speed     = 0.10f;
        m_scanCode.speed      = 0.08f;
        m_scanUnique.speed    = 0.22f;
        m_scanProf.speed      = 0.13f;

        // Question-type proficiency data
        m_tags = {
            {"Arrays & Hashing",    "ARR",  38, 42, 0.90f, AF::GREEN  },
            {"Two Pointers",        "2PTR", 18, 20, 0.90f, AF::GREEN  },
            {"Sliding Window",      "SW",   14, 18, 0.78f, AF::AMBER  },
            {"Binary Search",       "BS",   20, 28, 0.71f, AF::AMBER  },
            {"Stack / Queue",       "STK",  16, 20, 0.80f, AF::GREEN  },
            {"Trees / BST",         "TRE",  22, 35, 0.63f, AF::AMBER  },
            {"Graphs / BFS/DFS",    "GRP",  12, 30, 0.40f, AF::RED    },
            {"Dynamic Programming", "DP",    8, 32, 0.25f, AF::RED    },
            {"Greedy",              "GRD",  14, 18, 0.78f, AF::AMBER  },
            {"Backtracking",        "BT",    6, 20, 0.30f, AF::RED    },
            {"Bit Manipulation",    "BIT",  10, 12, 0.83f, AF::GREEN  },
            {"Math / Number Thy",   "MTH",   9, 14, 0.64f, AF::AMBER  },
        };
    }

    // ── Call every frame after ImGui::NewFrame() ──────────────────
    void Render(float dt){
        m_time += dt;

        ImGuiIO& io  = ImGui::GetIO();
        ImVec2   scr = io.DisplaySize;

        // ── Full-screen host window ───────────────────────────────
        ImGui::SetNextWindowPos({0,0});
        ImGui::SetNextWindowSize(scr);
        ImGui::SetNextWindowBgAlpha(0.f);
        ImGui::Begin("##AFHost", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoMove);

        ImDrawList* dl = ImGui::GetWindowDrawList();

        // ── Background ────────────────────────────────────────────
        DrawBackground(dl, scr);

        // ── Top command bar ───────────────────────────────────────
        float titleH = 30.f;   // custom OS-style title bar
        float statusH= 52.f;   // forge status bar
        float topBarH = titleH + statusH;
        DrawTitleBar(dl, scr, titleH);
        DrawTopBar(dl, scr, titleH, statusH);

        // ── Layout grid ───────────────────────────────────────────
        float pad    = 10.f;
        float colY   = topBarH + pad;
        float colH   = scr.y - topBarH - pad * 2.f;

        float col1W  = scr.x * 0.26f;
        float col2W  = scr.x * 0.45f;
        float col3W  = scr.x - col1W - col2W - pad * 4.f;

        float col1X  = pad;
        float col2X  = col1X + col1W + pad;
        float col3X  = col2X + col2W + pad;

        // ── COLUMN 1: Problem Input + Similarity + Edge Cases ─────
        DrawColumn1(dl, {col1X, colY}, {col1W, colH}, dt);

        // ── COLUMN 2: Approaches + Code Editor ───────────────────
        DrawColumn2(dl, {col2X, colY}, {col2W, colH}, dt);

        // ── COLUMN 3: Radar + Uniqueness + Stats ─────────────────
        DrawColumn3(dl, {col3X, colY}, {col3W, colH}, dt);

        ImGui::End();
    }

private:
    GLFWwindow* m_window = nullptr;
    float   m_time = 0;
    // Title-bar drag state
    bool    m_dragging   = false;
    double  m_dragOffX   = 0, m_dragOffY = 0;
    bool    m_maximized  = false;

    // Data
    string  m_bruteForce, m_optimized;
    char    m_problemBuf[2048];
    // Per-tab content buffers (solution=0, brute=1, test=2, notes=3)
    char    m_tabBuf[4][8192] = {};
    int     m_selectedApproach = 1;
    int     m_selectedTab      = 0;
    bool    m_analysisRunning  = false;
    float   m_analysisProgress = 0.f;

    // Tool panel state (below editor)
    int     m_toolTab          = 0;   // 0=TestGen 1=Debug 2=Compare
    char    m_testInputBuf[1024]= {};
    char    m_debugBuf[4096]   = {};
    char    m_cmpBuf[4096]     = {};
    bool    m_testRunning      = false;
    float   m_testProgress     = 0.f;
    bool    m_debugRunning     = false;
    float   m_debugProgress    = 0.f;
    bool    m_cmpRunning       = false;
    float   m_cmpProgress      = 0.f;

    // Generated test cases display
    struct TestCase { string input; string expected; string type; string status; ImU32 col; };
    vector<TestCase> m_generatedTests;
    // Debug output lines
    struct DebugLine { string text; ImU32 col; };
    vector<DebugLine> m_debugOutput;
    // Comparison result
    struct CmpDiff { int line; string left; string right; };
    vector<CmpDiff> m_cmpDiffs;
    ScanLine m_scanTool;

    vector<SimilarityEntry>  m_similarities;
    vector<EdgeCase>         m_edgeCases;
    vector<QuestionTag>      m_tags;
    DifficultyBreakdown      m_diffBreak;
    ScanLine                 m_scanProf;
    ComplexityRadar          m_radar;
    UniquenessRing           m_uniqueness;

    // Scan lines per panel
    ScanLine m_scanProblem, m_scanBrute, m_scanOpt;
    ScanLine m_scanSim,     m_scanEdge,  m_scanRadar;
    ScanLine m_scanCode,    m_scanUnique;

    // ── UPDATE SCANS ─────────────────────────────────────────────
    void TickScans(float dt, float h){
        m_scanProblem.Update(dt,h); m_scanBrute.Update(dt,h);
        m_scanOpt.Update(dt,h);     m_scanSim.Update(dt,h);
        m_scanEdge.Update(dt,h);    m_scanRadar.Update(dt,h);
        m_scanCode.Update(dt,h);    m_scanUnique.Update(dt,h);
    }

    // ─────────────────────────────────────────────────────────────
    //  BACKGROUND
    // ─────────────────────────────────────────────────────────────
    void DrawBackground(ImDrawList* dl, ImVec2 scr){
        dl->AddRectFilled({0,0}, scr, AF::BG_VOID);

        // Subtle grid overlay
        for(float x=0; x<scr.x; x+=40.f)
            dl->AddLine({x,0},{x,scr.y}, AF::Alpha(AF::BORDER, 0.3f), 0.5f);
        for(float y=0; y<scr.y; y+=40.f)
            dl->AddLine({0,y},{scr.x,y}, AF::Alpha(AF::BORDER, 0.3f), 0.5f);

        // Corner forge glow (ambient lighting)
        dl->AddCircleFilled({0,scr.y}, scr.x*.35f,
                            AF::Alpha(AF::AMBER, 0.03f), 48);
        dl->AddCircleFilled({scr.x,0}, scr.x*.3f,
                            AF::Alpha(AF::CYAN, 0.03f), 48);
    }

    // ─────────────────────────────────────────────────────────────
    //  CUSTOM OS-STYLE TITLE BAR  (row 0)
    //  Draggable area + Minimize / Maximize / Close buttons
    // ─────────────────────────────────────────────────────────────
    void DrawTitleBar(ImDrawList* dl, ImVec2 scr, float h){
        // Background: slightly lighter than the forge status bar
        dl->AddRectFilled({0,0},{scr.x,h},
                          AF::Alpha(IM_COL32(6,8,14,255), 0.99f));
        // Bottom separator line (thin amber)
        dl->AddLine({0,h-1},{scr.x,h-1}, AF::Alpha(AF::AMBER,0.25f), 1.f);

        // ── App icon + title (centred) ────────────────────────────
        const char* titleStr = "AlgoForges  —  Strategic Command Center";
        ImVec2 tsz = ImGui::CalcTextSize(titleStr);
        dl->AddText({scr.x*.5f - tsz.x*.5f, h*.5f - tsz.y*.5f},
                    AF::Alpha(AF::TXT_MID, 0.85f), titleStr);

        // ── Draggable region (everything except the 3 buttons) ────
        float btnAreaW = 110.f;
        ImGui::SetCursorScreenPos({0, 0});
        ImGui::InvisibleButton("##titleDrag", {scr.x - btnAreaW, h});
        if(ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)){
            if(m_window && !m_maximized){
                double mx, my;
                glfwGetCursorPos(m_window, &mx, &my);
                if(!m_dragging){
                    int wx,wy;
                    glfwGetWindowPos(m_window,&wx,&wy);
                    m_dragOffX = mx - 0; m_dragOffY = my - 0;
                    // recalculate offset relative to window origin
                    m_dragging = true;
                }
                // Move window: new pos = cursor screen pos minus offset
                // We need screen-space cursor: use glfwGetCursorPos gives
                // client coords, so add window position
                int wx,wy; glfwGetWindowPos(m_window,&wx,&wy);
                double screenMx = wx + mx;
                double screenMy = wy + my;
                if(!m_dragging){
                    m_dragOffX = mx; m_dragOffY = my; m_dragging=true;
                }
                glfwSetWindowPos(m_window,
                    (int)(screenMx - m_dragOffX),
                    (int)(screenMy - m_dragOffY));
            }
        } else { m_dragging = false; }

        // Double-click on drag area → toggle maximise
        if(ImGui::IsItemHovered() &&
           ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && m_window){
            if(m_maximized){ glfwRestoreWindow(m_window);  m_maximized=false; }
            else            { glfwMaximizeWindow(m_window); m_maximized=true;  }
        }

        // ── Window control buttons ────────────────────────────────
        // Layout: [─]  [□]  [✕]  right-aligned with spacing
        struct WinBtn {
            const char* symbol;
            ImU32       hoverBg;
            ImU32       iconCol;
        };
        WinBtn wbtns[] = {
            { "─",  IM_COL32(50, 55, 65, 200),  IM_COL32(200,200,200,255) }, // minimise
            { "[+]",  IM_COL32(50, 55, 65, 200),  IM_COL32(200,200,200,255) }, // maximise
            { "[X]",  IM_COL32(185, 38, 38, 230),  IM_COL32(255,255,255,255) }, // close
        };
        float btnW = 36.f, btnH = h;
        float btnStartX = scr.x - (btnW * 3.f);

        for(int i = 0; i < 3; i++){
            float bx = btnStartX + i * btnW;
            ImVec2 bp0 = {bx,    0};
            ImVec2 bp1 = {bx+btnW, btnH};
            bool hov = ImGui::IsMouseHoveringRect(bp0, bp1);

            // Hover background
            if(hov)
                dl->AddRectFilled(bp0, bp1, wbtns[i].hoverBg);

            // Icon centred in button
            ImVec2 isz = ImGui::CalcTextSize(wbtns[i].symbol);
            float  ix  = bx  + (btnW - isz.x) * .5f;
            float  iy  = btnH * .5f - isz.y * .5f;
            // For minimise draw as a line for precision
            if(i == 0){
                float ly = btnH * .5f + 1.f;
                dl->AddLine({bx+10,ly},{bx+btnW-10,ly},
                            hov ? IM_COL32(255,255,255,255) : wbtns[i].iconCol,
                            1.8f);
            } else {
                dl->AddText({ix, iy},
                            hov ? IM_COL32(255,255,255,255) : wbtns[i].iconCol,
                            wbtns[i].symbol);
            }

            // Thin vertical separator between buttons
            if(i < 2)
                dl->AddLine({bx+btnW-0.5f, 6},
                            {bx+btnW-0.5f, btnH-6},
                            AF::Alpha(AF::BORDER, 0.6f), 1.f);

            // Click handling
            ImGui::SetCursorScreenPos(bp0);
            ImGui::InvisibleButton(("##wb"+to_string(i)).c_str(), {btnW, btnH});
            if(ImGui::IsItemClicked() && m_window){
                if(i == 0){ glfwIconifyWindow(m_window); }
                else if(i == 1){
                    if(m_maximized){ glfwRestoreWindow(m_window);  m_maximized=false; }
                    else            { glfwMaximizeWindow(m_window); m_maximized=true;  }
                }
                else if(i == 2){ glfwSetWindowShouldClose(m_window, GLFW_TRUE); }
            }
        }
    }

    // ─────────────────────────────────────────────────────────────
    //  FORGE STATUS BAR  (row 1, below title bar)
    // ─────────────────────────────────────────────────────────────
    void DrawTopBar(ImDrawList* dl, ImVec2 scr, float titleH, float h){
        float y0 = titleH;           // start below title bar
        float y1 = titleH + h;

        // Bar background
        dl->AddRectFilled({0,y0},{scr.x,y1}, AF::Alpha(AF::BG_PANEL2,0.98f));
        dl->AddLine({0,y1},{scr.x,y1}, AF::AMBER, 2.f);

        float mid = y0 + h * .5f;

        // Left: Logo
        ImGui::SetWindowFontScale(1.6f);
        dl->AddText(ImGui::GetFont(),ImGui::GetFontSize(),
                    {16, mid - ImGui::GetFontSize()*.5f - 8.f},
                    AF::AMBER, ">> ALGOFORGES");
        ImGui::SetWindowFontScale(1.f);
        dl->AddText({18, mid + 4.f}, AF::TXT_DIM,
                    "STRATEGIC COMMAND CENTER  v2.4.1");

        // Separator
        dl->AddLine({260, y0+6},{260, y1-6}, AF::Alpha(AF::AMBER,0.3f), 1.f);

        // Status chips
        struct Chip { const char* label; ImU32 col; float x; };
        Chip chips[] = {
            {"[*] FORGE ONLINE",  AF::GREEN,  280.f },
            {"[~] AI ENGINE",     AF::CYAN,   420.f },
            {"[#] DB SYNC",       AF::AMBER,  540.f },
        };
        for(auto& c : chips){
            float pulse = 0.7f + 0.3f*sinf(m_time*2.f + c.x);
            dl->AddText({c.x, mid - 7.f}, AF::Alpha(c.col, pulse), c.label);
        }

        // Right: session timer
        char timeBuf[32];
        snprintf(timeBuf, sizeof(timeBuf), "SESSION  %05.1fs", m_time);
        ImVec2 tsz = ImGui::CalcTextSize(timeBuf);
        dl->AddText({scr.x - tsz.x - 150.f, mid - 7.f}, AF::TXT_MID, timeBuf);

        // Username badge
        float badgeX = scr.x - 138.f;
        float badgeW = 128.f, badgeH = 22.f;
        float badgeY = mid - badgeH * .5f;
        dl->AddRectFilled({badgeX, badgeY},{badgeX+badgeW, badgeY+badgeH},
                          AF::Alpha(AF::AMBER, 0.12f), 4.f);
        dl->AddRect({badgeX, badgeY},{badgeX+badgeW, badgeY+badgeH},
                    AF::Alpha(AF::AMBER, 0.45f), 4.f, 0, 1.f);
        ImVec2 usz = ImGui::CalcTextSize("[ FORGE_USR ]");
        dl->AddText({badgeX + (badgeW-usz.x)*.5f, badgeY + (badgeH-usz.y)*.5f},
                    AF::Alpha(AF::AMBER, 0.9f), "[ FORGE_USR ]");
    }

    // ─────────────────────────────────────────────────────────────
    //  COLUMN 1
    // ─────────────────────────────────────────────────────────────
    void DrawColumn1(ImDrawList* dl, ImVec2 pos, ImVec2 sz, float dt){
        float x=pos.x, y=pos.y, w=sz.x;
        float panelGap = 8.f;

        // ── Problem Input Panel ───────────────────────────────────
        float probH = sz.y * 0.32f;
        DrawProblemPanel(dl, {x,y}, {w, probH}, dt);

        y += probH + panelGap;

        // ── LeetCode Similarity Panel ─────────────────────────────
        float simH = sz.y * 0.33f;
        DrawSimilarityPanel(dl, {x,y}, {w, simH}, dt);

        y += simH + panelGap;

        // ── Edge Cases Panel ──────────────────────────────────────
        float edgeH = sz.y - (probH + simH + panelGap*2.f);
        DrawEdgeCasePanel(dl, {x,y}, {w, edgeH}, dt);
    }

    // ─────────────────────────────────────────────────────────────
    //  COLUMN 2
    // ─────────────────────────────────────────────────────────────
    void DrawColumn2(ImDrawList* dl, ImVec2 pos, ImVec2 sz, float dt){
        float x=pos.x, y=pos.y, w=sz.x;
        float panelGap = 8.f;

        // ── Approach Panels (brute + optimized side by side) ──────
        float approachH = sz.y * 0.36f;
        float halfW = (w - panelGap) * 0.5f;

        DrawApproachPanel(dl, {x,y},          {halfW, approachH}, dt, false);
        DrawApproachPanel(dl, {x+halfW+panelGap,y}, {halfW, approachH}, dt, true);

        y += approachH + panelGap;

        // ── Code Editor Panel ─────────────────────────────────────
        float codeH = sz.y - approachH - panelGap;
        DrawCodeEditorPanel(dl, {x,y}, {w, codeH}, dt);
    }

    // ─────────────────────────────────────────────────────────────
    //  COLUMN 3
    // ─────────────────────────────────────────────────────────────
    void DrawColumn3(ImDrawList* dl, ImVec2 pos, ImVec2 sz, float dt){
        float x=pos.x, y=pos.y, w=sz.x;
        float panelGap = 8.f;

        // ── Uniqueness Score ──────────────────────────────────────
        float uniH = sz.y * 0.20f;
        DrawUniquenessPanel(dl, {x,y}, {w, uniH}, dt);
        y += uniH + panelGap;

        // ── Complexity Radar ──────────────────────────────────────
        float radarH = sz.y * 0.30f;
        DrawRadarPanel(dl, {x,y}, {w, radarH}, dt);
        y += radarH + panelGap;

        // ── Stats / Complexity Tags ───────────────────────────────
        float statsH = sz.y * 0.17f;
        DrawStatsPanel(dl, {x,y}, {w, statsH}, dt);
        y += statsH + panelGap;

        // ── User Proficiency ──────────────────────────────────────
        float profH = sz.y - uniH - radarH - statsH - panelGap*3.f;
        DrawProficiencyPanel(dl, {x,y}, {w, profH}, dt);
    }

    // ═══════════════════════════════════════════════════════════════
    //  PANEL IMPLEMENTATIONS
    // ═══════════════════════════════════════════════════════════════

    // ── PROBLEM INPUT ─────────────────────────────────────────────
    void DrawProblemPanel(ImDrawList* dl, ImVec2 pos, ImVec2 sz, float dt){
        m_scanProblem.Update(dt, sz.y);
        Panel::Begin(dl, pos, sz, "PROBLEM STATEMENT", AF::AMBER, m_time,
                     true, &m_scanProblem);

        float ix=pos.x+10, iy=pos.y+32, iw=sz.x-20, ih=sz.y-32-44;

        // Background + border for the text area
        dl->AddRectFilled({ix,iy},{ix+iw,iy+ih}, AF::BG_INPUT, 4.f);
        dl->AddRect({ix,iy},{ix+iw,iy+ih},
                    AF::Alpha(AF::AMBER,0.25f), 4.f, 0, 1.f);

        // ── Scrollable text area ──────────────────────────────────
        // BeginChild forces AlwaysVerticalScrollbar.
        // InputTextMultiline width = child_width − scrollbar_width so text
        // wraps exactly at the scrollbar — no horizontal overflow ever.
        const float sbSz = 10.f;
        ImGui::SetCursorScreenPos({ix, iy});
        ImGui::PushStyleColor(ImGuiCol_ChildBg,              AF::ToV4(AF::BG_INPUT));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,          AF::ToV4(AF::Alpha(AF::BG_PANEL2,0.9f)));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        AF::ToV4(AF::Alpha(AF::AMBER,0.65f)));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, AF::ToV4(AF::Alpha(AF::AMBER,0.90f)));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  AF::ToV4(AF::AMBER));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, sbSz);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 0.f});
        ImGui::SetWindowFontScale(1.15f);

        ImGui::BeginChild("##probwrap", {iw, ih}, ImGuiChildFlags_None,
                          ImGuiWindowFlags_AlwaysVerticalScrollbar
                          | ImGuiWindowFlags_NoMove);

        ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_Text,           AF::ToV4(AF::TXT_BRIGHT));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, AF::ToV4(AF::Alpha(AF::AMBER,0.3f)));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {6.f, 6.f});

        // Width = child width minus scrollbar → wraps before reaching scrollbar
        // ImGuiInputTextFlags_NoHorizontalScroll enforces wrap-at-width
        float wrapW = iw - sbSz - 2.f;
        ImGui::InputTextMultiline("##problem",
            m_problemBuf, sizeof(m_problemBuf),
            {wrapW, ih},
            ImGuiInputTextFlags_NoHorizontalScroll);

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();
        ImGui::EndChild();

        ImGui::SetWindowFontScale(1.f);
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(5);

        // Placeholder hint drawn over child when buffer is empty
        if(m_problemBuf[0] == 0)
            dl->AddText({ix+8, iy+8}, AF::TXT_DIM,
                        "Paste your problem statement here...");

        // ── Buttons ──────────────────────────────────────────────
        float btnY = pos.y + sz.y - 38;
        float btnW = sz.x * 0.55f - 14.f;
        float clrW = sz.x * 0.45f - 18.f;

        DrawForgeButton(dl, {ix, btnY}, {btnW, 30}, "[!] FORGE ANALYSIS", AF::AMBER, m_time);
        ImGui::SetCursorScreenPos({ix, btnY});
        ImGui::InvisibleButton("##btnforge", {btnW, 30});
        if(ImGui::IsItemClicked() && m_problemBuf[0] != 0){
            m_analysisRunning  = true;
            m_analysisProgress = 0.f;
        }

        DrawForgeButton(dl, {ix+btnW+8, btnY}, {clrW, 30}, "[X] CLEAR", AF::RED, m_time);
        ImGui::SetCursorScreenPos({ix+btnW+8, btnY});
        ImGui::InvisibleButton("##btnclear", {clrW, 30});
        if(ImGui::IsItemClicked()){
            memset(m_problemBuf, 0, sizeof(m_problemBuf));
            m_analysisRunning  = false;
            m_analysisProgress = 0.f;
        }

        // Progress bar (bottom edge of panel)
        if(m_analysisRunning){
            m_analysisProgress = min(1.f, m_analysisProgress + dt * 0.15f);
            if(m_analysisProgress >= 1.f) m_analysisRunning = false;
            float bx=pos.x+2, by=pos.y+sz.y-4, bw=sz.x-4;
            dl->AddRectFilled({bx,by},{bx+bw*m_analysisProgress,by+3}, AF::AMBER);
        }
    }

    // ── SIMILARITY PANEL ──────────────────────────────────────────
    void DrawSimilarityPanel(ImDrawList* dl, ImVec2 pos, ImVec2 sz, float dt){
        m_scanSim.Update(dt, sz.y);
        Panel::Begin(dl, pos, sz, "LEETCODE SIMILARITY", AF::CYAN, m_time,
                     true, &m_scanSim);

        float cx  = pos.x + 10.f;
        float cy  = pos.y + 34.f;
        float rw  = sz.x - 20.f;
        float rowH = (sz.y - 42.f) / (float)m_similarities.size();

        // Pixel budget within rw (relative to cx):
        //  [0    .. 54)          difficulty badge  (52px)
        //  [57   .. 97)          problem number    (40px, clipped)
        //  [97   .. barL-6)      title             (clipped)
        //  [barL .. barL+barW)   similarity bar    (76px)
        //  [barL+barW+4 .. rw)   percentage        (clipped to right edge)
        const float badgeW = 52.f;
        const float numOff = 57.f, numW = 40.f;
        const float titOff = numOff + numW;          // 97px
        const float pctW   = 32.f, barW = 76.f;
        const float barOff = rw - barW - pctW - 4.f;
        const float titMaxW = barOff - titOff - 6.f;

        for(int i=0; i<(int)m_similarities.size(); i++){
            auto& s  = m_similarities[i];
            float ry = cy + i * rowH;

            if(i%2==0)
                dl->AddRectFilled({cx,ry},{cx+rw,ry+rowH-2},
                                  AF::Alpha(AF::BG_PANEL2,0.6f),2.f);

            // Difficulty badge
            ImU32 dCol = (s.difficulty=="Easy")  ? AF::GREEN  :
                         (s.difficulty=="Medium") ? AF::YELLOW : AF::RED;
            float by1=ry+4.f, by2=ry+rowH-6.f;
            dl->AddRectFilled({cx+2,by1},{cx+2+badgeW,by2},AF::Alpha(dCol,0.15f),3.f);
            dl->AddRect      ({cx+2,by1},{cx+2+badgeW,by2},AF::Alpha(dCol,0.50f),3.f,0,1.f);
            ImVec2 dsz=ImGui::CalcTextSize(s.difficulty.c_str());
            dl->AddText({cx+2+(badgeW-dsz.x)*.5f, ry+(rowH-dsz.y)*.5f},
                        dCol, s.difficulty.c_str());

            // Problem number
            dl->PushClipRect({cx+numOff,ry},{cx+numOff+numW,ry+rowH},true);
            dl->AddText({cx+numOff,ry+4}, AF::TXT_DIM, s.number.c_str());
            dl->PopClipRect();

            // Title
            dl->PushClipRect({cx+titOff,ry},{cx+titOff+titMaxW,ry+rowH},true);
            dl->AddText({cx+titOff,ry+4}, AF::TXT_BRIGHT, s.title.c_str());
            dl->PopClipRect();

            // Bar
            float absBarX = cx + barOff;
            float barH2   = 8.f;
            float barY    = ry + (rowH-barH2)*.5f;
            dl->AddRectFilled({absBarX,barY},{absBarX+barW,barY+barH2},
                              AF::Alpha(AF::BORDER,0.6f),2.f);
            ImU32 barCol = s.similarity>0.75f ? AF::RED   :
                           s.similarity>0.5f  ? AF::AMBER : AF::GREEN;
            float animW  = barW*s.similarity*(0.95f+0.05f*sinf(m_time*1.5f+i));
            dl->AddRectFilled({absBarX,barY},{absBarX+animW,barY+barH2},barCol,2.f);

            // Percentage
            char pct[8]; snprintf(pct,sizeof(pct),"%.0f%%",s.similarity*100.f);
            float pctX = absBarX + barW + 4.f;
            dl->PushClipRect({pctX,ry},{cx+rw,ry+rowH},true);
            dl->AddText({pctX,barY-1.f}, AF::Alpha(barCol,0.9f), pct);
            dl->PopClipRect();
        }
    }

    // ── EDGE CASE PANEL ───────────────────────────────────────────
    void DrawEdgeCasePanel(ImDrawList* dl, ImVec2 pos, ImVec2 sz, float dt){
        m_scanEdge.Update(dt, sz.y);
        Panel::Begin(dl, pos, sz, "PREDICTED EDGE CASES", AF::RED, m_time,
                     true, &m_scanEdge);

        float cx  = pos.x + 10.f;
        float cy  = pos.y + 34.f;
        float rw  = sz.x - 20.f;
        float rowH = max(1.f,(sz.y - 42.f) / (float)m_edgeCases.size());

        // ── Column pixel budget (relative to cx, within rw) ──────
        //  [0   .. 14)   critical dot          (dot at cx+6)
        //  [14  .. descW) description           (clipped)
        //  [descW .. descW+exW) example code    (clipped)
        //  [rw-badgeW .. rw) YES/NO badge       (fixed right)
        const float badgeW   = 36.f;   // width of YES/NO badge
        const float exW      = min(rw * 0.38f, 150.f); // example zone
        const float descMaxW = rw - exW - badgeW - 20.f; // description clip width
        const float exX      = cx + 14.f + descMaxW + 4.f; // example starts here
        const float badgeX   = cx + rw - badgeW;           // badge starts here

        for(int i = 0; i < (int)m_edgeCases.size(); i++){
            auto& e  = m_edgeCases[i];
            float ry = cy + i * rowH;

            // Alternating row background
            if(i % 2 == 0)
                dl->AddRectFilled({cx, ry},{cx+rw, ry+rowH-1},
                                  AF::Alpha(AF::BG_PANEL2, 0.45f), 2.f);

            // ── Critical dot ──────────────────────────────────────
            ImU32 mCol = e.critical ? AF::RED : AF::TXT_DIM;
            float dotAlpha = e.critical
                ? (0.6f + 0.4f * sinf(m_time * 3.f + i))
                : 0.35f;
            dl->AddCircleFilled({cx + 6.f, ry + rowH * .5f}, 4.f,
                                AF::Alpha(mCol, dotAlpha), 8);

            // ── Description (clipped) ─────────────────────────────
            dl->PushClipRect({cx+14.f, ry},{cx+14.f+descMaxW, ry+rowH}, true);
            dl->AddText({cx + 16.f, ry + 3.f}, AF::TXT_BRIGHT,
                        e.description.c_str());
            dl->PopClipRect();

            // ── Example code box (clipped) ────────────────────────
            dl->AddRectFilled({exX - 4.f, ry+2.f},{exX + exW, ry+rowH-4.f},
                              AF::Alpha(AF::BG_INPUT, 0.85f), 3.f);
            dl->PushClipRect({exX, ry},{exX + exW, ry+rowH}, true);
            dl->AddText({exX + 2.f, ry + 4.f}, AF::TXT_CYAN, e.example.c_str());
            dl->PopClipRect();

            // ── YES / NO badge ────────────────────────────────────
            ImU32 yesCol = AF::GREEN;
            ImU32 noCol  = AF::Alpha(AF::RED, 0.80f);
            ImU32 bCol   = e.present ? yesCol : noCol;
            const char* bTxt = e.present ? "YES" : "NO";

            float bPulse = e.present
                ? (0.85f + 0.15f * sinf(m_time * 2.f + i))
                : 0.70f;

            dl->AddRectFilled({badgeX,     ry+3.f},
                              {badgeX+badgeW, ry+rowH-3.f},
                              AF::Alpha(bCol, 0.18f), 3.f);
            dl->AddRect      ({badgeX,     ry+3.f},
                              {badgeX+badgeW, ry+rowH-3.f},
                              AF::Alpha(bCol, bPulse), 3.f, 0, 1.2f);

            ImVec2 tsz = ImGui::CalcTextSize(bTxt);
            dl->AddText({badgeX  + (badgeW - tsz.x) * .5f,
                         ry      + (rowH   - tsz.y) * .5f},
                        AF::Alpha(bCol, bPulse), bTxt);

            // ── Row separator ─────────────────────────────────────
            if(i < (int)m_edgeCases.size() - 1)
                dl->AddLine({cx, ry+rowH-1.f},{cx+rw, ry+rowH-1.f},
                            AF::Alpha(AF::BORDER, 0.45f), 0.5f);
        }
    }

    // ── APPROACH PANEL ────────────────────────────────────────────
    void DrawApproachPanel(ImDrawList* dl, ImVec2 pos, ImVec2 sz,
                           float dt, bool isOptimized){
        ScanLine& sc = isOptimized ? m_scanOpt : m_scanBrute;
        sc.Update(dt, sz.y);

        ImU32 acol    = isOptimized ? AF::GREEN : AF::RED;
        const char* title = isOptimized ? "OPTIMIZED APPROACH" : "BRUTE FORCE";
        Panel::Begin(dl, pos, sz, title, acol, m_time, true, &sc);

        const string& code = isOptimized ? m_optimized : m_bruteForce;

        float cx=pos.x+10, cy=pos.y+34;
        float cw=sz.x-20, ch=sz.y-42-26.f; // reserve 26px for complexity badge

        // ── H+V scrollable code view ──────────────────────────
        float gutW2  = 30.f;
        float sbSzAp = 10.f;

        // Fixed gutter
        dl->AddRectFilled({cx,cy},{cx+gutW2,cy+ch}, AF::Alpha(AF::BG_PANEL2,0.95f));
        dl->AddLine({cx+gutW2,cy},{cx+gutW2,cy+ch}, AF::Alpha(acol,0.25f), 1.f);
        dl->AddRectFilled({cx+gutW2,cy},{cx+cw,cy+ch}, AF::BG_INPUT);
        dl->AddRect({cx,cy},{cx+cw,cy+ch}, AF::Alpha(acol,0.28f), 2.f, 0, 1.f);

        // BeginChild — gets both scrollbars automatically
        ImGui::SetCursorScreenPos({cx+gutW2, cy});
        ImGui::PushStyleColor(ImGuiCol_ChildBg,              AF::ToV4(AF::BG_INPUT));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,          AF::ToV4(AF::Alpha(AF::BG_PANEL2,0.9f)));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        AF::ToV4(AF::Alpha(acol,0.60f)));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, AF::ToV4(AF::Alpha(acol,0.88f)));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  AF::ToV4(acol));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, sbSzAp);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f,0.f});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   {0.f,0.f});

        string apprChildId = string("##appr") + (isOptimized?"opt":"brute");
        ImGui::BeginChild(apprChildId.c_str(), {cw-gutW2, ch},
                          ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar
                          | ImGuiWindowFlags_AlwaysVerticalScrollbar
                          | ImGuiWindowFlags_AlwaysHorizontalScrollbar
                          | ImGuiWindowFlags_NoMove);

        float apScrollY = ImGui::GetScrollY();
        float apScrollX = ImGui::GetScrollX();
        float lineH2    = 17.f;
        ImDrawList* apl = ImGui::GetWindowDrawList();
        ImVec2      awp = ImGui::GetWindowPos();

        // Gutter line numbers (parent dl, screen-space — always safe)
        {
            int   first = (int)(apScrollY / lineH2);
            float top   = cy - fmodf(apScrollY, lineH2);
            int   vis   = (int)(ch / lineH2) + 2;
            for(int gi=0; gi<vis; gi++){
                float gy = top + gi * lineH2;
                if(gy >= cy && gy < cy + ch - sbSzAp){
                    char buf[8]; snprintf(buf,sizeof(buf),"%2d", first+gi+1);
                    dl->AddText({cx+3, gy+2}, AF::TXT_DIM, buf);
                }
            }
        }

        // Parse code into lines + find widest
        vector<pair<string,ImU32>> apLines;
        float maxApW = cw - gutW2 - sbSzAp;
        {
            string rem = code;
            while(!rem.empty()){
                size_t nl = rem.find('\n');
                string ln = (nl==string::npos) ? rem : rem.substr(0,nl);
                rem = (nl==string::npos) ? "" : rem.substr(nl+1);
                ImU32 col = (ln.rfind("//",0)==0)   ? AF::CYAN_DIM :
                            (ln.find("for")==0 || ln.find("if")==0 ||
                             ln.find("return")==0 || ln.find("int")==0 ||
                             ln.find("auto")==0  || ln.find("un")==0)
                                                    ? AF::TXT_AMBER
                                                    : AF::TXT_BRIGHT;
                apLines.push_back({ln, col});
                float w = ImGui::CalcTextSize(ln.c_str()).x + 16.f;
                if(w > maxApW) maxApW = w;
            }
        }

        // Render: Dummy sets content size, text drawn directly into child drawlist
        for(int li=0; li<(int)apLines.size(); li++){
            float ly = li * lineH2;
            ImGui::SetCursorPos({0.f, ly});
            ImGui::Dummy({maxApW, lineH2});
            float sy = awp.y + ly - apScrollY;
            if(sy + lineH2 >= cy && sy <= cy+ch && !apLines[li].first.empty())
                apl->AddText({awp.x + 4.f - apScrollX, sy + 2.f},
                             apLines[li].second, apLines[li].first.c_str());
        }
        // Terminal dummy to set full content height
        ImGui::SetCursorPos({maxApW, (float)apLines.size()*lineH2 + 4.f});
        ImGui::Dummy({1.f,1.f});

        ImGui::EndChild();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(5);

        // Complexity badge bottom
        float bdY = pos.y + sz.y - 22;
        const char* cmplx = isOptimized ? "O(n log n)  /  O(n)" : "O(n²)  /  O(1)";
        dl->AddRectFilled({cx,bdY-2},{cx+cw,bdY+18},
                          AF::Alpha(acol,0.08f));
        dl->AddLine({cx,bdY-2},{cx+cw,bdY-2},
                    AF::Alpha(acol,0.3f),1.f);
        ImVec2 csz = ImGui::CalcTextSize(cmplx);
        dl->AddText({cx+(cw-csz.x)*.5f, bdY+1},
                    AF::Alpha(acol,0.9f), cmplx);
    }

    // ── CODE EDITOR PANEL ─────────────────────────────────────────
    void DrawCodeEditorPanel(ImDrawList* dl, ImVec2 pos, ImVec2 sz, float dt){
        m_scanCode.Update(dt, sz.y);

        // Split panel: top 65% = editor, bottom 35% = tool panel
        float editorH  = sz.y * 0.65f;
        float toolH    = sz.y - editorH - 6.f;

        // ════════════════════════════════════════════════════════
        //  EDITOR SECTION
        // ════════════════════════════════════════════════════════
        Panel::Begin(dl, pos, {sz.x, editorH}, "CODE WORKSPACE",
                     AF::PURPLE, m_time, true, &m_scanCode);

        struct TabInfo {
            const char* name;
            bool        editable;
            ImU32       accent;
            const char* badge;   // shown right of tab name
        };
        // 3 tabs only: solution (RO), brute (RO), notes (RW)
        TabInfo tabs[3] = {
            {"solution.cpp", false, AF::CYAN,   "[R]"},
            {"brute.cpp",    false, AF::RED,    "[R]"},
            {"notes.md",     true,  AF::AMBER,  "[W]"},
        };

        // ── Tab bar ───────────────────────────────────────────
        float tx=pos.x+10, ty=pos.y+30;
        float tabW=124.f, tabH=24.f;   // wide enough for name + [R]/[W] with no overlap
        for(int i=0;i<3;i++){
            float tbx=tx+i*(tabW+3);
            bool sel=(m_selectedTab==i);
            ImU32 ac = tabs[i].accent;

            dl->AddRectFilled({tbx,ty},{tbx+tabW,ty+tabH},
                sel ? AF::Alpha(ac,0.20f) : AF::Alpha(AF::BG_INPUT,0.5f), 4.f);
            dl->AddRect({tbx,ty},{tbx+tabW,ty+tabH},
                AF::Alpha(ac, sel?0.9f:0.22f), 4.f, 0, sel?1.8f:0.8f);

            if(sel) dl->AddRectFilled({tbx+4,ty+tabH-2},{tbx+tabW-4,ty+tabH},ac);

            // Draw [R]/[W] badge first so we know where to stop the filename clip
            ImVec2 bsz = ImGui::CalcTextSize(tabs[i].badge);
            float  badgeX = tbx + tabW - bsz.x - 6.f;
            dl->AddText({badgeX, ty+(tabH-bsz.y)*.5f},
                        tabs[i].editable ? AF::Alpha(AF::GREEN,0.7f)
                                         : AF::Alpha(AF::RED, 0.7f),
                        tabs[i].badge);

            // File name — clipped so it never reaches the badge
            dl->PushClipRect({tbx+8.f, ty},{badgeX-4.f, ty+tabH}, true);
            ImVec2 tsz = ImGui::CalcTextSize(tabs[i].name);
            dl->AddText({tbx+8.f, ty+(tabH-tsz.y)*.5f},
                        sel ? ac : AF::TXT_DIM, tabs[i].name);
            dl->PopClipRect();

            ImGui::SetCursorScreenPos({tbx,ty});
            ImGui::InvisibleButton(("##tab"+to_string(i)).c_str(),{tabW,tabH});
            if(ImGui::IsItemClicked()) m_selectedTab=i;
        }

        // ── Mode badge (right of the 3 tabs) ────────────────────
        float       bdX     = tx + 3*(tabW+3) + 8;   // 3 tabs then gap
        float       bdW     = 84.f;                   // wide enough for "READ-ONLY"
        const char* modeTxt = tabs[m_selectedTab].editable ? "EDITABLE" : "READ-ONLY";
        ImU32       modeCol = tabs[m_selectedTab].editable ? AF::GREEN  : AF::RED;
        dl->AddRectFilled({bdX,ty},{bdX+bdW,ty+tabH}, AF::Alpha(modeCol,0.12f),4.f);
        dl->AddRect      ({bdX,ty},{bdX+bdW,ty+tabH}, AF::Alpha(modeCol,0.50f),4.f,0,1.f);
        ImVec2 msz=ImGui::CalcTextSize(modeTxt);
        dl->AddText({bdX+(bdW-msz.x)*.5f, ty+(tabH-msz.y)*.5f}, modeCol, modeTxt);

        // ── Editor body ───────────────────────────────────────
        float ey = ty+tabH+5, ex = pos.x+10;
        float ew = sz.x-20;
        float eh = pos.y+editorH-ey-34;

        float gutW = 36.f;
        ImU32 curAc = tabs[m_selectedTab].accent;

        // ── Code editor: BeginChild for true H+V scrollbars ─
        float sbSzEd = 10.f;
        float edInW  = ew - gutW;
        float lhEd   = 18.f;

        // Gutter background (fixed, outside child)
        dl->AddRectFilled({ex,ey},{ex+gutW,ey+eh}, AF::Alpha(AF::BG_PANEL2,0.95f));
        dl->AddLine({ex+gutW,ey},{ex+gutW,ey+eh},  AF::Alpha(curAc,0.25f), 1.f);
        dl->AddRectFilled({ex+gutW,ey},{ex+ew,ey+eh}, AF::BG_INPUT);
        dl->AddRect({ex,ey},{ex+ew,ey+eh}, AF::Alpha(curAc,0.30f), 0.f, 0, 1.f);

        ImGui::SetCursorScreenPos({ex+gutW, ey});
        ImGui::PushStyleColor(ImGuiCol_ChildBg,              AF::ToV4(AF::BG_INPUT));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,          AF::ToV4(AF::Alpha(AF::BG_PANEL2,0.9f)));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        AF::ToV4(AF::Alpha(curAc,0.60f)));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, AF::ToV4(AF::Alpha(curAc,0.88f)));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  AF::ToV4(curAc));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, sbSzEd);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f,0.f});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   {0.f,0.f});
        ImGui::SetWindowFontScale(1.12f);

        string edChildId = "##edchild" + to_string(m_selectedTab);
        ImGui::BeginChild(edChildId.c_str(), {edInW, eh},
                          ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar
                          | ImGuiWindowFlags_AlwaysVerticalScrollbar
                          | ImGuiWindowFlags_AlwaysHorizontalScrollbar
                          | ImGuiWindowFlags_NoMove);

        float edScrollY = ImGui::GetScrollY();
        float edScrollX = ImGui::GetScrollX();
        ImDrawList* edl = ImGui::GetWindowDrawList();
        ImVec2      ewp = ImGui::GetWindowPos();

        // Gutter numbers (parent dl, always safe)
        {
            int   firstLn = (int)(edScrollY / lhEd);
            float base    = ey - fmodf(edScrollY, lhEd);
            for(int gi=0; gi < (int)(eh/lhEd)+2; gi++){
                float gy = base + gi * lhEd;
                if(gy >= ey && gy < ey+eh-sbSzEd){
                    char b[8]; snprintf(b,sizeof(b),"%3d",firstLn+gi+1);
                    dl->AddText({ex+2, gy+3}, AF::TXT_DIM, b);
                }
            }
        }

        // Parse buffer into lines
        vector<string> edLines;
        {
            const char* p = m_tabBuf[m_selectedTab];
            while(*p){
                const char* e = p;
                while(*e && *e!='\n') e++;
                edLines.push_back(string(p,e));
                p = (*e=='\n') ? e+1 : e;
            }
            if(edLines.empty()) edLines.push_back("");
        }

        // Find widest line for H scrollbar range
        float maxEdW = edInW - sbSzEd;
        for(auto& ln : edLines){
            float w = ImGui::CalcTextSize(ln.c_str()).x + 20.f;
            if(w > maxEdW) maxEdW = w;
        }

        if(tabs[m_selectedTab].editable){
            // ── notes.md ─────────────────────────────────────────────────────
            // CURSOR ALIGNMENT FIX:
            //   ImGui draws the cursor at: child_origin.y + FramePadding.y + line*fontLH
            //   We draw visible text at:   ewp.y + li*fontLH - edScrollY
            //   Setting FramePadding {0,0} makes both identical — cursor aligned.
            //   We also use GetTextLineHeight() as the step so ImGui's internal
            //   stepping matches our draw loop exactly.

            const float fontLH = ImGui::GetTextLineHeight();

            // 1. Visible text
            for(int li=0; li<(int)edLines.size(); li++){
                float ly = li * fontLH;
                ImGui::SetCursorPos({0.f, ly});
                ImGui::Dummy({maxEdW, fontLH});
                float sy = ewp.y + ly - edScrollY;
                if(sy + fontLH >= ey && sy <= ey+eh){
                    auto& ln = edLines[li];
                    ImU32 tc = AF::TXT_BRIGHT;
                    if(!ln.empty()){
                        if(ln.rfind("# ",0)==0 || ln.rfind("## ",0)==0 ||
                           ln.rfind("### ",0)==0)
                            tc = AF::AMBER;
                        else if(ln.rfind("- ",0)==0 || ln.rfind("* ",0)==0)
                            tc = AF::CYAN;
                        else if(ln.rfind("//",0)==0)
                            tc = AF::TXT_DIM;
                    }
                    if(!ln.empty())
                        edl->AddText({ewp.x + 6.f - edScrollX, sy}, tc, ln.c_str());
                }
            }
            // Terminal dummy — sets full content height for scrollbar
            ImGui::SetCursorPos({maxEdW, (float)edLines.size()*fontLH + 4.f});
            ImGui::Dummy({1.f,1.f});

            // 2. Invisible overlay InputTextMultiline
            //    FramePadding {0,0} → line 0 starts at child origin y=0,
            //    matching where visible text is drawn → cursor is pixel-perfect.
            ImGui::SetCursorPos({0.f, 0.f});
            ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_Text,           ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, AF::ToV4(AF::Alpha(curAc,0.35f)));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.f, 0.f}); // KEY FIX
            ImGui::InputTextMultiline(
                ("##tabcontent"+to_string(m_selectedTab)).c_str(),
                m_tabBuf[m_selectedTab], sizeof(m_tabBuf[0]),
                {maxEdW, (float)edLines.size() * fontLH},  // exact height
                ImGuiInputTextFlags_AllowTabInput);
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
        } else {
            // ── Read-only tabs (solution/brute): Dummy + direct text draw ──
            const float fontLH = ImGui::GetTextLineHeight();
            for(int li=0; li<(int)edLines.size(); li++){
                float ly = li * fontLH;
                ImGui::SetCursorPos({0.f, ly});
                ImGui::Dummy({maxEdW, fontLH});
                float sy = ewp.y + ly - edScrollY;
                if(sy + fontLH >= ey && sy <= ey+eh){
                    auto& ln = edLines[li];
                    ImU32 tc = AF::TXT_BRIGHT;
                    if(!ln.empty()){
                        if(ln.rfind("//",0)==0) tc = AF::CYAN_DIM;
                        else if(ln.find("#include")==0||ln.find("#define")==0)
                            tc = AF::Alpha(AF::PURPLE,0.9f);
                        else if(ln.find("class ")==0||ln.find("struct ")==0)
                            tc = AF::AMBER;
                    }
                    if(!ln.empty())
                        edl->AddText({ewp.x+6.f-edScrollX, sy}, tc, ln.c_str());
                }
            }
            ImGui::SetCursorPos({maxEdW, (float)edLines.size()*fontLH+4.f});
            ImGui::Dummy({1.f,1.f});
        }

        ImGui::EndChild();
        ImGui::SetWindowFontScale(1.f);
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(5);

        // ── Bottom action bar ─────────────────────────────────
        float abY = pos.y+editorH-32;
        dl->AddRectFilled({pos.x,abY},{pos.x+sz.x,pos.y+editorH},
                          AF::Alpha(AF::BG_PANEL2,0.97f));
        dl->AddLine({pos.x,abY},{pos.x+sz.x,abY},
                    AF::Alpha(curAc,0.4f),1.f);

        float bx=pos.x+10;
        struct Btn2{const char* label; ImU32 col;};
        Btn2 btns[]={{"[>] RUN",AF::GREEN},{"[!] SUBMIT",AF::AMBER},
                     {"[~] RESET",AF::RED},{"[v] EXPORT",AF::CYAN}};
        for(auto& b:btns){
            DrawForgeButton(dl,{bx,abY+4},{88,22},b.label,b.col,m_time);
            bx+=94;
        }

        // Right status
        int lineCount=0;
        for(char* c=m_tabBuf[m_selectedTab];*c;c++) if(*c=='\n') lineCount++;
        char stBuf[48];
        snprintf(stBuf,sizeof(stBuf),"LINES: %d  |  C++17  |  UTF-8",lineCount+1);
        ImVec2 stsz=ImGui::CalcTextSize(stBuf);
        dl->AddText({pos.x+sz.x-stsz.x-12, abY+8}, AF::TXT_DIM, stBuf);

        // ════════════════════════════════════════════════════════
        //  TOOL PANEL  (below editor)
        // ════════════════════════════════════════════════════════
        float tp_y = pos.y + editorH + 6.f;
        DrawToolPanel(dl, {pos.x, tp_y}, {sz.x, toolH}, dt);
    }

    // ─────────────────────────────────────────────────────────────
    //  TOOL PANEL: Test Generator | Debugger | Code Comparison
    // ─────────────────────────────────────────────────────────────
    void DrawToolPanel(ImDrawList* dl, ImVec2 pos, ImVec2 sz, float dt){
        m_scanTool.Update(dt, sz.y);

        // Animate progress bars
        if(m_testRunning){
            m_testProgress += dt * 0.4f;
            if(m_testProgress >= 1.f){ m_testRunning=false; m_testProgress=1.f; }
        }
        if(m_debugRunning){
            m_debugProgress += dt * 0.55f;
            if(m_debugProgress >= 1.f){ m_debugRunning=false; m_debugProgress=1.f; }
        }
        if(m_cmpRunning){
            m_cmpProgress += dt * 0.5f;
            if(m_cmpProgress >= 1.f){ m_cmpRunning=false; m_cmpProgress=1.f; }
        }

        // Single-mode panel: Test Generator from problem statement
        Panel::Begin(dl, pos, sz, "[+] TEST GENERATOR  //  FROM PROBLEM STATEMENT",
                     AF::GREEN, m_time, true, &m_scanTool);

        float cy=pos.y+30, cx=pos.x+10, cw=sz.x-20;
        float ch=sz.y-30-8;  // purely relative height

        {  // Test Generator (only mode)
            // ── LEFT: problem statement source (read-only preview) ─
            float lw = cw * 0.32f;
            dl->AddRectFilled({cx,cy},{cx+lw,cy+ch},
                              AF::Alpha(AF::BG_INPUT,0.75f),4.f);
            dl->AddRect({cx,cy},{cx+lw,cy+ch},
                        AF::Alpha(AF::GREEN,0.28f),4.f,0,1.2f);

            // Header
            dl->AddRectFilled({cx,cy},{cx+lw,cy+22},
                              AF::Alpha(AF::GREEN,0.14f));
            dl->AddLine({cx,cy+22},{cx+lw,cy+22},
                        AF::Alpha(AF::GREEN,0.35f),1.f);
            dl->AddText({cx+8,cy+5},AF::GREEN,"SOURCE: PROBLEM STATEMENT");

            // Problem text preview: vertical scroll only, read-only
            float pvH = ch - 58.f;  // 22 header + 36 btn
            ImGui::SetCursorScreenPos({cx+4, cy+24});
            ImGui::PushStyleColor(ImGuiCol_ChildBg,              ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,          AF::ToV4(AF::Alpha(AF::BG_PANEL2,0.9f)));
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        AF::ToV4(AF::Alpha(AF::GREEN,0.55f)));
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, AF::ToV4(AF::Alpha(AF::GREEN,0.85f)));
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  AF::ToV4(AF::GREEN));
            ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 8.f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {4.f,4.f});
            ImGui::BeginChild("##pvchild",{lw-8, pvH},ImGuiChildFlags_None,
                              ImGuiWindowFlags_NoMove); // vertical only

            ImGui::PushStyleColor(ImGuiCol_FrameBg,  ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_Text,     AF::ToV4(AF::TXT_MID));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{3.f,3.f});
            char previewBuf[2048]; strncpy(previewBuf,m_problemBuf,sizeof(previewBuf)-1);
            ImGui::InputTextMultiline("##probpreview", previewBuf,
                sizeof(previewBuf), {-1.f,-1.f},
                ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor(2); ImGui::PopStyleVar();

            ImGui::EndChild();
            ImGui::PopStyleVar(2); ImGui::PopStyleColor(5);

            // Placeholder if empty
            if(m_problemBuf[0]==0)
                dl->AddText({cx+12, cy+32}, AF::Alpha(AF::TXT_DIM,0.6f),
                            "No problem statement yet.\nPaste one in the\nPROBLEM STATEMENT panel.");

            // [!] GENERATE TESTS button at bottom
            float gby = cy+ch-34;
            DrawForgeButton(dl,{cx+6,gby},{lw-12,28},
                            "[!] GENERATE FROM STATEMENT", AF::GREEN, m_time);
            ImGui::SetCursorScreenPos({cx+6,gby});
            ImGui::InvisibleButton("##gentest",{lw-12,28});
            if(ImGui::IsItemClicked()){
                m_testRunning=true; m_testProgress=0.f;
            }

            // Progress bar
            if(m_testRunning || m_testProgress>0.f){
                float pctW=(lw-12)*m_testProgress;
                dl->AddRectFilled({cx+6,gby-6},{cx+lw-6,gby-2},
                                  AF::Alpha(AF::BORDER,0.5f),2.f);
                dl->AddRectFilled({cx+6,gby-6},{cx+6+pctW,gby-2},
                                  AF::GREEN,2.f);
                if(m_testRunning){
                    char pctBuf[12];
                    snprintf(pctBuf,sizeof(pctBuf),"%.0f%%",m_testProgress*100);
                    dl->AddText({cx+lw*.5f-12,gby-18},AF::GREEN,pctBuf);
                }
            }

            // ── RIGHT: generated test cases table ──────────────
            float rx=cx+lw+8, rw=cw-lw-8;
            dl->AddRectFilled({rx,cy},{rx+rw,cy+ch},
                              AF::Alpha(AF::BG_PANEL2,0.65f),4.f);
            dl->AddRect({rx,cy},{rx+rw,cy+ch},
                        AF::Alpha(AF::GREEN,0.28f),4.f,0,1.2f);

            // Column headers
            float col0=rx+8, col1=rx+rw*0.38f, col2=rx+rw*0.68f, col3=rx+rw*0.83f;
            dl->AddRectFilled({rx,cy},{rx+rw,cy+24},AF::Alpha(AF::GREEN,0.16f));
            dl->AddLine({rx,cy+24},{rx+rw,cy+24},AF::Alpha(AF::GREEN,0.4f),1.f);
            dl->AddText({col0,cy+5},   AF::GREEN, "INPUT");
            dl->AddText({col1,cy+5},   AF::GREEN, "EXPECTED OUTPUT");
            dl->AddText({col2,cy+5},   AF::GREEN, "TYPE");
            dl->AddText({col3,cy+5},   AF::GREEN, "STATUS");

            float rh2 = min(22.f, (ch-32.f)/max(1,(int)m_generatedTests.size()));
            for(int i=0;i<(int)m_generatedTests.size();i++){
                float ry2=cy+26+i*rh2;
                if(ry2+rh2>cy+ch-4) break;
                auto& tc=m_generatedTests[i];

                // Alternating row bg
                if(i%2==0)
                    dl->AddRectFilled({rx,ry2},{rx+rw,ry2+rh2},
                                      AF::Alpha(AF::BG_PANEL,0.45f));

                // Input
                dl->PushClipRect({col0,ry2},{col1-4,ry2+rh2},true);
                dl->AddText({col0,ry2+3},AF::TXT_BRIGHT,tc.input.c_str());
                dl->PopClipRect();

                // Expected
                dl->PushClipRect({col1,ry2},{col2-4,ry2+rh2},true);
                dl->AddText({col1,ry2+3},AF::CYAN,tc.expected.c_str());
                dl->PopClipRect();

                // Type label (edge/normal/stress)
                dl->PushClipRect({col2,ry2},{col3-4,ry2+rh2},true);
                dl->AddText({col2,ry2+3},AF::Alpha(AF::AMBER,0.8f),tc.type.c_str());
                dl->PopClipRect();

                // Status badge
                dl->AddRectFilled({col3,ry2+2},{col3+46,ry2+rh2-2},
                                  AF::Alpha(tc.col,0.15f),3.f);
                dl->AddRect({col3,ry2+2},{col3+46,ry2+rh2-2},
                            AF::Alpha(tc.col,0.55f),3.f,0,1.f);
                ImVec2 ssz=ImGui::CalcTextSize(tc.status.c_str());
                dl->AddText({col3+(46-ssz.x)*.5f,ry2+(rh2-ssz.y)*.5f},
                            tc.col,tc.status.c_str());

                // Row separator
                if(i<(int)m_generatedTests.size()-1)
                    dl->AddLine({rx,ry2+rh2},{rx+rw,ry2+rh2},
                                AF::Alpha(AF::BORDER,0.4f),0.5f);
            }

            // Empty state hint
            if(m_generatedTests.empty()){
                const char* hint =
                    "Paste a problem statement, then click [!] GENERATE FROM STATEMENT";
                ImVec2 hsz=ImGui::CalcTextSize(hint);
                dl->AddText({rx+(rw-hsz.x)*.5f, cy+ch*.45f},
                            AF::Alpha(AF::TXT_DIM,0.7f), hint);
            }

            // Test count badge (top-right of table)
            {
                char cntBuf[32];
                snprintf(cntBuf,sizeof(cntBuf),"%d tests",
                         (int)m_generatedTests.size());
                ImVec2 csz=ImGui::CalcTextSize(cntBuf);
                dl->AddText({rx+rw-csz.x-8, cy+5},
                            AF::Alpha(AF::GREEN,0.6f),cntBuf);
            }
        }

    }

    // ── UNIQUENESS PANEL ──────────────────────────────────────────
    void DrawUniquenessPanel(ImDrawList* dl, ImVec2 pos, ImVec2 sz, float dt){
        m_scanUnique.Update(dt, sz.y);
        Panel::Begin(dl, pos, sz, "UNIQUENESS SCORE", AF::GREEN, m_time,
                     true, &m_scanUnique);

        float cx=pos.x+sz.x*.5f, cy=pos.y+sz.y*.5f+6;
        float r=min(sz.x,sz.y)*.32f;

        ImGui::SetCursorScreenPos({pos.x,pos.y});
        ImGui::InvisibleButton("##uni",{sz.x,sz.y});

        m_uniqueness.Draw(dl, {cx,cy}, r, m_time);

        // Sub-label
        const char* verdict = m_uniqueness.score>0.75f ? "HIGHLY ORIGINAL" :
                              m_uniqueness.score>0.5f  ? "MODERATELY UNIQUE" :
                                                         "COMMON PATTERN";
        ImVec2 vsz=ImGui::CalcTextSize(verdict);
        ImU32 vcol = m_uniqueness.score>0.75f ? AF::GREEN :
                     m_uniqueness.score>0.5f  ? AF::AMBER : AF::RED;
        dl->AddText({cx-vsz.x*.5f, pos.y+sz.y-22}, vcol, verdict);
    }

    // ── RADAR PANEL ───────────────────────────────────────────────
    void DrawRadarPanel(ImDrawList* dl, ImVec2 pos, ImVec2 sz, float dt){
        m_scanRadar.Update(dt, sz.y);
        Panel::Begin(dl, pos, sz, "COMPLEXITY RADAR", AF::CYAN, m_time,
                     true, &m_scanRadar);

        float cx=pos.x+sz.x*.5f;
        float cy=pos.y+28+((sz.y-28)*.5f);
        float r =min(sz.x*.38f,(sz.y-60)*.45f);

        m_radar.Draw(dl, {cx,cy}, r, m_time);
    }

    // ── STATS PANEL ───────────────────────────────────────────────
    void DrawStatsPanel(ImDrawList* dl, ImVec2 pos, ImVec2 sz, float dt){
        Panel::Begin(dl, pos, sz, "COMPLEXITY ANALYSIS", AF::AMBER, m_time,
                     false, nullptr);

        float cx=pos.x+10, cy=pos.y+32;
        float rw=sz.x-20;

        // Two complexity tags
        struct Tag{const char* label; const char* value; ImU32 col;};
        Tag tags[]={
            {"TIME",  "O(n log n)", AF::AMBER},
            {"SPACE", "O(n)",       AF::CYAN },
        };
        float tagW=(rw-8)*.5f;
        for(int i=0;i<2;i++){
            float tx=cx+i*(tagW+8);
            float ty=cy;
            float th=min(36.f, sz.y*0.25f);

            dl->AddRectFilled({tx,ty},{tx+tagW,ty+th},
                              AF::Alpha(tags[i].col,0.1f),6.f);
            dl->AddRect({tx,ty},{tx+tagW,ty+th},
                        AF::Alpha(tags[i].col,0.5f),6.f,0,1.5f);

            ImGui::SetWindowFontScale(0.85f);
            dl->AddText(ImGui::GetFont(),ImGui::GetFontSize(),
                        {tx+8,ty+4},tags[i].col,tags[i].label);
            ImGui::SetWindowFontScale(1.3f);
            dl->AddText(ImGui::GetFont(),ImGui::GetFontSize(),
                        {tx+8,ty+th-18},tags[i].col,tags[i].value);
            ImGui::SetWindowFontScale(1.f);
        }

        cy += min(44.f, sz.y*0.3f);

        // Mini stat rows
        struct Stat{const char* key; const char* val; ImU32 col;};
        Stat stats[]={
            {"PARADIGM",  "Divide & Conquer",  AF::TXT_BRIGHT},
            {"RECURSION", "Non-recursive",      AF::GREEN     },
            {"STABLE",    "Yes",                AF::GREEN     },
            {"IN-PLACE",  "No",                 AF::RED       },
        };
        float rowH2=(sz.y-cy+pos.y-8)/(float)(sizeof(stats)/sizeof(stats[0]));
        rowH2=max(14.f, min(rowH2, 22.f));

        for(auto& s:stats){
            if(cy+rowH2 > pos.y+sz.y-4) break;
            dl->AddText({cx,     cy+2}, AF::TXT_DIM,   s.key);
            dl->AddText({cx+100, cy+2}, s.col,          s.val);
            dl->AddLine({cx,cy+rowH2-1},{cx+rw,cy+rowH2-1},
                        AF::Alpha(AF::BORDER,0.4f),0.5f);
            cy+=rowH2;
        }
    }

    // ═══════════════════════════════════════════════════════════════
    //  SHARED WIDGET: FORGE BUTTON
    // ═══════════════════════════════════════════════════════════════
    void DrawForgeButton(ImDrawList* dl, ImVec2 pos, ImVec2 sz,
                         const char* label, ImU32 acol, float time){
        ImVec2 p2={pos.x+sz.x, pos.y+sz.y};
        bool hov = ImGui::IsMouseHoveringRect(pos,p2);
        float pulse=AF::Pulse(time,1.2f);
        float glow=hov?(0.85f+0.15f*pulse):(0.45f+0.1f*pulse);

        // Glow halo
        if(hov)
            dl->AddRectFilled({pos.x-3,pos.y-3},{p2.x+3,p2.y+3},
                              AF::Alpha(acol,0.2f),8.f);

        dl->AddRectFilled(pos,p2, AF::Alpha(acol,0.12f+0.05f*hov),6.f);
        dl->AddRect(pos,p2, AF::Alpha(acol,glow),6.f,0,hov?2.f:1.3f);
        dl->AddLine({pos.x+6,pos.y+1},{p2.x-6,pos.y+1},
                    AF::Alpha(acol,0.4f),1.f);

        // Scanlines on hover
        if(hov) for(float sy=pos.y+4;sy<p2.y;sy+=4.f)
            dl->AddLine({pos.x+2,sy},{p2.x-2,sy},
                        AF::Alpha(acol,0.06f),1.f);

        ImVec2 tsz=ImGui::CalcTextSize(label);
        dl->AddText({pos.x+(sz.x-tsz.x)*.5f, pos.y+(sz.y-tsz.y)*.5f},
                    AF::Alpha(acol, hov?1.f:0.85f), label);

        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton(label, sz);
    }
    // ═══════════════════════════════════════════════════════════════
    //  USER PROFICIENCY PANEL
    //  Shows question type breakdown, strengths and weaknesses
    // ═══════════════════════════════════════════════════════════════
    void DrawProficiencyPanel(ImDrawList* dl, ImVec2 pos, ImVec2 sz, float dt){
        m_scanProf.Update(dt, sz.y);
        Panel::Begin(dl, pos, sz, "SKILL MATRIX  //  USER PROFILE", AF::PURPLE,
                     m_time, true, &m_scanProf);

        float cx = pos.x + 10.f;
        float cy = pos.y + 30.f;
        float pw = sz.x - 20.f;

        // ── Difficulty Breakdown row ──────────────────────────────
        float dbH = 38.f;
        struct DC { const char* label; int solved; int total; ImU32 col; };
        DC dc[] = {
            {"EASY",   m_diffBreak.easySolved,   m_diffBreak.easyTotal,   AF::GREEN },
            {"MEDIUM", m_diffBreak.mediumSolved, m_diffBreak.mediumTotal, AF::AMBER },
            {"HARD",   m_diffBreak.hardSolved,   m_diffBreak.hardTotal,   AF::RED   },
        };
        float segW = (pw - 8.f) / 3.f;
        for(int i = 0; i < 3; i++){
            float sx = cx + i * (segW + 4.f);
            float sy = cy;
            dl->AddRectFilled({sx,sy},{sx+segW,sy+dbH},
                              AF::Alpha(dc[i].col, 0.10f), 5.f);
            dl->AddRect({sx,sy},{sx+segW,sy+dbH},
                        AF::Alpha(dc[i].col, 0.45f), 5.f, 0, 1.2f);

            // Mini arc ring
            float ringCx = sx + 22.f, ringCy = sy + dbH * .5f, ringR = 12.f;
            float pct = (float)dc[i].solved / max(1, dc[i].total);
            float arcEnd = -1.5708f + pct * 6.2832f;
            dl->AddCircle({ringCx, ringCy}, ringR,
                          AF::Alpha(AF::BORDER, 0.7f), 24, 3.f);
            int arcSegs = 20;
            for(int s = 0; s < arcSegs; s++){
                float a0 = -1.5708f + pct * 6.2832f * s / arcSegs;
                float a1 = -1.5708f + pct * 6.2832f * (s+1) / arcSegs;
                dl->AddLine(
                    {ringCx + cosf(a0)*ringR, ringCy + sinf(a0)*ringR},
                    {ringCx + cosf(a1)*ringR, ringCy + sinf(a1)*ringR},
                    dc[i].col, 3.f);
            }
            // Label
            dl->AddText({sx + 38.f, sy + 4.f}, dc[i].col, dc[i].label);
            char ratio[12];
            snprintf(ratio, sizeof(ratio), "%d / %d", dc[i].solved, dc[i].total);
            dl->AddText({sx + 38.f, sy + 18.f}, AF::TXT_MID, ratio);
        }
        cy += dbH + 8.f;

        // ── Divider ───────────────────────────────────────────────
        dl->AddLine({cx, cy},{cx+pw, cy}, AF::Alpha(AF::BORDER, 0.7f), 1.f);
        dl->AddText({cx, cy + 2.f}, AF::TXT_DIM, "TOPIC BREAKDOWN");
        cy += 16.f;

        // ── Tag bars ─────────────────────────────────────────────
        int  visibleTags = (int)m_tags.size();
        float availH = sz.y - (cy - pos.y) - 6.f;
        float rowH   = max(12.f, min(22.f, availH / visibleTags));

        // Split into strong / weak columns for a two-column layout
        // Left col: sorted by accuracy descending (strong) top 6
        // Right col: weakest 6
        vector<int> order(m_tags.size());
        for(int i=0;i<(int)order.size();i++) order[i]=i;
        sort(order.begin(), order.end(), [&](int a, int b){
            return m_tags[a].accuracy > m_tags[b].accuracy;
        });

        float colW2 = (pw - 8.f) * .5f;

        // Header labels
        dl->AddText({cx,           cy}, AF::GREEN, "[+] STRONG");
        dl->AddText({cx + colW2 + 8.f, cy}, AF::RED,   "[-] NEEDS WORK");
        cy += 14.f;

        int half = (int)order.size() / 2;
        for(int i = 0; i < half && i < (int)order.size(); i++){
            float ry = cy + i * rowH;
            if(ry + rowH > pos.y + sz.y - 4.f) break;

            // Per half-column layout constants:
            //   [0 .. codeW)           short code label   (e.g. "ARR")
            //   [codeW+2 .. -pctW-2)   animated bar
            //   [-pctW .. colW2)        accuracy %  (right-aligned, clipped)
            const float codeW = 30.f;
            const float pctW2 = 32.f;
            const float barW2 = colW2 - codeW - pctW2 - 4.f;

            // LEFT — strong
            {
                int   idx  = order[i];
                auto& tg   = m_tags[idx];
                float pulse = 0.9f + 0.1f*sinf(m_time*2.f + idx);

                dl->AddRectFilled({cx,ry+1},{cx+colW2,ry+rowH-2},
                                  AF::Alpha(AF::BG_PANEL2,0.5f),2.f);

                dl->PushClipRect({cx,ry},{cx+codeW,ry+rowH},true);
                dl->AddText({cx+2,ry+2}, tg.col, tg.shortCode.c_str());
                dl->PopClipRect();

                float bx0  = cx + codeW + 2.f;
                float fill = barW2 * tg.accuracy * (0.92f+0.08f*sinf(m_time+idx));
                dl->AddRectFilled({bx0,ry+3},{bx0+barW2,ry+rowH-4},
                                  AF::Alpha(AF::BORDER,0.5f),2.f);
                dl->AddRectFilled({bx0,ry+3},{bx0+fill,ry+rowH-4},
                                  AF::Alpha(tg.col,pulse*0.85f),2.f);

                char pct[8]; snprintf(pct,sizeof(pct),"%.0f%%",tg.accuracy*100);
                float px0 = cx + colW2 - pctW2;
                dl->PushClipRect({px0,ry},{cx+colW2,ry+rowH},true);
                dl->AddText({px0,ry+2}, tg.col, pct);
                dl->PopClipRect();
            }

            // RIGHT — weak
            {
                int   widx = order[order.size()-1-i];
                auto& tg   = m_tags[widx];
                float rx2  = cx + colW2 + 8.f;
                float pulse = 0.9f + 0.1f*sinf(m_time*2.2f + widx);

                dl->AddRectFilled({rx2,ry+1},{rx2+colW2,ry+rowH-2},
                                  AF::Alpha(AF::BG_PANEL2,0.5f),2.f);

                dl->PushClipRect({rx2,ry},{rx2+codeW,ry+rowH},true);
                dl->AddText({rx2+2,ry+2}, tg.col, tg.shortCode.c_str());
                dl->PopClipRect();

                float bx1  = rx2 + codeW + 2.f;
                float fill = barW2 * tg.accuracy * (0.92f+0.08f*sinf(m_time*1.1f+widx));
                dl->AddRectFilled({bx1,ry+3},{bx1+barW2,ry+rowH-4},
                                  AF::Alpha(AF::BORDER,0.5f),2.f);
                dl->AddRectFilled({bx1,ry+3},{bx1+fill,ry+rowH-4},
                                  AF::Alpha(tg.col,pulse*0.85f),2.f);

                char pct[8]; snprintf(pct,sizeof(pct),"%.0f%%",tg.accuracy*100);
                float px1 = rx2 + colW2 - pctW2;
                dl->PushClipRect({px1,ry},{rx2+colW2,ry+rowH},true);
                dl->AddText({px1,ry+2}, tg.col, pct);
                dl->PopClipRect();
            }
        }
    }

};