Scriptname HelloWorldMCM extends MCM_ConfigBase

String Property MessageDelaySetting = "iMessageDelay:General" AutoReadOnly
String Property MessageTextSetting = "sMessageText:General" AutoReadOnly
String Property SettingsChangedEvent = "HelloWorld_SettingsChanged" AutoReadOnly

Event OnSettingChange(String a_ID)
    Debug.Trace("[HelloWorld MCM 0.3.1] OnSettingChange: " + a_ID)
    If a_ID == MessageDelaySetting
        SendModEvent("HelloWorld_MessageDelayChanged", a_ID, GetModSettingInt(a_ID))
    ElseIf a_ID == MessageTextSetting
        SendModEvent("HelloWorld_MessageTextChanged", GetModSettingString(a_ID))
    EndIf
EndEvent

Event OnConfigOpen()
    Parent.OnConfigOpen()
    Debug.Trace("[HelloWorld MCM 0.3.1] Menu opened")
    SendModEvent("HelloWorld_MenuOpened", "0.3.1")
EndEvent

Function ShowMessageTextHelp()
    Debug.MessageBox("SkyUI VR may hide text input fields. Edit sMessageText under [General] in Data/MCM/Settings/HelloWorld.ini, then reopen this menu to apply it. Leave it empty for default messages.")
EndFunction
