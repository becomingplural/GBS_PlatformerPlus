const id = "PM_EVENT_ACTOR_GRAVITY_OFF";
const groups = ["EVENT_GROUP_ACTOR", "Platformer+"];
const name = "Disable Actor Gravity";

const fields = [
  {
    key: "actorId",
    label: "Actor",
    description: "Disable Actor Gravity",
    type: "actor",
    defaultValue: "$self$",
  },
];

const compile = (input, helpers) => {
    const { _addComment, _addNL, _callNative, _stackPushConst, _stackPop, actorPushById } =
      helpers;
      _addComment("Disables actor gravity");
      actorPushById(input.actorId);
      _callNative("actor_gravity_off");
      _stackPop(1);
  
    _addNL();
  };
  
  module.exports = {
    id,
    name,
    groups,
    fields,
    compile,
    allowedBeforeInitFade: true,
  };
  