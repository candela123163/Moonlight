#include "stdafx.h"
#include "gameApp.h"
#include "util.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    try
    {
        GameApp game(hInstance);
        if (!game.Initialize())
            return 0;

        return game.Run();
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}