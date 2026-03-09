#pragma once
// ================================================================
//  HackerLoginSystem.h  v2
//  Glassmorphism + Binary/Hacker aesthetic login page
//  Dear ImGui (OpenGL3 / GLFW)
//  v2 – bigger fonts, bigger panel, bigger boot text
// ================================================================

#include "imgui.h"
#include "imgui_internal.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;

// ── size constants (tweak here) ───────────────────────────────
static constexpr float HL_PANEL_W      = 640.0f;  // login card width
static constexpr float HL_PANEL_H      = 700.0f;  // login card height
static constexpr float HL_BOOT_SCALE   = 2.0f;    // font scale for boot lines
static constexpr float HL_BOOT_LINE_H  = 38.0f;   // px between boot lines
static constexpr float HL_BOOT_X       = 70.0f;   // boot text left margin
static constexpr float HL_BOOT_Y       = 70.0f;   // boot text top margin
static constexpr float HL_LABEL_SCALE  = 1.7f;    // label font scale ("> USER_ID:")
static constexpr float HL_LABEL_H      = 30.0f;   // height consumed by label
static constexpr float HL_INPUT_H      = 48.0f;   // input field height
static constexpr float HL_INPUT_SCALE  = 1.5f;    // font scale inside input fields
static constexpr float HL_BTN_H_LOGIN  = 56.0f;   // primary button height
static constexpr float HL_BTN_H_REG    = 46.0f;   // secondary button height
static constexpr float HL_BTN_SCALE    = 1.6f;    // button label font scale
static constexpr float HL_CB_SCALE     = 1.4f;    // checkbox + small button scale
static constexpr float HL_STATUS_SCALE = 1.1f;    // bottom status bar scale
static constexpr float HL_CONTENT_PAD  = 50.0f;   // panel inner side padding
// ─────────────────────────────────────────────────────────────

static inline float  LerpF   (float a,float b,float t){return a+(b-a)*t;}
static inline float  SinPulse(float t,float freq=1.f){return(sinf(t*freq*6.2832f)+1.f)*.5f;}
static inline ImU32  MulAlpha(ImU32 col,float a){
    ImVec4 c=ImGui::ColorConvertU32ToFloat4(col); c.w*=a;
    return ImGui::ColorConvertFloat4ToU32(c);
}
static inline float ELO(float t){float u=1-t;return 1-u*u*u;}

struct RainColumn {
    float x,y,speed; int length; float spacing;
    vector<char> chars; float changeTimer,changePeriod;
};
struct LoginParticle { ImVec2 pos,vel; float life,maxLife; ImU32 col; };
struct BootLine      { string full,shown; float revealTimer; bool done; };

// ================================================================
class HackerLoginSystem {
public:
    HackerLoginSystem(){ srand((unsigned)time(nullptr)); }

    void Init(ImVec2 screenSize){
        m_screen=screenSize; m_time=0;
        m_panelAlpha=0; m_bootDone=false; m_bootTimer=0; m_bootLineIdx=0;
        m_loginClicked=m_registerClicked=false; m_loginBurstTimer=0;
        m_rememberMe=m_showPass=false;
        memset(m_userBuf,0,sizeof(m_userBuf));
        memset(m_passBuf,0,sizeof(m_passBuf));
        m_particles.clear(); m_columns.clear(); m_bootLines.clear();
        m_cornerAngle=m_scanlineY=m_borderPhase=m_inputGlow=0; m_fieldFocus=-1;
        InitRain(); InitBootLines(); m_initialised=true;
    }

    bool Render(float dt, bool& loginOut, bool& registerOut){
        if(!m_initialised) Init(ImGui::GetIO().DisplaySize);
        loginOut=registerOut=false;
        m_time+=dt;
        if(m_bootDone) m_panelAlpha=min(1.f,m_panelAlpha+dt*0.85f);
        UpdateRain(dt); UpdateBoot(dt); UpdateParticles(dt);
        m_cornerAngle+=dt*0.6f;
        m_scanlineY=fmodf(m_scanlineY+dt*m_screen.y*0.18f,m_screen.y);
        m_borderPhase+=dt;
        m_inputGlow=SinPulse(m_time,0.7f);

        DrawBackground(); DrawMatrixRain(); DrawScanlines();
        DrawCornerDecorations(); DrawParticles();

        ImGui::SetNextWindowPos({0,0});
        ImGui::SetNextWindowSize(m_screen);
        ImGui::SetNextWindowBgAlpha(0.f);
        ImGuiWindowFlags loginFlags = ImGuiWindowFlags_NoDecoration |
                                      ImGuiWindowFlags_NoBringToFrontOnFocus;
        if (m_panelAlpha < 0.05f)
            loginFlags |= ImGuiWindowFlags_NoInputs;
        ImGui::Begin("##LoginHost", nullptr, loginFlags);

        DrawBootText();
        if(m_panelAlpha>0.01f) DrawLoginPanel(dt,loginOut,registerOut);
        ImGui::End();
        if(loginOut) SpawnLoginBurst();
        return true;
    }

    bool        LoginClicked()    const{ return m_loginClicked; }
    bool        RegisterClicked() const{ return m_registerClicked; }
    const char* GetUsername()     const{ return m_userBuf; }
    const char* GetPassword()     const{ return m_passBuf; }
    void Reset(){ m_initialised=false; }

private:
    ImVec2 m_screen{1920,1080};
    float  m_time=0,m_phaseT=0;
    bool   m_initialised=false;
    vector<RainColumn>    m_columns;
    vector<BootLine>      m_bootLines;
    int    m_bootLineIdx=0; float m_bootTimer=0; bool m_bootDone=false;
    float  m_panelAlpha=0,m_cornerAngle=0,m_scanlineY=0;
    float  m_borderPhase=0,m_inputGlow=0;
    int    m_fieldFocus=-1;
    char   m_userBuf[128]={},m_passBuf[128]={};
    bool   m_rememberMe=false,m_showPass=false;
    bool   m_loginClicked=false,m_registerClicked=false;
    float  m_loginBurstTimer=0;
    vector<LoginParticle> m_particles;

    static float Rnd01() { return (float)rand()/(float)RAND_MAX; }
    static float RndSym(){ return Rnd01()*2.f-1.f; }
    static char  RndDigit(){ return (rand()&1)?'1':'0'; }

    // ─── INIT ────────────────────────────────────────────────
    void InitRain(){
        int cols=(int)(m_screen.x/20.f)+2;
        m_columns.resize(cols);
        for(int i=0;i<cols;i++){
            auto& c=m_columns[i];
            c.x=(float)i*20.f; c.y=-Rnd01()*m_screen.y;
            c.speed=55.f+Rnd01()*110.f;
            c.length=8+rand()%22; c.spacing=18.f;
            c.changePeriod=0.05f+Rnd01()*0.15f; c.changeTimer=0;
            c.chars.resize(c.length);
            for(auto& ch:c.chars) ch=RndDigit();
        }
    }

    void InitBootLines(){
        m_bootLines={
            {">> SYSTEM BOOT v4.2.0 ...",                       "",0.035f,false},
            {">> Initializing quantum kernel         [OK]",     "",0.035f,false},
            {">> Loading neural cipher matrix ...   [OK]",      "",0.035f,false},
            {">> Decrypting AUTH_MODULE_v9 ...       [OK]",     "",0.035f,false},
            {">> Establishing AES-256 secure tunnel  [OK]",     "",0.035f,false},
            {">> Identity verification required.",              "",0.045f,false},
            {">> Enter credentials to continue _",              "",0.045f,false},
        };
    }

    // ─── UPDATE ──────────────────────────────────────────────
    void UpdateRain(float dt){
        for(auto& c:m_columns){
            c.y+=c.speed*dt;
            if(c.y>m_screen.y+c.length*c.spacing){
                c.y=-c.length*c.spacing*Rnd01()*2.f;
                c.speed=55.f+Rnd01()*110.f;
            }
            c.changeTimer-=dt;
            if(c.changeTimer<=0){
                c.changeTimer=c.changePeriod;
                c.chars[rand()%c.length]=RndDigit();
            }
        }
    }

    void UpdateBoot(float dt){
        if(m_bootDone) return;
        if(m_bootLineIdx>=(int)m_bootLines.size()){m_bootDone=true;return;}
        auto& line=m_bootLines[m_bootLineIdx];
        m_bootTimer-=dt;
        if(m_bootTimer<=0){
            m_bootTimer=line.revealTimer;
            if(line.shown.size()<line.full.size()){
                line.shown+=line.full[line.shown.size()];
            } else {
                line.done=true; m_bootTimer=0.22f; m_bootLineIdx++;
            }
        }
    }

    void UpdateParticles(float dt){
        for(auto& p:m_particles){
            p.pos.x+=p.vel.x*dt; p.pos.y+=p.vel.y*dt;
            p.vel.y+=60.f*dt; p.life-=dt;
        }
        m_particles.erase(
            remove_if(m_particles.begin(),m_particles.end(),
                [](const LoginParticle& p){return p.life<=0;}),
            m_particles.end());
    }

    // ─── DRAW LAYERS ─────────────────────────────────────────
    void DrawBackground(){
        ImDrawList* dl=ImGui::GetBackgroundDrawList();
        ImVec2 s=m_screen;
        dl->AddRectFilledMultiColor({0,0},s,
            IM_COL32(2,5,15,255),IM_COL32(2,5,15,255),
            IM_COL32(0,2,8,255), IM_COL32(0,2,8,255));
        dl->AddCircleFilled({s.x*.5f,s.y*.5f},s.x*.65f,IM_COL32(0,20,50,28),64);
    }

    void DrawMatrixRain(){
        ImDrawList* dl=ImGui::GetBackgroundDrawList();
        for(auto& c:m_columns){
            for(int i=0;i<c.length;i++){
                float fy=c.y-i*c.spacing;
                if(fy<-20||fy>m_screen.y+20) continue;
                float bright=(i==0)?1.f:max(0.f,1.f-(float)i/c.length);
                float fadeB=min(1.f,(m_screen.y-fy)/80.f);
                float fadeT=min(1.f,(fy+20.f)/60.f);
                float a=bright*fadeB*fadeT*0.55f;
                if(a<0.02f) continue;
                ImU32 col=(i==0)?IM_COL32(180,255,240,(int)(a*255)):
                                  IM_COL32(0,(int)(180*bright),(int)(160*bright),(int)(a*255));
                char buf[2]={c.chars[i],0};
                dl->AddText({c.x,fy},col,buf);
            }
        }
    }

    void DrawScanlines(){
        ImDrawList* dl=ImGui::GetBackgroundDrawList();
        float w=m_screen.x;
        for(int i=-1;i<=1;i++){
            float y=fmodf(m_scanlineY+i*8.f,m_screen.y);
            dl->AddRectFilled({0,y-1.5f},{w,y+1.5f},IM_COL32(0,200,190,18));
        }
        for(float y=0;y<m_screen.y;y+=4.f)
            dl->AddLine({0,y},{w,y},IM_COL32(0,80,70,8));
    }

    void DrawCornerDecorations(){
        ImDrawList* dl=ImGui::GetBackgroundDrawList();
        float sw=m_screen.x,sh=m_screen.y,t=m_time;
        struct Corn{float ox,oy;};
        Corn corners[]={{0,0},{sw,0},{0,sh},{sw,sh}};
        for(int ci=0;ci<4;ci++){
            float ox=corners[ci].ox,oy=corners[ci].oy;
            float sx=(ci%2==0)?1.f:-1.f, sy=(ci/2==0)?1.f:-1.f;
            float ringR=110.f;
            for(int n=0;n<6;n++){
                float angle=m_cornerAngle+n*6.2832f/6;
                float nx=ox+sx*(45.f+ringR*cosf(angle));
                float ny=oy+sy*(45.f+ringR*sinf(angle));
                float pulse=SinPulse(t+n*0.3f,1.2f);
                ImU32 col=IM_COL32(0,(int)(150+60*pulse),(int)(140+60*pulse),(int)(80+60*pulse));
                dl->AddCircle({nx,ny},4.f,col,6,1.8f);
                dl->AddLine({ox+sx*45.f,oy+sy*45.f},{nx,ny},IM_COL32(0,100,90,55),1.f);
            }
            float bl=70.f;
            ImU32 bCol=IM_COL32(0,220,210,130);
            dl->AddLine({ox,oy+sy*bl},{ox,oy},bCol,2.5f);
            dl->AddLine({ox,oy},{ox+sx*bl,oy},bCol,2.5f);
            char addr[32];
            snprintf(addr,sizeof(addr),"0x%04X:%02X",
                (int)(m_time*13.7f+ci*0x1F3)&0xFFFF,
                (int)(m_time*7.3f+ci*0x2A)&0xFF);
            float tx=ox+sx*(sx>0?8.f:-100.f);
            float ty=oy+sy*(sy>0?bl+10.f:-bl-22.f);
            dl->AddText({tx,ty},IM_COL32(0,160,150,140),addr);
        }
    }

    void DrawParticles(){
        ImDrawList* dl=ImGui::GetBackgroundDrawList();
        for(auto& p:m_particles){
            float a=p.life/p.maxLife;
            dl->AddCircleFilled(p.pos,4.f*a,MulAlpha(p.col,a),8);
        }
    }

    // ─── BOOT TEXT ───────────────────────────────────────────
    // Uses ImGui::SetWindowFontScale for big readable terminal text
    void DrawBootText(){
        ImDrawList* dl=ImGui::GetWindowDrawList();

        // Push larger font scale for the terminal block
        ImGui::SetWindowFontScale(HL_BOOT_SCALE);

        for(int i=0;i<(int)m_bootLines.size()&&i<=m_bootLineIdx;i++){
            auto& line=m_bootLines[i];
            if(line.shown.empty()) continue;

            bool isOK     =line.shown.find("[OK]")!=string::npos;
            bool isCurrent=(i==m_bootLineIdx&&!line.done);

            ImU32 col=isOK      ?IM_COL32(60,255,110,220):
                      isCurrent ?IM_COL32(0,255,235,240):
                                  IM_COL32(0,185,175,180);

            // Extra bright highlight on [OK] tag
            string display=line.shown;
            if(isCurrent&&(int)(m_time*4)%2==0) display+="|";

            float y=HL_BOOT_Y+(float)i*HL_BOOT_LINE_H;
            dl->AddText(ImGui::GetFont(),
                        ImGui::GetFontSize(), // already scaled
                        {HL_BOOT_X,y}, col, display.c_str());
        }

        // Restore default scale
        ImGui::SetWindowFontScale(1.0f);
    }

    // ─── LOGIN PANEL ─────────────────────────────────────────
    void DrawLoginPanel(float dt,bool& loginOut,bool& registerOut){
        float a=m_panelAlpha;
        if(a<0.01f) return;

        const float PW=HL_PANEL_W, PH=HL_PANEL_H;
        float px=(m_screen.x-PW)*.5f;
        float py=(m_screen.y-PH)*.5f;

        // Slide in from above
        py+=LerpF(-90.f,0.f,ELO(min(1.f,a)));

        ImDrawList* fg=ImGui::GetWindowDrawList();

        // ── Glass card ───────────────────────────────────────
        fg->AddRectFilled({px,py},{px+PW,py+PH},
                          MulAlpha(IM_COL32(5,20,40,210),a),20.f);
        fg->AddRectFilled({px,py},{px+PW,py+PH*0.055f},
                          MulAlpha(IM_COL32(0,160,150,45),a),20.f);

        // ── Pulsing border ───────────────────────────────────
        float bp=SinPulse(m_borderPhase,.5f);
        ImU32 bCol1=IM_COL32(0,(int)(185+65*bp),(int)(175+65*bp),(int)(a*(165+80*bp)));
        ImU32 bCol2=IM_COL32((int)(15*bp),(int)(100+55*bp),(int)(85+55*bp),(int)(a*(85+60*bp)));
        fg->AddRect({px-4,py-4},{px+PW+4,py+PH+4},bCol2,22.f,0,5.f);
        fg->AddRect({px,py},{px+PW,py+PH},bCol1,20.f,0,2.f);

        // Top accent line
        fg->AddLine({px+20,py+1.5f},{px+PW-20,py+1.5f},
                    IM_COL32(0,245,235,(int)(a*(0.7f+0.3f*bp)*255)),2.5f);

        // Circuit traces
        DrawPanelCircuit(fg,px,py,PW,PH,a);

        // ── Content ──────────────────────────────────────────
        float cX=px+HL_CONTENT_PAD;
        float cY=py+HL_CONTENT_PAD;
        float fieldW=PW-HL_CONTENT_PAD*2.f;

        // USER_ID label
        DrawFieldLabel(fg,cX,cY,"> USER_ID:",a);
        cY+=HL_LABEL_H+4.f;

        ImGui::SetCursorScreenPos({cX,cY});
        DrawStyledInput(fg,"##uid",m_userBuf,sizeof(m_userBuf),fieldW,false,a,0,cX,cY);
        cY+=HL_INPUT_H+18.f;

        // PASS_KEY label
        DrawFieldLabel(fg,cX,cY,"> PASS_KEY:",a);
        cY+=HL_LABEL_H+4.f;

        ImGui::SetCursorScreenPos({cX,cY});
        DrawStyledInput(fg,"##pwd",m_passBuf,sizeof(m_passBuf),fieldW,!m_showPass,a,1,cX,cY);
        cY+=HL_INPUT_H+14.f;

        // Show/hide password
        ImGui::SetCursorScreenPos({cX,cY});
        ImGui::SetWindowFontScale(HL_CB_SCALE);
        ImGui::PushStyleColor(ImGuiCol_Text,MulAlpha(IM_COL32(0,185,175,255),a));
        ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0,40,50,80));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(0,60,70,120));
        if(ImGui::SmallButton(m_showPass?"[HIDE PASS]":"[SHOW PASS]"))
            m_showPass=!m_showPass;
        ImGui::PopStyleColor(4);
        ImGui::SetWindowFontScale(1.f);
        cY+=36.f;

        // Remember Me
        ImGui::SetCursorScreenPos({cX,cY});
        ImGui::SetWindowFontScale(HL_CB_SCALE);
        ImGui::PushStyleColor(ImGuiCol_CheckMark,     ImVec4(0,1,.93f,a));
        ImGui::PushStyleColor(ImGuiCol_FrameBg,       ImVec4(0,.1f,.15f,a*.8f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImVec4(0,.2f,.25f,a*.9f));
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0,.82f,.78f,a));
        ImGui::Checkbox("REMEMBER SESSION",&m_rememberMe);
        ImGui::PopStyleColor(4);
        ImGui::SetWindowFontScale(1.f);
        cY+=48.f;

        // LOGIN button
        ImGui::SetCursorScreenPos({cX,cY});
        DrawGlowButton(fg,"##loginbtn","[ ACCESS SYSTEM ]",
                       fieldW,HL_BTN_H_LOGIN,
                       IM_COL32(0,215,205,(int)(a*225)),
                       IM_COL32(0,55,65,(int)(a*220)),
                       IM_COL32(0,255,240,(int)(a*255)),
                       a,loginOut);
        cY+=HL_BTN_H_LOGIN+14.f;

        // REGISTER button
        ImGui::SetCursorScreenPos({cX,cY});
        DrawGlowButton(fg,"##regbtn","[ REGISTER NEW USER ]",
                       fieldW,HL_BTN_H_REG,
                       IM_COL32(20,95,85,(int)(a*185)),
                       IM_COL32(0,22,32,(int)(a*185)),
                       IM_COL32(40,205,185,(int)(a*225)),
                       a,registerOut);

        // Status bar
        float sY=py+PH-34.f;
        fg->AddLine({px+20,sY-5.f},{px+PW-20,sY-5.f},
                    MulAlpha(IM_COL32(0,120,110,200),a*.45f),1.2f);
        char status[72];
        snprintf(status,sizeof(status),"CONN: 127.0.0.1  |  ENC: AES-256  |  TOKEN:%04X",
                 (int)(m_time*0xF3)&0xFFFF);
        ImGui::SetWindowFontScale(HL_STATUS_SCALE);
        fg->AddText(ImGui::GetFont(),ImGui::GetFontSize(),
                    {cX,sY},MulAlpha(IM_COL32(0,135,125,225),a),status);
        ImGui::SetWindowFontScale(1.f);
    }

    // ── Circuit traces inside panel ──────────────────────────
    void DrawPanelCircuit(ImDrawList* dl,float px,float py,float pw,float ph,float a){
        float t=m_time;
        struct Trace{float x1,y1,x2,y2,x3,y3,speed;};
        Trace tr[]={
            {.05f,.07f,.17f,.07f,.17f,.13f,.7f},
            {.83f,.07f,.94f,.07f,.94f,.13f,.9f},
            {.05f,.89f,.17f,.89f,.17f,.95f,.5f},
            {.83f,.89f,.94f,.89f,.94f,.95f,.6f},
        };
        for(auto& r:tr){
            float anim=SinPulse(t,r.speed);
            ImU32 col=IM_COL32(0,(int)(105+80*anim),(int)(95+80*anim),(int)(a*(65+80*anim)));
            ImVec2 p1={px+r.x1*pw,py+r.y1*ph};
            ImVec2 p2={px+r.x2*pw,py+r.y2*ph};
            ImVec2 p3={px+r.x3*pw,py+r.y3*ph};
            dl->AddLine(p1,p2,col,1.5f); dl->AddLine(p2,p3,col,1.5f);
            dl->AddCircleFilled(p2,3.f,col,6);
        }
    }

    // ── Field label (scaled) ─────────────────────────────────
    void DrawFieldLabel(ImDrawList* dl,float x,float y,const char* label,float a){
        float pulse=.85f+.15f*sinf(m_time*5.f);
        ImGui::SetWindowFontScale(HL_LABEL_SCALE);
        dl->AddText(ImGui::GetFont(),ImGui::GetFontSize(),{x,y},
                    IM_COL32(0,(int)(205*pulse),(int)(195*pulse),(int)(a*215)),
                    label);
        ImGui::SetWindowFontScale(1.f);
    }

    // ── Styled input field ───────────────────────────────────
    void DrawStyledInput(ImDrawList* dl,const char* id,char* buf,int sz,
                         float w,bool pw,float a,int idx,float x,float y){
        bool   active=(m_fieldFocus==idx);
        float  glow=active?(0.72f+0.28f*m_inputGlow):0.3f;

        ImVec2 p0={x,y}, p1={x+w,y+HL_INPUT_H};
        dl->AddRectFilled(p0,p1,MulAlpha(IM_COL32(0,28,48,225),a),8.f);
        dl->AddRect(p0,p1,
            IM_COL32(0,(int)(185*glow),(int)(175*glow),(int)(a*205)),
            8.f,0,active?2.f:1.3f);
        dl->AddLine({x+8,y+1.5f},{x+w-8,y+1.5f},
                    MulAlpha(IM_COL32(0,205,195,65),a*glow),1.2f);

        // Active glow bar on left edge
        if(active)
            dl->AddRectFilled({x,y+4},{x+3,y+HL_INPUT_H-4},
                              MulAlpha(IM_COL32(0,235,225,255),a),2.f);

        ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0,.1f,.15f,.3f*a));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ImVec4(0,.12f,.2f,.4f*a));
        ImGui::PushStyleColor(ImGuiCol_Text,           ImVec4(.7f,1.f,.97f,a));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0,.6f,.55f,.5f*a));
        // Padding scales with font to keep text vertically centred
        float vpad=(HL_INPUT_H-(ImGui::GetFontSize()*HL_INPUT_SCALE))*.5f;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(12,max(4.f,vpad)));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,8.f);
        ImGui::SetWindowFontScale(HL_INPUT_SCALE);
        ImGui::SetNextItemWidth(w);
        ImGuiInputTextFlags flags=pw?ImGuiInputTextFlags_Password:0;
        ImGui::InputText(id,buf,sz,flags);
        if(ImGui::IsItemActive()) m_fieldFocus=idx;
        ImGui::SetWindowFontScale(1.f);
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(5);
    }

    // ── Glowing button (label scaled) ────────────────────────
    void DrawGlowButton(ImDrawList* dl,const char* id,const char* label,
                        float w,float h,ImU32 borderCol,ImU32 bgCol,
                        ImU32 textCol,float a,bool& clicked){
        ImVec2 p1=ImGui::GetCursorScreenPos();
        ImVec2 p2={p1.x+w,p1.y+h};
        bool hov=ImGui::IsMouseHoveringRect(p1,p2);
        float pulse=SinPulse(m_time,1.f);

        if(hov) dl->AddRectFilled({p1.x-5,p1.y-5},{p2.x+5,p2.y+5},
                                   MulAlpha(borderCol,.28f*a),14.f);

        ImU32 fill=hov?IM_COL32(0,(int)(42+28*pulse),(int)(52+18*pulse),(int)(a*215)):
                       MulAlpha(bgCol,a);
        dl->AddRectFilled(p1,p2,fill,10.f);

        float bg=hov?(0.82f+0.18f*pulse):(0.52f+0.18f*pulse);
        dl->AddRect(p1,p2,MulAlpha(borderCol,a*bg),10.f,0,hov?2.5f:1.8f);
        dl->AddLine({p1.x+10,p1.y+1.8f},{p2.x-10,p1.y+1.8f},
                    MulAlpha(borderCol,a*.45f),1.2f);

        if(hov) for(float sy=p1.y+5;sy<p2.y;sy+=5.f)
            dl->AddLine({p1.x+3,sy},{p2.x-3,sy},
                        MulAlpha(IM_COL32(0,205,195,32),a),1.f);

        // Scaled label centred in button
        ImGui::SetWindowFontScale(HL_BTN_SCALE);
        ImVec2 tsz=ImGui::CalcTextSize(label);
        ImGui::SetWindowFontScale(1.f); // reset before AddText
        ImGui::SetWindowFontScale(HL_BTN_SCALE);
        ImVec2 tp={p1.x+(w-tsz.x)*.5f, p1.y+(h-tsz.y)*.5f};
        dl->AddText(ImGui::GetFont(),ImGui::GetFontSize(),tp,
                    MulAlpha(textCol,a),label);
        ImGui::SetWindowFontScale(1.f);

        ImGui::SetCursorScreenPos(p1);
        ImGui::InvisibleButton(id,{w,h});
        if(ImGui::IsItemClicked()) clicked=true;
    }

    // ── Burst particles ──────────────────────────────────────
    void SpawnLoginBurst(){
        float cx=m_screen.x*.5f,cy=m_screen.y*.5f;
        for(int i=0;i<140;i++){
            LoginParticle p;
            p.pos={cx,cy};
            float ang=(float)i/140.f*6.2832f;
            float spd=110.f+Rnd01()*310.f;
            p.vel={cosf(ang)*spd,sinf(ang)*spd};
            p.maxLife=p.life=.55f+Rnd01()*.85f;
            p.col=(i%3==0)?IM_COL32(0,255,200,255):
                  (i%3==1)?IM_COL32(0,200,255,255):
                            IM_COL32(100,255,180,255);
            m_particles.push_back(p);
        }
    }
};