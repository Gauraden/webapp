"use strict";

class UITable extends Com {
  constructor (name, param, style) {
    super(name, param, style, (json) => {
      this.updateHeader(json);
      this.updateBody(json);      
    }); // super
    this._forms      = [];
    this._node       = document.createElement("div");
    this._forms_grp  = document.createElement("div");
    this._table      = document.createElement("table");
    this._table_head = document.createElement("thead");
    this._table_body = document.createElement("tbody");
    
    this._table.appendChild(this._table_head);
    this._table.appendChild(this._table_body);
    this._table.className = style.BASE_STYLE;
    
    this._setupForm(this._param, this._forms_grp);
    this._node.appendChild(this._table);
  } // constructor

  get form() {
    return this._forms_grp;
  }

  _setupForm(form_layouts, form_container) {
    widget.ConstructCOM(form_layouts, (com_obj) => {
      if (com_obj.setRequestSender) {
        com_obj.setRequestSender(this);
      }
      this._forms.push(com_obj);
      form_container.appendChild(com_obj.html_node);        
    });    
  }

  syncWithBackend() {
    super.syncWithBackend();
    this._forms.forEach((form) => {
      form.syncWithBackend();
    });
  }
  
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
    widget.ReplaceDOM(this._table_head, head_row);
  }
  
  updateBody(json) {
    if (json.data === undefined) {
      return;
    }
    this._table_body.innerHTML = "";
    for (let row_key in json.data) {
      let row_json = json.data[row_key];
      let row_node = document.createElement("tr");
      for (let col_key in row_json) {
        let col_node = document.createElement("td");
        widget.SetTextFor(col_node, row_json[col_key]);
        row_node.appendChild(col_node);
      }
      this._table_body.appendChild(row_node);
    }
  }
} // class UITable