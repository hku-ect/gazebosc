#ifndef ACTORCONTAINER_H
#define ACTORCONTAINER_H

#include "libsphactor.h"
#include <string>
#include <vector>
#include "ImNodes.h"
#include "ImNodesEz.h"
#include <czmq.h>

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
    ActorSlotOSC
};

struct ActorContainer {
    /// Default value UI strings will have if no maximum is set
    const int MAX_STR_DEFAULT = 256;
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

    sphactor_t *actor;
    zconfig_t *capabilities;

    ActorContainer(sphactor_t *actor) {
        this->actor = actor;
        this->title = sphactor_ask_actor_type(actor);
        this->capabilities = zconfig_dup(sphactor_ask_capability(actor));

        ParseConnections();
        //TODO: figure out why this doesn't work
        InitializeCapabilities();
    }

    ~ActorContainer() {
        zconfig_destroy(&this->capabilities);
    }

    void ParseConnections() {
        if ( this->capabilities == NULL ) return;

        zconfig_t *inputs = zconfig_locate(this->capabilities, "inputs");
        zconfig_t *outputs = zconfig_locate(this->capabilities, "outputs");

        Parse(inputs, "input", &input_slots);
        Parse(outputs, "output", &output_slots);
    }

    void InitializeCapabilities() {
        if ( this->capabilities == NULL ) return;

        zconfig_t *root = zconfig_locate(this->capabilities, "capabilities");
        if ( root == NULL ) return;

        zconfig_t *data = zconfig_locate(root, "data");
        while( data != NULL ) {
            HandleAPICalls(data);
            data = zconfig_next(data);
        }
    }

    void HandleAPICalls(zconfig_t * data) {
        zconfig_t *zapic = zconfig_locate(data, "api_call");
        if ( zapic ) {
            zconfig_t *zapiv = zconfig_locate(data, "api_value");
            zconfig_t *zvalue = zconfig_locate(data, "value");

            if (zapiv)
            {
                char * zapivStr = zconfig_value(zapiv);
                char type = zapivStr[0];
                switch( type ) {
                    case 'i':
                        int ival;
                        ReadInt(&ival, zvalue);
                        SendAPI<int>(zapic, zapiv, zvalue, &ival);
                    break;
                    case 'f':
                        float fval;
                        ReadFloat(&fval, zvalue);
                        SendAPI<float>(zapic, zapiv, zvalue, &fval);
                    break;
                    case 's':
                        char* buf = new char[MAX_STR_DEFAULT];
                        const char* zvalueStr = zconfig_value(zvalue);
                        strcpy(buf, zvalueStr);
                        SendAPI<char*>(zapic, zapiv, zvalue, &buf);
                        zstr_free(&buf);
                    break;
                }
            }
            else {
                //assume int
                int ival;
                ReadInt(&ival, zvalue);
                zsock_send( sphactor_socket(this->actor), "si", zconfig_value(zapic), ival);
            }
        }
    }

    template<typename T>
    void SendAPI(zconfig_t *zapic, zconfig_t *zapiv, zconfig_t *zvalue, T * value) {
        if ( !zapic ) return;

        if (zapiv)
        {
            std::string pic = "s";
            pic += zconfig_value(zapiv);
            //zsys_info("Sending: %s", pic.c_str());
            zsock_send( sphactor_socket(this->actor), pic.c_str(), zconfig_value(zapic), *value);
        }
        else
            zsock_send( sphactor_socket(this->actor), "si", zconfig_value(zapic), *value);
    }

    void Parse(zconfig_t * config, const char* node, std::vector<ImNodes::Ez::SlotInfo> *list ) {
        if ( config == NULL ) return;

        zconfig_t *localNode = zconfig_locate( config, node);
        while ( localNode != NULL ) {
            zconfig_t *type = zconfig_locate(localNode, "type");
            assert(type);

            char* typeStr = zconfig_value(type);
            if ( streq(typeStr, "OSC")) {
                list->push_back({ "OSC", ActorSlotOSC });
            }
            else {
                zsys_error("Unsupported %s: %s", node, typeStr);
            }

            localNode = zconfig_next(localNode);
        }
    }

    void Render(float deltaTime) {
        RenderCustomReport();

        //loop through each data element in capabilities
        if ( this->capabilities == NULL ) return;

        zconfig_t *root = zconfig_locate(this->capabilities, "capabilities");
        if ( root == NULL ) return;

        zconfig_t *data = zconfig_locate(root, "data");
        while( data != NULL ) {
            zconfig_t *name = zconfig_locate(data, "name");
            zconfig_t *type = zconfig_locate(data, "type");
            assert(name);
            assert(type);

            char* nameStr = zconfig_value(name);
            char* typeStr = zconfig_value(type);
            if ( streq(typeStr, "int")) {
                RenderInt( nameStr, data );
            }
            else if ( streq(typeStr, "float")) {
                RenderFloat( nameStr, data );
            }
            else if ( streq(typeStr, "string")) {
                RenderString( nameStr, data );
            }

            data = zconfig_next(data);
        }
    }

    void RenderCustomReport() {
        const int LABEL_WIDTH = 25;
        const int VALUE_WIDTH = 50;

        sphactor_report_t * report = sphactor_report(actor);
        assert(report);
        zosc_t * customData = sphactor_report_custom(report);
        if ( customData ) {
            const char* address = zosc_address(customData);
            ImGui::Text("%s", address);

            char type = '0';
            const void *data = zosc_first(customData, &type);
            bool name = true;
            while( data ) {
                if ( name ) {
                    //expecting a name string for each piece of data
                    assert(type == 's');

                    char* nameStr;
                    zosc_pop_string(customData, &nameStr);

                    ImGui::BeginGroup();
                    ImGui::SetNextItemWidth(LABEL_WIDTH);
                    ImGui::Text("%s:", nameStr);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(VALUE_WIDTH);
                }
                else {
                    switch(type) {
                        case 's': {
                            char* value;
                            zosc_pop_string(customData, &value);
                            ImGui::Text("%s", value);
                        } break;
                        case 'c': {
                            char value;
                            zosc_pop_char(customData, &value);
                            ImGui::Text("%c", value);
                        } break;
                        case 'i': {
                            int32_t value;
                            zosc_pop_int32(customData, &value);
                            ImGui::Text("%i", value);
                        } break;
                        case 'h': {
                            int64_t value;
                            zosc_pop_int64(customData, &value);
                            ImGui::Text("%lli", value);
                        } break;
                        case 'f': {
                            float value;
                            zosc_pop_float(customData, &value);
                            ImGui::Text("%f", value);
                        } break;
                        case 'd': {
                            double value;
                            zosc_pop_double(customData, &value);
                            ImGui::Text("%f", value);
                        } break;
                        case 'F': {
                            ImGui::Text("FALSE");
                        } break;
                        case 'T': {
                            ImGui::Text("TRUE");
                        } break;
                    }

                    ImGui::EndGroup();
                }

                //flip expecting name or value
                name = !name;
                data = zosc_next(customData, &type);
            }
        }
    }

    void SolvePadding( int* position ) {
        if ( *position % 4 != 0 ) {
            *position += 4 - *position % 4;
        }
    }

    template<typename T>
    void RenderValue(T *value, const byte * bytes, int *position) {
        memcpy(value, bytes+*position, sizeof(T));
        *position += sizeof(T);
    }

    void RenderInt(const char* name, zconfig_t *data) {
        int value;
        int min = 0, max = 0, step = 0;

        zconfig_t * zvalue = zconfig_locate(data, "value");
        zconfig_t * zapic = zconfig_locate(data, "api_call");
        zconfig_t * zapiv = zconfig_locate(data, "api_value");
        assert(zvalue);

        zconfig_t * zmin = zconfig_locate(data, "min");
        zconfig_t * zmax = zconfig_locate(data, "max");
        zconfig_t * zstep = zconfig_locate(data, "step");

        ReadInt( &value, zvalue);
        ReadInt( &min, zmin);
        ReadInt( &max, zmax);
        ReadInt( &step, zstep);

        ImGui::SetNextItemWidth(100);
        if ( ImGui::InputInt( name, &value, step, 100,ImGuiInputTextFlags_EnterReturnsTrue ) ) {
            if ( min != max ) {
                if ( value < min ) value = min;
                if ( value > max ) value = max;
            }

            zconfig_set_value(zvalue, "%i", value);
            SendAPI<int>(zapic, zapiv, zvalue, &value);
        }
    }

    void RenderFloat(const char* name, zconfig_t *data) {
        float value;
        float min = 0, max = 0, step = 0;

        zconfig_t * zvalue = zconfig_locate(data, "value");
        zconfig_t * zapic = zconfig_locate(data, "api_call");
        zconfig_t * zapiv = zconfig_locate(data, "api_value");
        assert(zvalue);

        zconfig_t * zmin = zconfig_locate(data, "min");
        zconfig_t * zmax = zconfig_locate(data, "max");
        zconfig_t * zstep = zconfig_locate(data, "step");

        ReadFloat( &value, zvalue);
        ReadFloat( &min, zmin);
        ReadFloat( &max, zmax);
        ReadFloat( &step, zstep);

        ImGui::SetNextItemWidth(100);
        if ( min != max ) {
            if ( ImGui::SliderFloat( name, &value, min, max) ) {
                zconfig_set_value(zvalue, "%f", value);
                SendAPI<float>(zapic, zapiv, zvalue, &value);
            }
        }
        else {
            if ( ImGui::InputFloat( name, &value, min, max) ) {
                zconfig_set_value(zvalue, "%f", value);
                SendAPI<float>(zapic, zapiv, zvalue, &value);
            }
        }
    }

    void RenderString(const char* name, zconfig_t *data) {
        int max = MAX_STR_DEFAULT;

        zconfig_t * zvalue = zconfig_locate(data, "value");
        zconfig_t * zapic = zconfig_locate(data, "api_call");
        zconfig_t * zapiv = zconfig_locate(data, "api_value");
        assert(zvalue);

        zconfig_t * zmax = zconfig_locate(data, "max");

        ReadInt( &max, zmax );

        char buf[MAX_STR_DEFAULT];
        const char* zvalueStr = zconfig_value(zvalue);
        strcpy(buf, zvalueStr);
        char *p = &buf[0];

        ImGui::SetNextItemWidth(200);
        if ( ImGui::InputText(name, buf, max, ImGuiInputTextFlags_EnterReturnsTrue ) ) {
            zconfig_set_value(zvalue, "%s", buf);
            SendAPI<char*>(zapic, zapiv, zvalue, &(p);
        }
    }

    void ReadInt( int *value, zconfig_t * data) {
        if ( data != NULL ) {
            *value = atoi(zconfig_value(data));
        }
    }

    void ReadFloat( float *value, zconfig_t * data) {
        if ( data != NULL ) {
            *value = atof(zconfig_value(data));
        }
    }

    void CreateActor() {
    }

    void DestroyActor() {
        sphactor_destroy(&actor);
    }

    void DeleteConnection(const Connection& connection)
    {
        for (auto it = connections.begin(); it != connections.end(); ++it)
        {
            if (connection == *it)
            {
                connections.erase(it);
                break;
            }
        }
    }

    //TODO: Add custom report data to this?
    void SerializeActorData(zconfig_t *section) {
        zconfig_t *xpos = zconfig_new("xpos", section);
        zconfig_set_value(xpos, "%f", pos.x);

        zconfig_t *ypos = zconfig_new("ypos", section);
        zconfig_set_value(ypos, "%f", pos.y);

        // Parse current state of data capabilities
        if ( this->capabilities ) {
            zconfig_t *root = zconfig_locate(this->capabilities, "capabilities");
            if ( root ) {
                zconfig_t *data = zconfig_locate(root, "data");
                while ( data ) {
                    zconfig_t *name = zconfig_locate(data,"name");
                    zconfig_t *value = zconfig_locate(data,"value");

                    char *nameStr = zconfig_value(name);
                    char *valueStr = zconfig_value(value);

                    zconfig_t *stored = zconfig_new(nameStr, section);
                    zconfig_set_value(stored, "%s", valueStr);

                    data = zconfig_next(data);
                }
            }
        }
    }

    void DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it) {
        char* xpos = *it;
        it++;
        char* ypos = *it;
        it++;

        if ( it != args->end()) {
            if ( this->capabilities ) {
                zconfig_t *root = zconfig_locate(this->capabilities, "capabilities");
                if ( root ) {
                    zconfig_t *data = zconfig_locate(root, "data");
                    while ( data && it != args->end() ) {
                        zconfig_t *value = zconfig_locate(data,"value");
                        char* valueStr = *it;
                        zconfig_set_value(value, "%s", valueStr);

                        HandleAPICalls(data);
                        data = zconfig_next(data);
                        it++;
                    }
                }
            }
        }

        pos.x = atof(xpos);
        pos.y = atof(ypos);

        free(xpos);
        free(ypos);
    }
};

#endif
