Scriptname HelloWorldMCM extends MCM_ConfigBase

String Property MessageDelaySetting = "iMessageDelay:General" AutoReadOnly
String Property MessageTextSetting = "sMessageText:General" AutoReadOnly
String Property SettingsChangedEvent = "HelloWorld_SettingsChanged" AutoReadOnly

Event OnSettingChange(String a_ID)
    If a_ID == MessageDelaySetting || a_ID == MessageTextSetting
        SendModEvent(SettingsChangedEvent, a_ID)
    EndIf
EndEvent
