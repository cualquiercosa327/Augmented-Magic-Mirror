// Main.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "AugmentedMagicMirror.h"

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	AugmentedMagicMirror App(hInstance);

	return App.Run(nCmdShow);
}
