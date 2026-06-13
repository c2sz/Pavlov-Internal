#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#define M_PI 3.14159265358979323846

// Header Files
#include <Windows.h>
#include <numbers>
#include <fstream>
#include <chrono>
#include <algorithm>

#include <Memory/VirtualTable.hpp>
#include <SDK/SDK.hpp>

#include <MinHook.h>
#pragma comment(lib, "minhook.lib")

SDK::FString StringToFString( const std::string& Input ) {
    std::wstring wstr( Input.begin( ), Input.end( ) );
    return SDK::FString( wstr.c_str( ) );
}

SDK::FName CreateFNameFromString( const std::string& Name ) {
    SDK::FName::InitInternal( );

    SDK::FName NewName;
    SDK::FString TempFString = StringToFString( Name );

    using AppendStringFn = void( * )( SDK::FString&, SDK::FName* );
    AppendStringFn Func = reinterpret_cast< AppendStringFn >( SDK::FName::AppendString );

    if ( Func ) {
        Func( TempFString, &NewName );
    }
    else {
        NewName = SDK::FName( );
    }

    return NewName;
}

SDK::FVector RotateVectorByQuat( const SDK::FVector& V, const SDK::FQuat& Q ) {
    double X = Q.X;
    double Y = Q.Y;
    double Z = Q.Z;
    double W = Q.W;

    double vx = V.X;
    double vy = V.Y;
    double vz = V.Z;

    double ix = W * vx + Y * vz - Z * vy;
    double iy = W * vy + Z * vx - X * vz;
    double iz = W * vz + X * vy - Y * vx;
    double iw = -X * vx - Y * vy - Z * vz;
    
    SDK::FVector Result;
    Result.X = ix * W + iw * -X + iy * -Z - iz * -Y;
    Result.Y = iy * W + iw * -Y + iz * -X - ix * -Z;
    Result.Z = iz * W + iw * -Z + ix * -Y - iy * -X;

    return Result;
}