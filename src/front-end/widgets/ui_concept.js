"use strict";

class UIConcept extends Com {
  constructor (name, param, style) {
    super(name, param, style, (json) => {
      // ...
    }); // super
    
    this._stages       = [];
    this._stage_ptr    = 0;
    this._node         = document.createElement("div");
    this._control_node = document.createElement("div");
    this._nav_node     = document.createElement("div");
    this._stage_node   = document.createElement("div");
    this._stage_title  = document.createElement("div");
    this._stage_form   = document.createElement("div");
    this._stage_close  = document.createElement("button");
    
    this._nav_node.appendChild(this._stage_close);
    this._control_node.appendChild(this._stage_title);
    this._control_node.appendChild(this._nav_node);
    this._control_node.appendChild(this._stage_form);
    this._node.appendChild(this._control_node);
    this._node.appendChild(this._stage_node);
    
    this._prepareStages(this._param);
    
    this._stage_node.className   = style.BODY_STYLE;
    this._stage_title.className  = style.TITLE_STYLE;
    this._node.className         = style.BASE_STYLE;
    this._control_node.className = style.CTL_PANEL_STYLE;
    this._nav_node.className     = style.NAV_PANEL_STYLE;
    
    this._stage_close.className = style.NAV_PANEL_BUTTON_STYLE;
    this._stage_close.style.display = "none";
    this._stage_close.addEventListener("click", () => {
      this.switchToPrevStage()();
    });

    widget.SetTextFor(this._stage_close, "Закрыть");
    this._applySynergy(this._stages[this._stage_ptr]);
  } // constructor
  
  _setTitleText(text) {
    widget.SetTextFor(this._stage_title, text !== undefined ? text : "...");
  }
  
  _setTitleNavigation(nav) {
    widget.RemoveAllSiblingsOf(this._stage_close);
    if (nav === undefined) {
      return;
    }
    this._stage_close.style.display = "none";
    for (let nav_id in nav) {
      this._nav_node.appendChild(nav[nav_id]);
    }
  }
  
  _setTitleForm(form) {
    if (form === undefined) {
      return;
    }
    this._stage_form.innerHTML = "";
    this._stage_form.appendChild(form);
  }
  
  _applySynergy(stage) {
    if (stage === undefined) {
      return false;
    }
    this._setTitleText(stage.title);
    this._setTitleNavigation(stage.navigation);
    this._setTitleForm(stage.form);
    return true;
  }
  
  _prepareStages(param) {
    for (let stage_id in param) {
      let stage = param[stage_id];
      widget.ConstructCOM(stage, (com_obj) => {
        com_obj.onOpen   = this.switchToNextStage();
        com_obj.onClose  = this.switchToPrevStage();
        com_obj.onUpdate = this.updateCurrentStage();
        this._stages.push(com_obj);
        this._stage_node.appendChild(com_obj.html_node);
        if (this._stages.length > 1) {
          com_obj.html_node.style.display = "none";
        }        
      });
    }
  } // _prepareStages

  _switchStage(from_id, step) {
    this._stages[from_id].html_node.style.display = "none";
    let to_id      = from_id + step;
    let next_stage = this._stages[to_id]; 
    next_stage.html_node.style.display = "block";
    next_stage.syncWithBackend();
    if (to_id > 0) {
      this._stage_close.style.display = "block";
    } else {
      this._stage_close.style.display = "none";
    }
    this._applySynergy(next_stage);
    return to_id;
  }
  
  switchToNextStage() {
    return () => {
      if (this._stage_ptr == this._stages.length - 1) {
        return;
      }
      this._stage_ptr = this._switchStage(this._stage_ptr, 1);
    }
  }
  
  switchToPrevStage() {
    return () => {
      if (this._stage_ptr == 0) {
        return;
      }
      this._stage_ptr = this._switchStage(this._stage_ptr, -1);
    }
  }
  
  updateCurrentStage() {
    return () => {
      this._applySynergy(this._stages[this._stage_ptr]);
    }
  }
} // class UIButton