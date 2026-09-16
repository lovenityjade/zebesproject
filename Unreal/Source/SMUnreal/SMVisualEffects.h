#pragma once
#include "CoreMinimal.h"
struct FSMVisualParticle {
    FVector2f Position,Velocity;
    FLinearColor Color;
    float Age=0,Life=.2f,Radius=1,Strength=1,FloorY=MAX_flt;
    int Kind=0;
};
/* Presentation state only. Native events and projectile state drive Unreal
 * particles/lights; no damage, collision, item, palette or RNG writes. */
class FSMVisualEffects {
public:
    void Advance(int (*Read)(int,int),int Room,int Fx,int Water,float Dt);
    void WriteTexture(TArray<FVector4f>& Pixels,float CameraX,float CameraY) const;
    FLinearColor Charge(float CameraX,float CameraY) const;
    FLinearColor PowerBomb(float CameraX,float CameraY) const;
    FLinearColor PowerBombPhase() const;
    FLinearColor Grapple(float X,float Y) const;
    FLinearColor GrappleEnd(float X,float Y) const;
    FLinearColor Screw(float X,float Y) const;
    void Reset(){*this=FSMVisualEffects();}
    int GrappleCount=0,ScrewCount=0,EnemyCount=0;
    int RelicBursts=0,RelicGlowFrames=0;
    int SaveBursts=0,SpeedFrames=0,SpeedImpacts=0;
    int SplashCount=0,MuzzleCount=0,ExplosionCount=0,PowerBombCount=0;
    int FootstepCount[6]={};
private:
    TArray<FSMVisualParticle> Particles;
    FRandomStream Random{14092026};
    int LastRoom=-1,PreviousProjectiles[10]={},PreviousTypes[10]={};
    float ChargeStrength=0,SaveGlow=0,SaveArcClock=0;
    FVector2f SavePosition{0,0};
    FVector2f CannonPosition{0,0},BombPosition{0,0};
    bool BombActive=false,BombExploding=false;
    float BombRadius=0,BombTime=0,Clock=0;
    FVector2f GrappleStart{0,0},GrappleTip{0,0},ScrewPosition{0,0};
    bool GrappleActive=false,ScrewActive=false;
    float ElectricTime=0,GrappleAge=0;
    void Add(FVector2f Position,FVector2f Velocity,FLinearColor Color,float Radius,float Life,float Strength,int Kind,float FloorY=MAX_flt);
    void Footstep(FVector2f Position,int Style,float Facing);
    void Burst(FVector2f Position,float Power);
};
