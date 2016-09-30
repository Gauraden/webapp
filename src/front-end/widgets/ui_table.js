"use strict";

class UITable extends Com {
  constructor (name, param, style) {
    super(name, param, style, (json) => {
      this.updateHeader(json);
      this.updateBody(json);      
    }); // super
    this._node      = document.createElement("table");
    this._node_head = document.createElement("thead");
    this._node_body = document.createElement("tbody");
    
    this._node.appendChild(this._node_head);
    this._node.appendChild(this._node_body);
    this._node.className = style.BASE_STYLE;
  } // constructor
  
  updateHeader(json) {
    if (json.meta === undefined) {
      return; 
    }
    let head_row = document.createElement("tr");
    for (let key in json.meta) {
      let col_node = document.createElement("th");
      widget.SetTextFor(col_node, json.meta[key].name);
      head_row.appendChild(col_node);
    }
    widget.ReplaceDOM(this._node_head, head_row);
  }
  
  updateBody(json) {
    if (json.data === undefined) {
      return;
    }
    this._node_body.innerHTML = "";
    for (let row_key in json.data) {
      let row_json = json.data[row_key];
      let row_node = document.createElement("tr");
      for (let col_key in row_json) {
        let col_node = document.createElement("td");
        widget.SetTextFor(col_node, row_json[col_key]);
        row_node.appendChild(col_node);
      }
      this._node_body.appendChild(row_node);
    }
  }
} // class UITable