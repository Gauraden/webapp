"use strict";

class UIButton extends Com {
  constructor (name, param, style) {
    super(name, param, style, (json) => {
      widget.SetTextFor(this._node, json.label);
    }); // super
    this._node = document.createElement("button");
  } // constructor
} // class UIButton