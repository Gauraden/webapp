"use strict";

const ELEMENT_NODE_ID = 1;
const NAME_ATTR       = Symbol("NAME_ATTR");
const INNER_TEXT_ATTR = Symbol("INNER_TEXT_ATTR");

class Com {
  constructor (name, param, style, onDraw) {
    this._name     = name;
    this._node     = undefined;
    this._notifier = undefined;
    this._param    = param;
    this._style    = style;
    this._onDraw   = onDraw;
  }
  
  get name() {
    return this._name;
  }
  
  get html_node() {
    return this._node;
  }
  
  get title() {
    return this._param.TITLE;
  }
  
  get navigation() {
    return undefined;
  }
  
  set style(style) {
    this._style = style;
  }
  
  _showNotifier(notice) {
    if (notice === undefined) {
      return false;
    }
    if (notice.status === "finished") {
      this._hideNotifier();
      return false;
    }
    if (this._notifier === undefined) {
      this._notifier = document.createElement("div");
      this._notifier.className = "notifier";
      //this._notifier.style.top = `${this._node.style.top + 10}px`;
      this._node.appendChild(this._notifier);
    }
    let progress = "...";
    if (notice.progress !== undefined) {
      progress = `${notice.progress} %`;
    }
    this._notifier.innerText = `Ожидание: ${notice.message} ${progress}`;
    return true;
  }
  
  _hideNotifier() {
    if (this._notifier === undefined) {
      return;
    }
    this._node.removeChild(this._notifier);
    this._notifier = undefined;
  }
  
  encodeString(str) {
    return str.replace(/([=&]{1,1})/g, (str, templ) => { 
      return "%" + templ.charCodeAt(0).toString(16);
    });
  }
  
  sendAction(action = "", args = {}) {
    let request = "";
    if (action && action !== "") {
      request = "?action=" + action;
      if (args) {
        for (let name in args) {
          let value = this.encodeString(args[name]);
          request += `&${name}=${value}`;
        }
      }
    }
    SendAsyncRequest(`webui/${this._name}${request}`, this);
  }
  
  syncWithBackend() {
    this.sendAction("sync");
  }
  
  onSync(json) {
    // обработка ожидания завершения длительного процесса
    // на сервере
    if (json !== undefined &&
        this._showNotifier(json.notification)) {
      setTimeout(() => {
        this.syncWithBackend();
      }, 1000);
      return;
    }
    //console.log("onUpdate: " + this._name + ": " + JSON.stringify(json));
    this._onDraw(json);
  }
} // class Com

let widget = {
  "objects": {},
  "styles" : {},
  "layouts": {},
  "setup"  : {},
  "IsFunction": (obj) => {
    return (obj && obj.constructor === Function);
  },
  "SetTextFor": (node, text) => {
    if (node === undefined) {
      return;
    }
    node.innerHTML = "";
    node.appendChild(document.createTextNode(text));
  },
  "ReplaceDOM": (where, new_dom) => {
    where.innerHTML = "";
    where.appendChild(new_dom);
  },
  "RemoveAllSiblingsOf": (node) => {
    for (let sibling = node.nextSibling;
         sibling !== null;
         sibling = node.nextSibling) {
      sibling.parentNode.removeChild(sibling);
    }
  },
  "ParseUiComParam": (cont, node, parent_name) => {
    let param = node.firstChild;
    for (; param !== null; param = param.nextSibling) {
      if (param.nodeType != ELEMENT_NODE_ID) {
        continue; 
      }
      let child      = param.children;
      let name       = param.getAttribute("name");
      let param_cont = {};
      if (name !== null) {
        if (parent_name && parent_name.constructor === String) {
          name = `${parent_name}/${name}`;
        }
        param_cont[NAME_ATTR] = name;
      }
      if (child.length == 0) {
        if (name === null) {
          cont[param.nodeName] = param.innerText;
          continue;
        }
        param_cont[INNER_TEXT_ATTR] = param.innerText;
        cont[param.nodeName] = param_cont;
        continue;
      }
      widget.ParseUiComParam(param_cont, param, name);
      cont[param.nodeName] = param_cont;
    }
  },
  "ParseUiStyle": (webapp) => {
    let ui_styles = webapp.getElementsByTagName("ui_style");
    for (let idx = 0; idx < ui_styles.length; idx++) {
      let style  = ui_styles[idx];
      let name   = style.getAttribute("name");
      let ui_com = style.firstChild;
      widget.styles[name] = {};
      for (; ui_com !== null; ui_com = ui_com.nextSibling) {
        if (ui_com.nodeType != ELEMENT_NODE_ID) {
          continue;
        }
        let ui_com_type  = ui_com.nodeName;
        widget.styles[name][ui_com_type] = {};
        widget.ParseUiComParam(widget.styles[name][ui_com_type], ui_com);
      }
    }
  },
  "ParseUiLayout": (webapp) => {
    let ui_layouts = webapp.getElementsByTagName("ui_layout");
    for (let idx = 0; idx < ui_layouts.length; idx++) {
      let layout = ui_layouts[idx];
      let name   = layout.getAttribute("name");
      let ui_com = layout.firstChild;
      widget.layouts[name] = {};
      for (; ui_com !== null; ui_com = ui_com.nextSibling) {
        if (ui_com.nodeType != ELEMENT_NODE_ID) {
          continue;
        }
        let ui_com_type  = ui_com.nodeName;
        let ui_com_name  = ui_com.getAttribute("name");
        widget.layouts[name][ui_com_type] = {};
        widget.ParseUiComParam(widget.layouts[name][ui_com_type],
          ui_com,
          ui_com_name
        );
        widget.layouts[name][ui_com_type][NAME_ATTR] = ui_com_name;
      }
    }
  },
  "ConstructCOM": (layout, com_handler) => {
    let style_name = widget.setup.UI_STYLE;
    let style      = widget.styles[style_name];
    if (style === undefined) {
      console.log(`Error: ui style \"${style_name}\" was not found!`);
      return;
    }
    for (let com_name in layout) {
      let splited_name = com_name.split("_");
      if (splited_name[0] !== "UI") {
        continue;
      }
      let com_type = com_name;
      if (splited_name[splited_name.length - 1].match(/([0-9]+)/)) {
        splited_name.pop();
        com_type = splited_name.join("_");
      }      
      let com_param = layout[com_name];
      let com_style = style[com_type];
      let com_obj = widget.Constructor(
        com_type,
        com_param,
        com_style
      );
      if (com_obj     !== undefined &&
          com_handler !== undefined) {
        com_handler(com_obj);
      }
    }
  },
  "ConstructLayout": () => {
    let layout_out  = widget.setup.UI_OUTPUT;
    let layout_name = widget.setup.UI_LAYOUT;
    if (layout_out  === undefined || 
        layout_name === undefined) {
      return false;
    }
    let layout = widget.layouts[layout_name];
    let out    = document.getElementById(layout_out);
    if (layout === undefined || out === undefined) {
      return false;
    }
    out.innerHTML = "";
    widget.ConstructCOM(layout, (com_obj) => {
      out.appendChild(com_obj.html_node);
      com_obj.syncWithBackend();      
    });
  },
  "SetupUI": () => {
    let webapp = document.getElementsByTagName("webapp")[0];
    widget.ParseUiStyle(webapp);
    widget.ParseUiLayout(webapp);
    let ui_setup = webapp.getElementsByTagName("ui_setup")[0];
    let ui_param = ui_setup.firstChild;
    for (; ui_param !== null; ui_param = ui_param.nextSibling) {
      if (ui_param.nodeType != ELEMENT_NODE_ID) {
        continue;
      }
      let param_name = ui_param.nodeName;
      widget.setup[param_name] = ui_param.getAttribute("use");
    }
    widget.ConstructLayout();
    webapp.innerHTML = "";
  },
  "Constructor": (wtype, wparam, wstyle) => {
    let wname = wparam[NAME_ATTR];
    let func  = widget.funcs[wtype];
    if (func === undefined || func.constructor !== Function) {
      console.log("Error: unknown type of UI component: " + wtype);
      return undefined;
    }
    let wobj = widget.objects[wname];
    if (wobj === undefined) {
      wobj = func(wname, wparam, wstyle);
      widget.objects[wname] = wobj;
    }
    wobj.style = wstyle;
    return wobj;
  }, // "Constructor"
  "funcs": {
    "UI_FILE_DIALOG": (name, param, style) => {
      return new UIFileDialog(name, param, style);
    },
    "UI_BUTTON": (name, param, style) => {
      return new UIButton(name, param, style);
    },
    "UI_PANEL": (name, param, style) => {
      return new UIPanel(name, param, style);
    },
    "UI_TABLE": (name, param, style) => {
      return new UITable(name, param, style);
    },
    "UI_CONCEPT": (name, param, style) => {
      return new UIConcept(name, param, style);
    },
    "UI_INVITATION": (name, param, style) => {
      return new UIInvitation(name, param, style);
    },
    "UI_DATE_RANGE": (name, param, style) => {
      return new UIDateRange(name, param, style);
    },
    "UI_FORM": (name, param, style) => {
      return new UIForm(name, param, style);
    }
  } // funcs
}; // widget

function SendAsyncRequest(relative_url, com) {
  let xhr = new XMLHttpRequest();
  xhr.open("GET", `http://${location.host}/${relative_url}`, true);
  xhr.onreadystatechange = () => {
    if (xhr.readyState  == 4 &&
        xhr.status      == 200) {
      let resp_json = undefined;
      eval("resp_json = " +  xhr.responseText);
      com.onSync(resp_json);
    }
  };
  xhr.send(null);
}

function RunWebAppUI() {
  widget.SetupUI();
}