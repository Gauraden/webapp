"use strict";

class UIForm extends Com {
  constructor (name, param, style) {
    super(name, param, style, (json) => {
      // TODO
    }); // super
    this._fields     = [];
    this._req_sender = undefined;
    this._node       = document.createElement("form");
    this._submit_bt  = document.createElement("button");
    
    widget.ConstructCOM(this._param, (com_obj) => {
      this._fields.push(com_obj);
      this._node.appendChild(com_obj.html_node);
    });
    
    widget.SetTextFor(this._submit_bt, "Найти");
    
    this._submit_bt.addEventListener("click", () => {
      if (this._req_sender === undefined) {
        return; 
      }
      let args = "";
      this._fields.forEach((field) => {
        args += (args.length > 0 ? "&&" : "") + field.getArgsForRequest();        
      });
      this._req_sender.sendAction("select", {
        "where": args
      });
    });
    
    this._node.addEventListener("submit", (event) => {
      event.preventDefault();
      return false;
    })
    
    this._submit_bt.className = style.SUBMIT_STYLE;
    this._node.appendChild(this._submit_bt);
  } // constructor
  
  setRequestSender(sender) {
    this._req_sender = sender;
  }
  
  syncWithBackend() {
    //super.syncWithBackend();
    this._fields.forEach((field) => {
      field.syncWithBackend();
    });
  }
} // class UIButton