const id = "PM_EVENT_PLATPLUS_STATE_STORE";
const groups = ["Platformer+", "Player Fields", "EVENT_GROUP_VARIABLES"];
const name = "Store Platformer+ State In a Variable";

const fields = [
  {
    label: "Gets player state and stores it in a variable."
  },
  {
    key: "variable",
    type: "variable",
    defaultValue: "LAST_VARIABLE",
  },
  {
    key: "field",
    defaultValue: "plat_state",
  },
  {
    label: "0 = Started Falling, 1 = Falling",
  },
  {
    label: "2 = Started Landing, 3 = On the Ground", 
  },

  {
    label: "4 = Started Jumping, 5 = Jumping",
  },

  {
    label: "6 = Started Dashing, 7 = Dashing",
  },
  {
    label: "8 = Started Climbing, 9 = On a Ladder",
  },
  {
    label: "10 = Started Sliding, 11 = On a Wall",
  },
  {
    label: "12 = Started Knockback, 13 = Knockback",
  },
  {
    label: "14 = Started Blank, 15 = Blank",
  },
];

const compile = (input, helpers) => {
  const { appendRaw, getVariableAlias, _addComment } = helpers;
  const fieldVarTypeLookup = {
    plat_state: "UINT8",
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
