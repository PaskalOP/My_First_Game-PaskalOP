namespace Unity.LiveCapture.CompanionApp
{
    /// <summary>
    /// A class that contains the message IDs which define the companion app protocol.
    /// </summary>
    static class CompanionAppMessages
    {
        public static class ToClient
        {
            const string k_Base = "CompanionApp_ToClient_";

            public const string Initialize = k_Base + "Initialize";
            public const string IsRecordingChanged = k_Base + "IsRecordingChanged";
            public const string DeviceModeChanged = k_Base + "DeviceModeChanged";
            public const string FrameRate = k_Base + "FrameRate";
            public const string HasSlateChanged = k_Base + "HasSlateChanged";
            public const string SlateDurationChanged = k_Base + "SlateDurationChanged";
            public const string SlateIsPreviewingChanged = k_Base + "SlateIsPreviewingChanged";
            public const string SlatePreviewTimeChanged = k_Base + "SlatePreviewTimeChanged";
            public const string SlateSelectedTake = k_Base + "SlateSelectedTake";
            public const string SlateIterationBase = k_Base + "SlateIterationBase";
            public const string SlateTakes_V0 = k_Base + "SlateTakes_V0";
            public const string NextTakeName = k_Base + "NextTakeName";
            public const string NextAssetName = k_Base + "NextAssetName";
            public const string TexturePreview = k_Base + "TexturePreview";
        }

        public static class ToServer
        {
            const string k_Base = "CompanionApp_ToServer_";

            public const string SetDeviceMode = k_Base + "SetDeviceMode";
            public const string StartRecording = k_Base + "StartRecording";
            public const string StopRecording = k_Base + "StopRecording";
            public const string PlayerStart = k_Base + "PlayerStart";
            public const string PlayerStop = k_Base + "PlayerStop";
            public const string PlayerPause = k_Base + "PlayerPause";
            public const string PlayerSetTime = k_Base + "PlayerSetTime";
            public const string SetSelectedTake = k_Base + "SetSelectedTake";
            public const string SetTakeData_V0 = k_Base + "SetTakeData_V0";
            public const string DeleteTake = k_Base + "DeleteTake";
            public const string SetIterationBase = k_Base + "SetIterationBase";
            public const string ClearIterationBase = k_Base + "ClearIterationBase";
            public const string RequestTexturePreview = k_Base + "RequestTexturePreview";
        }
    }
}
