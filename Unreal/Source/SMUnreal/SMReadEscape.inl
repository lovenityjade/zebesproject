// Read typed, value-owned escape plans without mutating native pending slots.
static bool ReadEscapeContract(const TSharedPtr<FJsonObject>& Data,FSMSeedPlan& Out,FString& Error){
    auto Fail=[&](){Error=TEXT("Invalid native escape clock or routing contract");return false;};
    auto Word=[](const TSharedPtr<FJsonObject>& O,const TCHAR* Key,uint16& Dest){double N=0;
        if(!O->TryGetNumberField(Key,N) || N<0 || N>65535 || N!=FMath::FloorToDouble(N))return false;
        Dest=uint16(N);return true;};
    auto Bcd=[](uint16 N){return (N&15)<10 && ((N>>4)&15)<6 && ((N>>8)&15)<10 && (N>>12)<10;};
    double Schema=0;const TSharedPtr<FJsonObject> *Clock=nullptr,*Routing=nullptr;
    if(!Data->TryGetNumberField(TEXT("schema"),Schema) || Schema!=1 || !Data->TryGetObjectField(TEXT("clock"),Clock) || !Data->TryGetObjectField(TEXT("routing"),Routing))return Fail();
    auto& C=Out.EscapeClock;auto& R=Out.EscapeRouting;
    if(!(*Clock)->TryGetNumberField(TEXT("schema"),Schema) || Schema!=1 || !Word(*Clock,TEXT("flags"),C.flags) || C.flags>7 || (C.flags&3)==3 ||
       !Word(*Clock,TEXT("timer"),C.timer) || !Word(*Clock,TEXT("halfTimer"),C.half_timer) || !Bcd(C.timer) || !Bcd(C.half_timer) ||
       bool(C.flags&1)!=(C.timer==0) || C.half_timer>C.timer)return Fail();
    const TArray<TSharedPtr<FJsonValue>> *Timers=nullptr,*Halves=nullptr;
    if(!(*Clock)->TryGetArrayField(TEXT("timers"),Timers) || Timers->Num()!=10 || !(*Clock)->TryGetArrayField(TEXT("halfTimers"),Halves) || Halves->Num()!=10)return Fail();
    for(int32 I=0;I<10;I++){double T=0,H=0;
        if(!(*Timers)[I]->TryGetNumber(T) || !(*Halves)[I]->TryGetNumber(H) || T<0 || T>65535 || H<0 || H>T || T!=FMath::FloorToDouble(T) || H!=FMath::FloorToDouble(H) ||
           !Bcd(uint16(T)) || !Bcd(uint16(H)) || ((C.flags&1)?T==0:(T!=0 || H!=0)))return Fail();
        C.timers[I]=uint16(T);C.half_timers[I]=uint16(H);
    }
    const TArray<TSharedPtr<FJsonValue>>* Pairs=nullptr;
    if(!(*Routing)->TryGetNumberField(TEXT("schema"),Schema) || Schema!=1 || !(*Routing)->TryGetStringField(TEXT("catalogSha256"),Out.EscapeCatalog) || Out.EscapeCatalog!=NativeEscapeCatalog ||
       !Word(*Routing,TEXT("animals"),R.animals) || R.animals>4 || !(*Routing)->TryGetArrayField(TEXT("pairs"),Pairs) || Pairs->Num()<6 || Pairs->Num()>16)return Fail();
    TSet<int32> Doors;int32 TourianSources=0;
    for(int32 I=0;I<Pairs->Num();I++){
        const TArray<TSharedPtr<FJsonValue>>* Pair=nullptr;
        if(!(*Pairs)[I]->TryGetArray(Pair) || Pair->Num()!=2)return Fail();
        for(int32 J=0;J<2;J++){double N=0;if(!(*Pair)[J]->TryGetNumber(N) || N<0 || N>=UE_ARRAY_COUNT(NativeEscapeEndpoints) || N!=FMath::FloorToDouble(N))return Fail();R.pairs[I][J]=uint8(N);}
        Doors.Add(NativeEscapeDoors[R.pairs[I][0]]);TourianSources+=R.pairs[I][0]==0;
    }
    for(int32 Door:{0xadac,0xadc4,0xaddc,0xadf4,0xae00})if(!Doors.Contains(Door))return Fail();
    if(TourianSources!=1)return Fail();
    C.version=R.version=1;C.size=sizeof(C);R.size=sizeof(R);R.count=Pairs->Num();return true;
}
