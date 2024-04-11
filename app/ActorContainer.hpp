#ifndef ACTORCONTAINER_HPP
#define ACTORCONTAINER_HPP
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#   define IMGUI_DEFINE_MATH_OPERATORS
#endif
#define GZB_TOOLTIP_THRESHOLD 1.0f
#define MAX_STR_DEFAULT 256

#include "fontawesome5.h"
#include "config.h"
#include "helpers.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "libsphactor.h"
#include "ImNodes.h"
#include "ImNodesEz.h"
#include "ext/ImGui-Addons/FileBrowser/ImGuiFileBrowser.h"

namespace gzb {
// actor file browser

/// A structure defining a connection between two slots of two actors.
struct Connection
{
    /// `id` that was passed to BeginNode() of input node.
    void* input_node = nullptr;
    /// Descriptor of input slot.
    const char* input_slot = nullptr;
    /// `id` that was passed to BeginNode() of output node.
    void* output_node = nullptr;
    /// Descriptor of output slot.
    const char* output_slot = nullptr;

    bool operator==(const Connection& other) const
    {
        return input_node == other.input_node &&
               input_slot == other.input_slot &&
               output_node == other.output_node &&
               output_slot == other.output_slot;
    }

    bool operator!=(const Connection& other) const
    {
        return !operator ==(other);
    }
};

enum GActorSlotTypes
{
    ActorSlotAny = 1,    // ID can not be 0
    ActorSlotPosition,
    ActorSlotRotation,
    ActorSlotMatrix,
    ActorSlotInt,
    ActorSlotOSC,
    ActorSlotNatNet
};

struct ActorContainer
{
    /// Title which will be displayed at the center-top of the actor.
    const char* title = nullptr;
    /// Flag indicating that actor is selected by the user.
    bool selected = false;
    /// Actor position on the canvas.
    ImVec2 pos{};
    /// List of actor connections.
    std::vector<Connection> connections{};
    /// A list of input slots current actor has.
    std::vector<ImNodes::Ez::SlotInfo> input_slots{};
    /// A list of output slots current actor has.
    std::vector<ImNodes::Ez::SlotInfo> output_slots{};

    /// Last received "lastActive" clock value
    int64_t lastActive = 0;

    sphactor_t *actor;
    zconfig_t *capabilities;

    ActorContainer(sphactor_t *actor);
    ~ActorContainer();
    void ParseConnections();
    ActorContainer *FindActorContainerByEndpoint(const char *endpoint);
    void InitializeCapabilities();
    void SetCapabilities(const char* capabilities );
    void RenderTooltip(const char *help);
    void HandleAPICalls(zconfig_t * data);
    template<typename T>
    void SendAPI(zconfig_t *zapic, zconfig_t *zapiv, zconfig_t *zvalue, T * value);
    void Render(float deltaTime);
    void RenderCustomReport();
    void RenderList(const char *name, zconfig_t *data);
    void RenderMediacontrol(const char* name, zconfig_t *data);
    void RenderFilename(const char* name, zconfig_t *data);
    void RenderBool(const char* name, zconfig_t *data);
    void RenderInt(const char* name, zconfig_t *data);
    void RenderSlider(const char* name, zconfig_t *data);
    void RenderFloat(const char* name, zconfig_t *data);
    void RenderString(const char* name, zconfig_t *data);
    void RenderMultilineString(const char* name, zconfig_t *data);
    void RenderTrigger(const char* name, zconfig_t *data);
    void ReadBool( bool *value, zconfig_t * data);
    void ReadInt( int *value, zconfig_t * data);
    void ReadFloat( float *value, zconfig_t * data);

    void DestroyActor();
    void DeleteConnection(const Connection& connection);
    void SerializeActorData(zconfig_t *section);
    void DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it);

    void OpenTextEditor(const char *filepath);
    void SolvePadding( int* position );
    template<typename T>
    void RenderValue(T *value, const byte * bytes, int *position);
    // Make the UI compact because there are so many fields
    static void PushStyleCompact();
    static void PopStyleCompact();

};

} // namespace
#endif // ACTORCONTAINER_HPP
