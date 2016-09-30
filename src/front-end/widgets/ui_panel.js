"use strict";

class UIPanel extends Com {
  constructor (name, param, style) {
    super(name, param, style, (json) => {
      // TODO ...
    }); // super
    this._node = document.createElement("nav");
    let menu     = document.createElement("ul");
    let item_num = 0;
    for (let idx in this._param) {
      let bt_node   = this._param[idx];
      let menu_item = document.createElement("li");
      let dummy_a   = document.createElement("a");
      if (item_num == 0) {
        menu_item.className = style.MENU_ITEM_ACTIVE;
      }
      item_num++;
      widget.SetTextFor(dummy_a, bt_node.LABEL);
      dummy_a.addEventListener("click", () => { 
        eval(bt_node.ACTION)
        let prev_item = menu.getElementsByClassName(style.MENU_ITEM_ACTIVE)[0];
        if (prev_item !== undefined) {
          prev_item.className = "";
        }
        menu_item.className = style.MENU_ITEM_ACTIVE;
      });
      menu_item.appendChild(dummy_a);
      menu.appendChild(menu_item);
    }
    menu.className = this._style.NAV_BUTTON_STYLE;
    this._node.className = this._style.BASE_STYLE;
    this._node.appendChild(menu);
  } // constructor
  
  changeLayout(name) {
    widget.setup.UI_LAYOUT = name;
    widget.ConstructLayout();
  }
  
  syncWithBackend() {
   // экземпляры объекта не синхронизируются с back-end
   this.onSync(); 
  }
} // class UIPanel