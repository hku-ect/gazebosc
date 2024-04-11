#ifndef STAGEWINDOW_HPP
#define STAGEWINDOW_HPP
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#   define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "ActorContainer.hpp"
#include <vector>
#include <map>
#include <string>
#include <stack>
#include <filesystem>
#include "Window.hpp"
#include "libsphactor.h"

namespace gzb {

//enum for menu actions
enum MenuAction
{
    MenuAction_Save = 0,
    MenuAction_Load,
    MenuAction_Clear,
    MenuAction_Exit,
    MenuAction_SaveAs,
    MenuAction_None
};

//enum for undo stack
enum UndoAction
{
    UndoAction_Create = 0,
    UndoAction_Delete,
    UndoAction_Disconnect,
    UndoAction_Connect,
    UndoAction_Invalid
};

struct UndoData {
    UndoAction type = UndoAction_Invalid;
    const char * title = nullptr;
    const char * uuid = nullptr;
    const char * endpoint = nullptr;
    const char * input_slot = nullptr;
    const char * output_slot = nullptr;
    zconfig_t * sphactor_config = nullptr;
    ImVec2 position;

    UndoData( UndoData * from ) {
        type = from->type;
        title = from->title ? strdup(from->title) : nullptr;
        uuid = from->uuid ? strdup(from->uuid) : nullptr;
        endpoint = from->endpoint ? strdup(from->endpoint) : nullptr;
        input_slot = from->input_slot ? strdup(from->input_slot) : nullptr;
        output_slot = from->output_slot ? strdup(from->output_slot) : nullptr;
        if (from->sphactor_config != nullptr)
            sphactor_config = zconfig_dup(from->sphactor_config);
        position = from->position;
    }
    UndoData() {}
};

class StageWindow : public Window
{
public:
    sph_stage_t *stage = NULL;
    std::string editing_file = "";
    std::string editing_path = "";
    ImVector<char*> actor_types;
    std::vector<ActorContainer*> actors;
    std::map<std::string, int> max_actors_by_type;
    std::stack<UndoData> undoStack;
    std::stack<UndoData> redoStack;

    StageWindow();
    ~StageWindow();
    void Init();
    void Clear();
    bool Load( const char* configFile );
    bool Save( const char* configFile );
    int RenderMenuBar();
    int UpdateActors(float deltaTime);
    ActorContainer *Find(const char *endpoint);
    ActorContainer * CreateFromType( const char* typeStr, const char* uuidStr );
    int CountActorsOfType( const char* type );

    // undo stuff
    void performUndo(UndoData &undo);
    void RegisterCreateAction( ActorContainer * actor );
    void RegisterDeleteAction( ActorContainer * actor );
    void RegisterConnectAction(ActorContainer * in, ActorContainer * out, const char * input_slot, const char * output_slot);
    void RegisterDisconnectAction(ActorContainer * in, ActorContainer * out, const char * input_slot, const char * output_slot);
    void swapUndoType(UndoData * undo);

    void moveCwdIfNeeded();
};
}
#endif // STAGEWINDOW_HPP
