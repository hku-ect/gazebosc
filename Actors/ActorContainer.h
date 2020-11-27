#ifndef ACTORCONTAINER_H
#define ACTORCONTAINER_H

#include "libsphactor.h"
#include "GActor.h"

struct ActorContainer {
  /// Title which will be displayed at the center-top of the actor.
  const char* title = nullptr;
  /// Flag indicating that actor is selected by the user.
  bool selected = false;
  /// Actor position on the canvas.
  ImVec2 pos{};
  /// List of actor connections.
  std::vector<Connection> connections{};
  /// A list of input slots current actor has.
  std::vector<ImNodes::Ez::SlotInfo> input_slots{ { "OSC", ActorSlotOSC } };
  /// A list of output slots current actor has.
  std::vector<ImNodes::Ez::SlotInfo> output_slots{ { "OSC", ActorSlotOSC } };

  sphactor_t *actor;

  ActorContainer(sphactor_t *actor) {
    this->actor = actor;
    this->title = sphactor_ask_actor_type(actor);
    //HACK
    sphactor_ask_set_timeout(actor, 16);
  }

  ~ActorContainer() {

  }

  void Render(float deltaTime) {

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
  }

  void DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it) {
      char* xpos = *it;
      it++;
      char* ypos = *it;
      it++;

      pos.x = atof(xpos);
      pos.y = atof(ypos);

      free(xpos);
      free(ypos);
  }
};

#endif
