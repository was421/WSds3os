﻿//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version:4.0.30319.42000
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

namespace Loader {
    
    
    [global::System.Runtime.CompilerServices.CompilerGeneratedAttribute()]
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.Editors.SettingsDesigner.SettingsSingleFileGenerator", "17.1.0.0")]
    internal sealed partial class ProgramSettings : global::System.Configuration.ApplicationSettingsBase {
        
        private static ProgramSettings defaultInstance = ((ProgramSettings)(global::System.Configuration.ApplicationSettingsBase.Synchronized(new ProgramSettings())));
        
        public static ProgramSettings Default {
            get {
                return defaultInstance;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("")]
        public string exe_location {
            get {
                return ((string)(this["exe_location"]));
            }
            set {
                this["exe_location"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("")]
        public string server_config_json {
            get {
                return ((string)(this["server_config_json"]));
            }
            set {
                this["server_config_json"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("http://ds3os-master.timleonard.uk:50020")]
        public string master_server_url {
            get {
                return ((string)(this["master_server_url"]));
            }
            set {
                this["master_server_url"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("True")]
        public bool hide_passworded {
            get {
                return ((bool)(this["hide_passworded"]));
            }
            set {
                this["hide_passworded"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("5")]
        public int minimum_players {
            get {
                return ((int)(this["minimum_players"]));
            }
            set {
                this["minimum_players"] = value;
            }
        }
    }
}
