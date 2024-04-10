#include "AboutWindow.hpp"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#   define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "config.h"
#include "imgui.h"
#include "libsphactor.h"
#include "czmq.h"
#include <math.h>

namespace gzb {

/* Ref: https://github.com/ocornut/imgui/issues/3606
 * These guys are amazing! Just copied one of the entries
 */
#define V2 ImVec2
#define F float
V2 conv(V2 v, F z, V2 sz, V2 o){return V2((v.x/z)*sz.x*5.f+sz.x*0.5f,(v.y/z)*sz.y*5.f+sz.y*0.5f)+o;}
V2 R(V2 v, F ng){ng*=0.1f;return V2(v.x*cosf(ng)-v.y*sinf(ng),v.x*sinf(ng)+v.y*cosf(ng));}
void FX(ImDrawList* d, V2 o, V2 b, V2 sz, ImVec4, F t) {
    d->AddRectFilled(o,b,0xFF000000,0);
    t*=4;
    for (int i = 0; i < 20; i++) {
        F z=21.-i-(t-floorf(t))*2.,ng=-t*2.1+z,ot0=-t+z*0.2,ot1=-t+(z+1.)*0.2,os=0.3;
        V2 s[]={V2(cosf((t+z)*0.1)*0.2+1.,sinf((t+z)*0.1)*0.2+1.),V2(cosf((t+z+1.)*0.1)*0.2+1.,sinf((t+z+1.)*0.1)*0.2+1.)};
        V2 of[]={V2(cosf(ot0)*os,sinf(ot0)*os),V2(cosf(ot1)*os,sinf(ot1)*os)};
        V2 p[]={V2(-1,-1),V2(1,-1),V2(1,1),V2(-1,1)};
        ImVec2 pts[8];int j;
        for (j=0;j<8;j++) {
            int n = (j/4);pts[j]=conv(R(p[j%4]*s[n]+of[n],ng+n),(z+n)*2.,sz,o);
        }
        for (j=0;j<4;j++){
            V2 q[4]={pts[j],pts[(j+1)%4],pts[((j+1)%4)+4],pts[j+4]};
            F it=(((i&1)?0.5:0.6)+j*0.05)*((21.-z)/21.);
            d->AddConvexPolyFilled(q,4,ImColor::HSV(0.6+sinf(t*0.03)*0.5,1,sqrtf(it)));
        }
    }
}

void SelectableText(const char *buf)
{
    ImGui::PushID(buf);
    ImGui::PushItemWidth(ImGui::GetColumnWidth());
    ImGui::GetStyleColorVec4(ImGuiCol_TableRowBg);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_TableRowBg)); //otherwise it is colored
    ImGui::InputText("", (char *)buf, strlen(buf), ImGuiInputTextFlags_ReadOnly);
    ImGui::PopItemWidth();
    ImGui::PopStyleColor();
    ImGui::PopID();
}

AboutWindow::AboutWindow()
{

}

AboutWindow::~AboutWindow()
{
    if (nics)
        ziflist_destroy(&nics);
}

void AboutWindow::OnImGui()
{
    ImGuiIO& io = ImGui::GetIO();
    //TODO: try to center window, doesn't work somehow
    ImGui::SetNextWindowPos(ImGui::GetWindowSize()/2, ImGuiCond_Once, ImVec2(0.5,0.5));
    ImGui::Begin("About", &showing, ImGuiWindowFlags_AlwaysAutoResize);
    ImVec2 size(320.0f, 180.0f);
    ImGui::InvisibleButton("canvas", size);
    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRect(p0, p1);

    ImVec4 mouse_data;
    mouse_data.x = (io.MousePos.x - p0.x) / size.x;
    mouse_data.y = (io.MousePos.y - p0.y) / size.y;
    mouse_data.z = io.MouseDownDuration[0];
    mouse_data.w = io.MouseDownDuration[1];
    float time = (float)ImGui::GetTime();
    FX(draw_list, p0, p1, size, mouse_data, time);
    draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize()*4.f, p0 + ImVec2(16,120), ImColor::HSV(mouse_data.x,mouse_data.x,0.9), "GazebOsc");
    //draw_list->AddText(p0 + size/2, IM_COL32(255,0,255,255), "GazebOsc");
    //draw_list->AddCircleFilled( p0 + size/2, 10.f, IM_COL32_WHITE, 8);
    draw_list->PopClipRect();
    ImGui::Text("Gazebosc: " GIT_VERSION);
    ImGui::Text("ZeroMQ: %i.%i.%i", ZMQ_VERSION_MAJOR, ZMQ_VERSION_MINOR, ZMQ_VERSION_PATCH);
    ImGui::Text("CZMQ: %i.%i.%i", CZMQ_VERSION_MAJOR, CZMQ_VERSION_MINOR, CZMQ_VERSION_PATCH);
    ImGui::Text("Sphactor: %i.%i.%i", SPHACTOR_VERSION_MAJOR, SPHACTOR_VERSION_MINOR, SPHACTOR_VERSION_PATCH);
    ImGui::Text("Dear ImGui: %s", IMGUI_VERSION);
    if (ImGui::CollapsingHeader("System Info"))
    {
        if (nics == NULL)
            nics = ziflist_new();

        static ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

        if (ImGui::BeginTable("interfaces", 5, flags))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("MacAddress");
            ImGui::TableSetupColumn("IpAddress");
            ImGui::TableSetupColumn("Netmask");
            ImGui::TableSetupColumn("Broadcast");
            ImGui::TableHeadersRow();

            const char *name = ziflist_first (nics);
            while (name)
            {
                ImGui::TableNextRow();
                // name
                ImGui::TableSetColumnIndex(0);
                SelectableText(name);

                // mac
                char buf[32];
                ImGui::TableSetColumnIndex(1);
                sprintf(buf, "%s", ziflist_mac(nics) );
                SelectableText(buf);
                // address
                ImGui::TableSetColumnIndex(2);
                sprintf(buf, "%s", ziflist_address (nics));
                SelectableText(buf);
                // netmask
                ImGui::TableSetColumnIndex(3);
                sprintf(buf, "%s", ziflist_netmask (nics));
                SelectableText(buf);
                // broadcast
                ImGui::TableSetColumnIndex(4);
                //char buf[32];
                sprintf(buf, "%s", ziflist_broadcast (nics));
                SelectableText(buf);

                name = ziflist_next (nics);
            }
            ImGui::EndTable();
        }
        if ( ImGui::Button("refresh interfaces") )
        {
            ziflist_reload(nics);
        }

    }
    ImGui::End();
}

} // namespace
