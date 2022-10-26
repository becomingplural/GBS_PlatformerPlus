const id = "PM_EVENT_ACTOR_GRAVITY_ON";
const groups = ["EVENT_GROUP_ACTOR", "Platformer+"];
const name = "Enable Actor Gravity";

const fields = [
  {
    key: "actorId",
    label: "Actor",
    description: "Enable Actor Gravity",
    type: "actor",
    defaultValue: "$self$",
  },
];

const compile = (input, helpers) => {
    const { _addComment, _addNL, _callNative, _stackPushConst, _stackPop, actorPushById } =
      helpers;
      _addComment("Enables actor gravity");
      actorPushById(input.actorId);
      _callNative("actor_gravity_on");
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
  