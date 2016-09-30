"use strict";

class UIInvitation extends Com {
  constructor (name, param, style) {
    super(name, param, style, (json) => {
      // ... 
    }); // super
    this._node = document.createElement("div");
    let msg = "";
    for (let part_id in param) {
      let part_val = param[part_id];
      if (part_val.constructor === String) {
        msg += part_val; 
      }
    }
    widget.SetTextFor(this._node, msg);
    this._node.className = style.BASE_STYLE;
    this._node.addEventListener("click", () => {
      if (widget.IsFunction(this.onOpen)) {
        this.onOpen();
      }
    });
  } // constructor
} // class UIButton