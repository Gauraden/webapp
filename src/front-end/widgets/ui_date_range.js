"use strict";

class UIDateRange extends Com {
  constructor (name, param, style) {
    super(name, param, style, (json) => {
      // TODO
    }); // super
    
    function AddDTPicker(parent, id, label) {
      let cell = document.createElement("div");
      let node = document.createElement("input");
      node.setAttribute("type", "text");
      node.setAttribute("id", id);
      node.className = style.INPUT_STYLE;
      cell.className = style.INPUT_CELL_STYLE;
      cell.appendChild(node);
      if (label !== undefined) {
        let label_cell = document.createElement("div");
        label_cell.className = style.LABEL_CELL_STYLE;
        widget.SetTextFor(label_cell, label);
        parent.appendChild(label_cell);
      }
      parent.appendChild(cell);
    }
    
    this._node = document.createElement("div");
    this._node.className = style.BASE_STYLE;
    AddDTPicker(this._node, this._getId("from"), param.LABEL_FROM);
    AddDTPicker(this._node, this._getId("to"),   param.LABEL_TO);
  } // constructor
  
  _getId(postfix) {
    let esc_name = this._name.replace(/\//g, "-");
    return `${esc_name}_${postfix}`;
  }
  
  getArgsForRequest() {
    let dt_from = Date.parse(document.getElementById(this._getId("from")).value);
    let dt_to   = Date.parse(document.getElementById(this._getId("to")).value);
    return `from_dt=${dt_from}&&to_dt=${dt_to}`;
  }
  
  syncWithBackend() {
    let dt_from = $("#" + this._getId("from"));
    let dt_to   = $("#" + this._getId("to"));

    dt_from.datetimepicker({
      locale: "ru",
      format: "YYYY-MM-DD HH:mm:ss"
    });
    dt_to.datetimepicker({
      locale         : "ru",
      format         : "YYYY-MM-DD HH:mm:ss",
      showTodayButton: true,
      useCurrent     : false //Important! See issue #1075
    });
    dt_from.on("dp.change", function (e) {
        dt_to.data("DateTimePicker").minDate(e.date);
    });
    dt_to.on("dp.change", function (e) {
        dt_from.data("DateTimePicker").maxDate(e.date);
    });
  }
} // class UIButton