const id = "PM_EVENT_TOGGLE_WALL_SLIDE";
const groups = ["Platformer+"];
const name = "Toggle Wall Slide";

const fields = [
  {
    key: "wallSlideEnabled",
    label: "Enable or Disable Wall Slide",
    type: "select",
    defaultValue: "0",
    options: [
      ["0", "Disable Wall Slide"],
      ["1", "Enable Wall Slide"],
    ],
  },
  {
    key: "field",
    defaultValue: "plat_wall_slide",
  },
];

const compile = (input, helpers) => {
  const {
    _addComment,
    _addNL,
    _setConstMemInt16,
    _setMemInt16ToVariable,
  } = helpers;
  _addComment("Toggle Wall Slide");
  _setConstMemInt16(input.field, input.wallSlideEnabled);

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