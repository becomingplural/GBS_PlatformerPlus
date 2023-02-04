const id = "PM_EVENT_PLATPLUS_FIELD_SET";
const groups = ["Platformer+", "Player Fields", "EVENT_GROUP_VARIABLES"];
const name = "Update Platformer+ Field";

const fields = [
  {
    key: "field",
    label: "Field",
    type: "select",
    defaultValue: "dj_val",
    options: [
      ["dj_val", "Number of double jumps left"],
      ["wj_val", "Number of wall jumps left"],
    ],
  },
  {
    type: "group",
    width: "50%",
    fields: [
      {
        key: "variable",
        type: "variable",
        defaultValue: "LAST_VARIABLE",
        conditions: [
          {
            key: "type",
            eq: "variable",
          },
        ],
      },
      {
        key: "value",
        type: "number",
        defaultValue: 0,
        min: 0,
        max: 255,
        conditions: [
          {
            key: "type",
            eq: "number",
          },
        ],
      },
      {
        key: "type",
        type: "selectbutton",
        options: [
          ["variable", "variable"],
          ["number", "number"],
        ],
        inline: true,
        defaultValue: "number",
      },
    ],
  },
];

const compile = (input, helpers) => {
  const { _addComment, _addNL, _setConstMemInt16, _setMemInt16ToVariable } =
    helpers;

  if (input.type === "variable") {
    _addComment("Platformer Plus Field Set To Variable");
    _setMemInt16ToVariable(input.field, input.variable);
  } else {
    _addComment("Platformer Plus Field Set To Value");
    _setConstMemInt16(input.field, input.value);
  }
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