"use strict";

class UIFileDialog extends Com {
  constructor (name, param, style) {
    super(name, param, style, (json) => {
      let table = this._list_of_files;
     
      function ReduceSize(sz_in_bytes) {
        let kb = sz_in_bytes / 1024;
        let mb = kb / 1024;
        let gb = mb / 1024;
        if (gb >= 1.0) {
          return gb.toFixed(3) + " Гб";
        }
        if (mb >= 1.0) {
          return mb.toFixed(3) + " Мб";
        }
        if (kb >= 1.0) {
          return kb.toFixed(3) + " Кб";
        }
        return sz_in_bytes + " байт";
      } // ReduceSize
      
      function AddFile(fname, fattrs, pos, style) {
        let row  = table.insertRow(pos);
        let cell = row.insertCell();
        if (style) {
          row.className = style;
        }
        widget.SetTextFor(cell, fname);
        widget.SetTextFor(row.insertCell(),
          fattrs.size ? ReduceSize(fattrs.size) : "---"
        );
        return cell;
      } // AddFile

      this._selected_file = {};      
      table.innerHTML = "";
      let list_of_dirs = [];
      for (let fname in json.files) {
        let file = json.files[fname];
        if (file.is_directory === true) {
          if (fname === "..") {
            list_of_dirs.push(fname);
            continue;  
          }
          list_of_dirs.unshift(fname);
          continue;
        }
        AddFile(fname, file).addEventListener("click", 
          () => {
            this._selected_file = {
              "path": json.cur_dir,
              "name": fname
            };
            if (widget.IsFunction(this.onUpdate)) {
              this.onUpdate(); 
            }
        });        
      } // for ...
      
      list_of_dirs.forEach((dir) => {
        AddFile(dir, {}, 0, this._style.STYLE_OF_DIR).addEventListener("click", 
        () => {
          this.sendAction("goto", {"file": dir});
        });        
      });
      this._path_panel.innerHTML = "";
      if (json && json.cur_dir) {
        json.cur_dir.split("/").forEach((val) => {
          let path_node = document.createElement("li");
          widget.SetTextFor(path_node, val);
          this._path_panel.appendChild(path_node);
        });
      }
    }); // super
    
    this._selected_file = {};
    this._node          = document.createElement("div");
    this._path_panel    = document.createElement("ol");
    this._list_of_files = document.createElement("table");
    this._menu_confirm  = document.createElement("button");
    this._menu_cancel   = document.createElement("button");
    if (style) {
      this._path_panel.className    = style.PATH_STYLE;
      this._list_of_files.className = style.BASE_STYLE;
      this._menu_cancel.className   = style.NAV_BUTTON_STYLE;
      this._menu_confirm.className  = style.NAV_BUTTON_STYLE;
    }
    widget.SetTextFor(this._menu_confirm, "Открыть");
    widget.SetTextFor(this._menu_cancel,  "Отмена");
    this._node.appendChild(this._path_panel);
    this._node.appendChild(this._list_of_files);
    
    this._menu_confirm.addEventListener("click", () => {
      if (widget.IsFunction(this.onOpen)) {
        if (this._selected_file.name === undefined) {
          return;
        }
        let path = `${this._selected_file.path}/${this._selected_file.name}`;
        this.sendAction("open", {"file": path});
        this.onOpen();
      }
    });
    
    this._menu_cancel.addEventListener("click", () => {
      if (widget.IsFunction(this.onClose)) {
        this.onClose(); 
      }
    });
  } // constructor

  get title() {
    if (this._selected_file.name !== undefined) {
      return this._param.TITLE + ": " + this._selected_file.name;  
    }
    return this._param.TITLE;
  }

  get navigation() {
    return {
      "nav_ctl_0": this._menu_confirm,
      "nav_ctl_1": this._menu_cancel 
    };
  }
} // class UIFileDialog