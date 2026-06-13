// dllmain.cpp : Defines the entry point for the DLL application.

#include <Framework.h>

using namespace SDK;
using namespace Hooking;

bool bOnce = false;

APavlovPawn* TargetPlayer = nullptr;
float ClosestDist = FLT_MAX;
float BestScore = 0;

FRotator OriginalGunRot;
FVector OriginalGunLoc;

typedef void( __fastcall* PostRender )( UGameViewportClient*, UCanvas* );
PostRender PostRenderOriginal = nullptr;

auto PostRenderHook( UGameViewportClient* GameViewportClient, UCanvas* Canvas ) -> void __stdcall {
    UWorld* World = nullptr;
    UGameInstance* OwningGameInstance = nullptr;
    ULocalPlayer* LocalPlayer = nullptr;
    APlayerController* PlayerController = nullptr;
    APavlovPawn* LocalPavPawn = nullptr;
    APavlovPlayerController* LocalPavController = nullptr;
    APavlovGameState* PavGameState = nullptr;
    TArray<AActor*> Actors;
    int32 LocalTeamId = 0;
    FVector MyPos;
    bool bIsTeamMode;

    if ( !GameViewportClient || !Canvas ) goto exit;

    World = SDK::UWorld::GetWorld( );
    if ( !World ) goto exit;

    OwningGameInstance = World->OwningGameInstance;
    if ( !OwningGameInstance ) goto exit;
    if ( OwningGameInstance->LocalPlayers.Num( ) == 0 ) goto exit;

    LocalPlayer = OwningGameInstance->LocalPlayers[0];
    if ( !LocalPlayer ) goto exit;

    PlayerController = LocalPlayer->PlayerController;
    if ( !PlayerController ) goto exit;

    LocalPavPawn = ( APavlovPawn* ) PlayerController->AcknowledgedPawn;
    if ( !LocalPavPawn ) goto draw_text;

    MyPos = LocalPavPawn->GetNavAgentLocation( );
    if ( MyPos.IsZero( ) ) goto draw_text;

    LocalPavController = ( APavlovPlayerController* ) PlayerController;
    if ( !LocalPavController ) goto draw_text;

    PavGameState = ( APavlovGameState* ) World->GameState;
    if ( !PavGameState ) goto draw_text;

    Actors = World->PersistentLevel->Actors;

    TargetPlayer = nullptr;
    ClosestDist = DBL_MAX;
    BestScore = 360.f;

    for ( AActor* Actor : Actors ) {
        if ( !Actor ) continue;

        __try {

            if ( Actor->IsA( AGun_Base_C::StaticClass( ) ) ) {
                AGun_Base_C* Gun = ( AGun_Base_C* ) LocalPavPawn->GetItemOfClass( AGun_Base_C::StaticClass( ), false, true );
                if ( Gun ) {
                    OriginalGunRot = Gun->K2_GetActorRotation( );
                    OriginalGunLoc = Gun->K2_GetActorLocation( );

                    Gun->BulletSpraySpread = 0;
                    Gun->RecoilTraslationMul = 0;
                    Gun->RecoilMul = 0;
                    Gun->RecoilAngleMul = 0;
                }
            }

            if ( Actor->IsA( AVRGun::StaticClass( ) ) ) {
                AVRGun* Gun = ( AVRGun* ) LocalPavPawn->GetItemOfClass( AVRGun::StaticClass( ), false, true );
                if ( Gun ) {
                    OriginalGunRot = Gun->K2_GetActorRotation( );
                    OriginalGunLoc = Gun->K2_GetActorLocation( );

                    Gun->BulletSpraySpread = 0;
                    Gun->RecoilTraslationMul = 0;
                    Gun->RecoilMul = 0;
                    Gun->RecoilAngleMul = 0;
                }
            }

            if ( Actor->IsA( APavlovPawn::StaticClass( ) ) ) {
                auto Player = ( APavlovPawn* ) Actor;
                if ( !Player ) continue;

                if ( !Player->Avatar || !Player->Avatar->bHasValidBodies ) continue;
                if ( Player->IsDead( ) ) continue;

                if ( PavGameState->GameModeType != EPavlovGameModeType::TTT &&
                    PavGameState->GameModeType != EPavlovGameModeType::Deathmatch &&
                    PavGameState->GameModeType != EPavlovGameModeType::LastManStanding &&
                    PavGameState->GameModeType != EPavlovGameModeType::GunGame &&
                    PavGameState->GameModeType != EPavlovGameModeType::WW2GunGame )
                    if ( Player->TeamId == LocalPavPawn->TeamId ) continue;

                if ( Player->Avatar && Player->Avatar->bHasValidBodies && Player->XRayMaterialTeam1 )
                    Player->Avatar->SetMaterial( 0, Player->XRayMaterialTeam1 );

                { // targeting
                    AGun_Base_C* Gun = ( AGun_Base_C* ) LocalPavPawn->GetItemOfClass( AGun_Base_C::StaticClass( ), false, true );
                    AVRGun* Gun2 = ( AVRGun* ) LocalPavPawn->GetItemOfClass( AVRGun::StaticClass( ), false, true );
                    if ( Gun ) {
                        FVector HeadPos = Player->Avatar->SkeletalMesh->FindSocket( Player->AvatarSkin->SkullSocket )->GetSocketLocation( Player->Avatar );
                        FRotator ToPlayerRot = UKismetMathLibrary::FindLookAtRotation( Gun->K2_GetActorLocation( ), HeadPos );
                        FRotator DeltaRot = ToPlayerRot - OriginalGunRot;
                        DeltaRot.Normalize( );

                        float PitchDiff = fabs( DeltaRot.Pitch );
                        float YawDiff = fabs( DeltaRot.Yaw );

                        if ( PitchDiff > 3 || YawDiff > 3 )
                            continue;

                        float Score = PitchDiff + YawDiff;

                        if ( Score < BestScore ) {
                            BestScore = Score;
                            TargetPlayer = Player;
                        }
                    } else if ( Gun2 ) {
                        FVector HeadPos = Player->Avatar->SkeletalMesh->FindSocket( Player->AvatarSkin->SkullSocket )->GetSocketLocation( Player->Avatar );
                        FRotator ToPlayerRot = UKismetMathLibrary::FindLookAtRotation( Gun2->K2_GetActorLocation( ), HeadPos );
                        FRotator DeltaRot = ToPlayerRot - OriginalGunRot;
                        DeltaRot.Normalize( );

                        float PitchDiff = fabs( DeltaRot.Pitch );
                        float YawDiff = fabs( DeltaRot.Yaw );

                        if ( PitchDiff > 3 || YawDiff > 3 )
                            continue;

                        float Score = PitchDiff + YawDiff;

                        if ( Score < BestScore ) {
                            BestScore = Score;
                            TargetPlayer = Player;
                        }
                    }
                }

                { // silent and bullet tp
                    AGun_Base_C* PavGun = ( AGun_Base_C* ) LocalPavPawn->GetItemOfClass( AGun_Base_C::StaticClass( ), false, true );
                    AVRGun* VRGun = ( AVRGun* ) LocalPavPawn->GetItemOfClass( AVRGun::StaticClass( ), false, true );
                    if ( PavGun ) {
                        if ( TargetPlayer && TargetPlayer->Avatar && TargetPlayer->Avatar->bHasValidBodies ) {
                            auto Head = TargetPlayer->Avatar->SkeletalMesh->FindSocket( TargetPlayer->AvatarSkin->SkullSocket );
                            if ( Head ) {
                                FVector HeadPos = Head->GetSocketLocation( TargetPlayer->Avatar );

                                FVector MuzzlePos = PavGun->Mesh->GetSocketLocation( PavGun->MuzzleSocket );
                                FVector GunPos = PavGun->K2_GetActorLocation( );

                                FVector Offset = GunPos - MuzzlePos;

                                FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation( 
                                    PavGun->Mesh->GetSocketLocation( PavGun->MuzzleSocket ),
                                    FVector( HeadPos.X, HeadPos.Y, HeadPos.Z - 5 ) 
                                );

                                PavGun->K2_SetActorRotation( LookAtRot, false );

                                //FVector NewGunPos = HeadPos + Offset - ( LookAtRot.Vector( ) * 12.f );
                                //PavGun->K2_SetActorLocation( NewGunPos, false, nullptr, true );
                            }
                        }
                        else {
                            if ( !OriginalGunRot.IsZero( ) ) {
                                PavGun->K2_SetActorRotation( OriginalGunRot, false );
                                PavGun->K2_SetActorLocation( OriginalGunLoc, false, nullptr, true );
                            }
                        }
                    } else if ( VRGun ) {
                        if ( TargetPlayer && TargetPlayer->Avatar && TargetPlayer->Avatar->bHasValidBodies ) {
                            auto Head = TargetPlayer->Avatar->SkeletalMesh->FindSocket( TargetPlayer->AvatarSkin->SkullSocket );
                            if ( Head ) {
                                FVector HeadPos = Head->GetSocketLocation( TargetPlayer->Avatar );

                                FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation( 
                                    VRGun->K2_GetActorLocation( ),
                                    FVector( HeadPos.X, HeadPos.Y, HeadPos.Z - 5 )
                                );

                                VRGun->K2_SetActorRotation( LookAtRot, false );
                            }
                        }
                        else {
                            if ( !OriginalGunRot.IsZero( ) ) {
                                VRGun->K2_SetActorRotation( OriginalGunRot, false );
                            }
                        }
                    }
                }

                if ( TargetPlayer && TargetPlayer->Avatar && TargetPlayer->Avatar->bHasValidBodies && TargetPlayer->XRayMaterialTeam0 && !TargetPlayer->IsDead( ) )
                    TargetPlayer->Avatar->SetMaterial( 0, TargetPlayer->XRayMaterialTeam0 );
            }
        }
        __except ( EXCEPTION_EXECUTE_HANDLER ) {
            continue;
        }
    }

draw_text:
    __try {
        Canvas->K2_DrawText(
            SDK::UEngine::GetEngine( )->MediumFont,
            FString( TEXT( "pavlov hack 1337" ) ), // sorry for the shitty code
            FVector2D( 15, 500 ),
            FVector2D( 1.5f, 1.5f ),
            FLinearColor( 1.f, 1.f, 1.f, 1.f ),
            1.0f,
            FLinearColor( 1.f, 1.f, 1.f, 0.f ),
            FVector2D( 1.f, 1.f ),
            false,
            true,
            true,
            FLinearColor( 1.f, 1.f, 1.f, 0.f )
        );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {
    }

exit:
    PostRenderOriginal( GameViewportClient, Canvas );
}

auto get_module_size( DWORD64 base ) -> DWORD {
    IMAGE_DOS_HEADER dos_header = { 0 };
    IMAGE_NT_HEADERS nt_headers = { 0 };

    if ( !base )
        return -1;

    dos_header = *reinterpret_cast< IMAGE_DOS_HEADER* >( base );
    nt_headers = *reinterpret_cast< IMAGE_NT_HEADERS* >( base + dos_header.e_lfanew );
    return nt_headers.OptionalHeader.SizeOfImage;
}

typedef struct {
    DWORD64 entry_point;
    void* param;
} call_function_t, * pcall_function_t;

typedef DWORD( *function_t )( VOID* p );

auto function_thread( pcall_function_t pcf ) -> PVOID WINAPI {
    if ( pcf != NULL && pcf->entry_point != NULL ) {
        function_t function = reinterpret_cast< function_t >( pcf->entry_point );
        function( pcf->param );
    }
    return NULL;
}

auto create_thread( LPTHREAD_START_ROUTINE start_address, LPVOID parameter, LPDWORD thread_id ) -> HANDLE {
    HMODULE ntdll = ( GetModuleHandleA ) ( "ntdll.dll" );
    if ( ntdll ) {
        DWORD image_size = get_module_size( reinterpret_cast< DWORD64 >( ntdll ) );
        BYTE* memory_data = reinterpret_cast< BYTE* >( ntdll ) + image_size - 0x400;

        if ( memory_data ) {
            DWORD protect;
            ( VirtualProtect ) ( memory_data, 0x100, PAGE_EXECUTE_READWRITE, &protect );

            call_function_t* pcf = reinterpret_cast< call_function_t* >( VirtualAlloc( NULL, 0x100, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE ) );

            pcf->entry_point = reinterpret_cast< DWORD64 >( start_address );
            pcf->param = parameter;

            ( memcpy ) ( memory_data, reinterpret_cast< void* >( function_thread ), 0x100 );

            HANDLE handle = CreateRemoteThread( GetCurrentProcess( ), NULL, 0, reinterpret_cast< LPTHREAD_START_ROUTINE >( memory_data ), pcf, 0, thread_id );
            return handle;
        }
    }
    return 0;
}

auto main_thread( HMODULE module ) -> DWORD {
    AllocConsole( );
    FILE* f;
    freopen_s( &f, "CONIN$", "r", stdin );
    freopen_s( &f, "CONOUT$", "w", stdout );
    freopen_s( &f, "CONOUT$", "w", stderr );

    ViewportClientHook->VMT( SDK::UEngine::GetEngine( )->GameViewport, PostRenderHook, 0x6D, &PostRenderOriginal );

    return 0;
}

auto DllMain( HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
) -> BOOL APIENTRY {
    switch ( ul_reason_for_call ) {
    case DLL_PROCESS_ATTACH:
        create_thread( ( LPTHREAD_START_ROUTINE ) main_thread, hModule, 0 );
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}