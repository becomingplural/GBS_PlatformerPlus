const id = "PM_EVENT_PLATPLUS_FIELD_STORE";
const groups = ["Platformer+", "Player Fields", "EVENT_GROUP_VARIABLES"];
const name = "Store Platformer+ Fields In a Variable";

const fields = [
  {
    key: "field",
    label: "Field",
    type: "select",
    defaultValue: "mp_attached",
    options: [
      ["mp_attached", "Player on a moving platform (true false)"],
      ["run_stage", "Current run stage (0-4)"],
      ["jump_type", "Current jump type (0: none, 1: ground, 2: double, 3: wall)"],
      ["ct_val", "Frames of coyote time left"],
      ["dash_interrupt", "Checks if Dashing is Frozen (true false)"],
    ],
  },
  {
    key: "variable",
    type: "variable",
    defaultValue: "LAST_VARIABLE",
  },
];

const compile = (input, helpers) => {
  const { appendRaw, getVariableAlias, _addComment } = helpers;

  const fieldVarTypeLookup = {
    actor_attached: "UINT8",
    run_stage: "INT8",
    jump_type: "UINT8",
    dash_interrupt: "UINT8",
    ct_val: "UINT8",
  };

  const fieldName = `_${input.field}`;
  const variableAlias = getVariableAlias(input.variable);

  _addComment("Store player field in variable");
  appendRaw(
    `VM_GET_${fieldVarTypeLookup[input.field]} ${variableAlias}, ${fieldName}`
  );
};

module.exports = {
  id,
  name,
  groups,
  fields,
  compile,
  allowedBeforeInitFade: true,
};
