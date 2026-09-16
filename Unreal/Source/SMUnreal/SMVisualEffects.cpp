#include "SMVisualEffects.h"
void FSMVisualEffects::Add(FVector2f P,FVector2f V,FLinearColor C,float R,float Life,float Strength,int Kind,float FloorY) {
    if(Particles.Num()>=63) {
        int Oldest=0;float Priority=MAX_flt;
        for(int I=0;I<Particles.Num();I++) {
            const auto& Existing=Particles[I];
            float Remaining=(1-Existing.Age/Existing.Life)*Existing.Strength*(Existing.Kind==1?3.f:1.f);
            if(Remaining<Priority){Oldest=I;Priority=Remaining;}
        }
        Particles.RemoveAt(Oldest);
    }
    FSMVisualParticle Q;Q.Position=P;Q.Velocity=V;Q.Color=C;Q.Radius=R;Q.Life=Life;Q.Strength=Strength;Q.Kind=Kind;Q.FloorY=FloorY;
    Particles.Add(Q);
}
void FSMVisualEffects::Burst(FVector2f P,float Power) {
    ++ExplosionCount;
    Add(P,FVector2f(0,0),FLinearColor(1,.74f,.32f),38*Power,.28f,2.4f,1);
    for(int I=0;I<16;I++) {
        const float A=Random.FRandRange(-PI,PI),Speed=Random.FRandRange(28,110)*Power;
        Add(P,FVector2f(FMath::Cos(A)*Speed,FMath::Sin(A)*Speed),FLinearColor(1,.55f+Random.FRand()*.35f,.18f),
            Random.FRandRange(.6f,1.4f),.4f+Random.FRand()*.5f,1.1f,0);
    }
}
void FSMVisualEffects::Footstep(FVector2f P,int Style,float Facing) {
    ++FootstepCount[Style];
    if(Style==1) {
        ++SplashCount;
        for(int I=0;I<6;I++)Add(P-FVector2f(0,1),
            FVector2f(Random.FRandRange(-26,26)-Facing*5,Random.FRandRange(-40,-18)),
            FLinearColor(.34f,.60f,.76f),Random.FRandRange(.45f,.8f),Random.FRandRange(.25f,.45f),.7f,2,P.Y);
        Add(P-FVector2f(0,.7f),FVector2f(0,0),FLinearColor(.23f,.39f,.49f),5,.18f,.3f,2);
        return;
    }
    const FLinearColor Colors[]={FLinearColor(.28f,.25f,.22f),FLinearColor::Black,
        FLinearColor(.22f,.37f,.36f),FLinearColor(.48f,.38f,.22f),
        FLinearColor(.29f,.19f,.14f),FLinearColor(.28f,.32f,.36f)};
    const int Count=Style==5?2:5;
    for(int I=0;I<Count;I++) {
        const float Speed=Style==2?8.f:18.f;
        Add(P+FVector2f(Random.FRandRange(-3,3),-2),
            FVector2f(Random.FRandRange(-Speed,Speed)-Facing*4,Random.FRandRange(-17,-6)),
            Colors[Style],Random.FRandRange(Style==5?.7f:1.2f,Style==5?1.4f:2.8f),
            Random.FRandRange(.28f,Style==2?.85f:.55f),Style==5?.3f:.6f,3,P.Y);
    }
    if(Style==2)for(int I=0;I<2;I++)Add(P-FVector2f(0,3),
        FVector2f(Random.FRandRange(-7,7),Random.FRandRange(-32,-18)),
        FLinearColor(.35f,.60f,.65f),Random.FRandRange(.65f,1.1f),.8f,.5f,4);
}
void FSMVisualEffects::Advance(int (*Read)(int,int),int Room,int Fx,int Water,float Dt) {
    const float X=Read(18,0),Y=Read(19,0);Clock+=Dt;
    if(Room!=LastRoom) {
        Particles.Reset();FMemory::Memzero(PreviousProjectiles);FMemory::Memzero(PreviousTypes);
        BombActive=BombExploding=false;BombTime=SaveGlow=0;LastRoom=Room;
        GrappleActive=ScrewActive=false;ElectricTime=GrappleAge=0;
    }
    for(int I=Particles.Num()-1;I>=0;--I) {
        auto& P=Particles[I];P.Age+=Dt;
        if(P.Age>=P.Life){Particles.RemoveAt(I);continue;}
        P.Position+=P.Velocity*Dt;
        if(P.Kind==4) {
            P.Velocity.X*=FMath::Exp(-2.f*Dt);
            if(Water!=32767 && P.Position.Y<=Water){Particles.RemoveAt(I);continue;}
        } else if(P.Kind!=1)P.Velocity.Y+=(P.Kind==2?110.f:P.Kind==3?18.f:45.f)*Dt;
        if(P.Position.Y>P.FloorY){Particles.RemoveAt(I);continue;}
    }
    // Continuous eye light is anchored in world space, never an HDMA window.
    if(Read(46,0))Add(FVector2f(X,Y),FVector2f::ZeroVector,FLinearColor(1,.015f,.005f),350,.03f,.11f+.06f*FMath::Sin(Clock*5),1);
    if(Read(38,0)) {
        const FVector2f Eye(Read(39,0),Read(40,0));
        FVector2f Direction=FVector2f(X,Y)-Eye;Direction.Normalize();
        Add(Eye,FVector2f::ZeroVector,FLinearColor(1,.78f,.26f),18,.025f,.65f,1);
        for(int J=1;J<=5;J++)Add(Eye+Direction*(J*18),FVector2f::ZeroVector,
            FLinearColor(.8f,.67f,.24f),6+J*5,.025f,.20f*(1-J*.12f),1);
    }
    if(SaveGlow>0) {
        SaveGlow=FMath::Max(0.f,SaveGlow-Dt);SaveArcClock+=Dt;
        Add(SavePosition,FVector2f::ZeroVector,FLinearColor(.22f,.65f,1),40,.03f,.65f*FMath::Min(SaveGlow,1.f),1);
        if(SaveArcClock>.035f) {
            SaveArcClock=0;
            for(int J=0;J<4;J++)Add(SavePosition+FVector2f(Random.FRandRange(-14,14),Random.FRandRange(-22,18)),
                FVector2f(Random.FRandRange(-15,15),Random.FRandRange(-30,-12)),FLinearColor(.55f,.82f,1),.8f,.16f,1.5f,0);
        }
    }
    const float Facing=Read(3,0)==4?-1.f:1.f;
    const bool Boosting=Read(47,0)!=0;
    if(Boosting) {
        ++SpeedFrames;
        Add(FVector2f(X,Y),FVector2f::ZeroVector,FLinearColor(.15f,.4f,1),28,.025f,.45f,1);
        // Short jagged arcs follow the original sprite; blue sparks trail it.
        if((SpeedFrames&1)==0) {
            const float Side=(SpeedFrames&2)?1.f:-1.f;
            for(int J=0;J<7;J++)Add(FVector2f(X+Side*(8+(J&1)*5),Y-20+J*6),
                FVector2f(-Facing*24,-6),FLinearColor(.45f,.75f,1),.7f,.06f,1.4f,0);
            Add(FVector2f(X-Facing*12,Y+Random.FRandRange(-16,16)),FVector2f(-Facing*65,-12),
                FLinearColor(.35f,.65f,1),.8f,.20f,1.1f,0);
        }
    }
    CannonPosition=FVector2f(X+Facing*12,Y-6);
    if(Read(24,0))CannonPosition=FVector2f(Read(22,0),Read(23,0));
    ChargeStrength=FMath::Clamp((Read(4,0)-10)/50.f,0.f,1.f);
    const bool Grappling=Read(34,0)!=0,Spinning=Read(35,0)!=0;
    GrappleStart=FVector2f(Read(30,0),Read(31,0));GrappleTip=FVector2f(Read(32,0),Read(33,0));
    ScrewPosition=FVector2f(X,Y);
    if(Grappling && !GrappleActive) {
        ++GrappleCount;GrappleAge=0;
        Add(GrappleStart,FVector2f(0,0),FLinearColor(.6f,.8f,1),22,.14f,1.8f,1);
    }
    if(Spinning && !ScrewActive)++ScrewCount;
    GrappleActive=Grappling;ScrewActive=Spinning;GrappleAge+=Dt;ElectricTime+=Dt;
    if(ElectricTime>=.035f) {
        ElectricTime=0;
        if(Grappling)Add(GrappleTip,FVector2f(Random.FRandRange(-32,32),Random.FRandRange(-36,12)),
            FLinearColor(.6f,.8f,1),.7f,.22f,1.1f,0);
        if(Spinning) {
            const float A=Clock*27;
            const FVector2f D(FMath::Cos(A),FMath::Sin(A));
            Add(ScrewPosition+D*14,D*24,FLinearColor(1,.85f,.32f),.85f,.28f,1.25f,0);
        }
    }
    for(int I=0;I<10;I++) {
        const int Active=Read(5,I),Type=Read(8,I)&0xf00;
        const FVector2f P(Read(6,I),Read(7,I));
        if(Active && !PreviousProjectiles[I] && Type<0x300 && FVector2f::Distance(P,FVector2f(X,Y))<50) {
            ++MuzzleCount;
            Add(P,FVector2f(0,0),FLinearColor(.50f,.85f,1.f),22,.10f,.9f,1);
            for(int K=0;K<2;K++)Add(P,FVector2f(Facing*Random.FRandRange(20,50),Random.FRandRange(-12,12)),FLinearColor(.65f,.85f,1.f),.55f,.12f,.7f,0);
        }
        if(Active && (Type==0x700 || Type==0x800) && PreviousTypes[I]!=Type)Burst(P,1);
        PreviousProjectiles[I]=Active;PreviousTypes[I]=Active?Type:0;
    }
    for(int I=0;I<Read(14,0);I++) {
        const int Kind=Read(17,I);const FVector2f P(Read(15,I),Read(16,I));
        if(Kind==142){++SaveBursts;SavePosition=P;SaveGlow=2.4f;SaveArcClock=0;}
        else if(Kind==144){++SpeedImpacts;Add(P,FVector2f::ZeroVector,FLinearColor(.5f,.8f,1),48,.18f,2.2f,1);}
        else if(Kind==140) {
            ++RelicBursts;
            Add(P,FVector2f(0,0),FLinearColor(.16f,.85f,.72f),12,.45f,.65f,1);
            for(int J=0;J<7;J++) {
                const float A=Random.FRandRange(-PI,PI);
                Add(P,FVector2f(FMath::Cos(A)*14,FMath::Sin(A)*14-10),
                    FLinearColor(.35f,1,.82f),.55f,Random.FRandRange(.35f,.7f),.75f,0);
            }
        }
        else if(Kind==143)Burst(P,1.7f);
        else if(Kind==141) {
            ++RelicGlowFrames;
            // Short-lived light events do not accumulate a large bloom halo.
            Add(P,FVector2f(0,0),FLinearColor(.12f,.7f,.57f),9,.025f,.18f,1);
            if(Random.FRand()<Dt*1.5f)Add(P+FVector2f(Random.FRandRange(-5,5),2),
                FVector2f(Random.FRandRange(-2,2),-14),FLinearColor(.3f,.85f,.72f),.4f,.4f,.4f,0);
        }
        else if(Kind>=129 && Kind<=134)Footstep(P,Kind-129,Facing);
        else if(Kind==128) {++EnemyCount;Burst(P,1.35f);}
        else if(Kind==3 || Kind==21 || Kind==24 || Kind==29 || Kind==61) {
            bool DeathHere=false;
            for(int J=0;J<Read(14,0);J++)if(Read(17,J)==128 && FVector2f::Distance(P,FVector2f(Read(15,J),Read(16,J)))<8)DeathHere=true;
            if(!DeathHere)Burst(P,Kind==21?1.25f:.85f);
        }
        else if(Kind==6 || Kind==55)Add(P,FVector2f(0,0),FLinearColor(1,.70f,.30f),15,.10f,.65f,1);
    }
    const bool Active=(Read(13,0)&0x8000)!=0;
    const float Radius=Read(9,0)/256.f*1.65f;
    const bool Exploding=Active && Radius>0;
    if(Exploding && !BombExploding) {
        BombTime=0;++PowerBombCount;
        const FVector2f Origin(Read(11,0),Read(12,0));
        for(int I=0;I<24;I++) {
            const float A=I*2*PI/24,Speed=Random.FRandRange(100,240);
            Add(Origin,FVector2f(FMath::Cos(A)*Speed,FMath::Sin(A)*Speed),FLinearColor(1,.86f,.45f),1.1f,.85f,1.8f,0);
        }
    }
    else if(Active)BombTime+=Dt;
    else BombTime=0;
    BombActive=Active;BombExploding=Exploding;BombRadius=Radius;
    BombPosition=FVector2f(Read(11,0),Read(12,0));
}
void FSMVisualEffects::WriteTexture(TArray<FVector4f>& Out,float CameraX,float CameraY) const {
    Out.SetNumZeroed(128);FMemory::Memzero(Out.GetData(),128*sizeof(FVector4f));
    int I=0;
    if(ChargeStrength>0) {
        const float Pulse=.95f+.05f*FMath::Sin(Clock*7);
        Out[I]=FVector4f(CannonPosition.X-CameraX,CannonPosition.Y-CameraY,12+40*ChargeStrength,ChargeStrength*.6f*Pulse);
        Out[64+I]=FVector4f(.20f,.65f,1.f,1);I++;
    }
    for(const auto& P:Particles) {
        if(I>=64)break;
        const float Fade=FMath::Square(1-P.Age/P.Life);
        Out[I]=FVector4f(P.Position.X-CameraX,P.Position.Y-CameraY,P.Radius,P.Strength*Fade);
        Out[64+I]=FVector4f(P.Color.R,P.Color.G,P.Color.B,P.Kind);I++;
    }
}
FLinearColor FSMVisualEffects::Charge(float X,float Y) const {return FLinearColor(CannonPosition.X-X,CannonPosition.Y-Y,ChargeStrength,Clock);}
FLinearColor FSMVisualEffects::PowerBomb(float X,float Y) const {return FLinearColor(BombPosition.X-X,BombPosition.Y-Y,BombRadius,BombTime);}
FLinearColor FSMVisualEffects::PowerBombPhase() const {return FLinearColor(BombActive?1:0,BombExploding?1:0,0,0);}
FLinearColor FSMVisualEffects::Grapple(float X,float Y) const {return FLinearColor(GrappleStart.X-X,GrappleStart.Y-Y,GrappleActive?1:0,Clock);}
FLinearColor FSMVisualEffects::GrappleEnd(float X,float Y) const {return FLinearColor(GrappleTip.X-X,GrappleTip.Y-Y,GrappleAge,0);}
FLinearColor FSMVisualEffects::Screw(float X,float Y) const {return FLinearColor(ScrewPosition.X-X,ScrewPosition.Y-Y,ScrewActive?1:0,Clock);}
