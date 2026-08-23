Scriptname HelloWorldMCM extends MCM_ConfigBase

String Property MessageDelaySetting = "iMessageDelay:General" AutoReadOnly
String Property SettingsChangedEvent = "HelloWorld_SettingsChanged" AutoReadOnly

Event OnSettingChange(String a_ID)
    If a_ID == MessageDelaySetting
        SendModEvent(SettingsChangedEvent, a_ID)
    EndIf
EndEvent
