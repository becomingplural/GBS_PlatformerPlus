const id = "PM_EVENT_DASH_INTERRUPT";
const groups = ["Platformer+", "Player Fields", "EVENT_GROUP_VARIABLES"];
const name = "Interrupt Dash Ability";

const fields = [
  {
    label: "Interrupts the player's ability to dash. Use this event to break a player out of a dash."
  },
  {
    key: "field",
    defaultValue: "dash_interrupt",
  },
  {
    key: "value",
    type: "number",
    defaultValue: 1,
  },
];

const compile = (input, helpers) => {
  const { _addComment, _addNL, _setConstMemInt16, _setMemInt16ToVariable } =
    helpers;
    _addComment("Set Dash Interrupt to True");
    _setConstMemInt16(input.field, input.value);

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
