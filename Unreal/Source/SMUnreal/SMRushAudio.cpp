#include "SMHUD.h"
#include "SMRom.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

void ASMHUD::LoadBossRushAudio(){
    using ConfigureFn=int(*)(const char*,const char*,const char*);
    auto Configure=reinterpret_cast<ConfigureFn>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_soundtrack_rush_configure")));
    RushAudioRender=reinterpret_cast<void(*)(int16*,int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_soundtrack_rush_render")));
    RushAudioTransition=reinterpret_cast<void(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_soundtrack_rush_transition")));
    RushAudioStatus=reinterpret_cast<uint64(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_soundtrack_rush_status")));
    if(!Configure || !RushAudioRender || !RushAudioStatus)return;
    const FString Storage=SMRom::DataRoot(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("..")))/TEXT("Soundtracks/BossRush");
    auto Path=[&](const TCHAR* Name){
        const FString Local=Storage/Name;
        return IFileManager::Get().FileExists(*Local)?Local:FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("BossRush")/Name);
    };
    const FString Theme=Path(TEXT("Theme.pcm")),Enter=Path(TEXT("WireIn.pcm")),Exit=Path(TEXT("WireOut.pcm"));
    const int Available=Configure(TCHAR_TO_UTF8(*Theme),TCHAR_TO_UTF8(*Enter),TCHAR_TO_UTF8(*Exit));
    UE_LOG(LogTemp,Display,TEXT("SM_RUSH_AUDIO_ASSETS mask=%d theme=%s"),Available,*Theme);
    using ResultsFn=int(*)(const char*,const char*);
    auto Results=reinterpret_cast<ResultsFn>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_soundtrack_rush_results_configure")));
    const FString Failed=Path(TEXT("Failed.pcm")),Success=Path(TEXT("Success.pcm"));
    const int FullMask=Results?Results(TCHAR_TO_UTF8(*Failed),TCHAR_TO_UTF8(*Success)):Available;
    UE_LOG(LogTemp,Display,TEXT("SM_RUSH_RESULT_AUDIO mask=%d failed=%s success=%s looping=0"),FullMask,*Failed,*Success);
    if(!(FullMask&25))return; // Missing/corrupt theme keeps the original native audio.
    RushAudioWave=NewObject<USoundWaveProcedural>(this);
    RushAudioWave->SetSampleRate(44100);RushAudioWave->NumChannels=2;
    RushAudioWave->Duration=INDEFINITELY_LOOPING_DURATION;RushAudioWave->bLooping=false;
    RushAudioWave->SoundGroup=SOUNDGROUP_Music;
    RushAudioComponent=NewObject<UAudioComponent>(this);
    RushAudioComponent->bIsUISound=true;RushAudioComponent->bAllowSpatialization=false;
    RushAudioComponent->SetSound(RushAudioWave);RushAudioComponent->RegisterComponent();
}

void ASMHUD::TickBossRushAudio(){
    if(!RushAudioWave || !RushAudioComponent || !RushAudioStatus)return;
    if(!RushAudioStatus(0)){
        if(RushAudioComponent->IsPlaying())RushAudioComponent->Stop();
        RushAudioWave->ResetAudio();RushAudioCue=-1;return;
    }
    const int Cue=int(RushAudioStatus(3));
    if(Cue!=RushAudioCue){
        RushAudioCue=Cue;RushAudioWave->ResetAudio();
        UE_LOG(LogTemp,Display,TEXT("SM_RUSH_AUDIO_CUE cue=%d looping=%d frames=%llu"),Cue,Cue==0 || Cue==6,RushAudioStatus(5));
    }
    RushAudioComponent->SetVolumeMultiplier(AudioComponent?AudioComponent->VolumeMultiplier:1.f);
    RushAudioComponent->SetPaused(Paused);
    if(Paused)return;
    // A separate real-time queue keeps the theme running through VR/loading,
    // without accelerating it with the simulation or discarding native SFX.
    constexpr int BlockFrames=441,TargetBytes=2646*4;
    int16 Samples[BlockFrames*2];
    int Queued=RushAudioWave->GetAvailableAudioByteCount();
    while(Queued<TargetBytes){
        RushAudioRender(Samples,BlockFrames);
        RushAudioWave->QueueAudio(reinterpret_cast<const uint8*>(Samples),sizeof(Samples));
        Queued+=sizeof(Samples);
    }
    if(!RushAudioComponent->IsPlaying()){
        RushAudioComponent->Play();
        UE_LOG(LogTemp,Display,TEXT("SM_RUSH_AUDIO_START playing=%d"),RushAudioComponent->IsPlaying());
    }
}
