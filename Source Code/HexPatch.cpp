//--------------------------------------------------------------------------------------
// Based of XeUnshackle By Bryrom90
// Authors of HexPatch: Kryptal & Kryptik-Dev
// Contributors: Safauri
//--------------------------------------------------------------------------------------

// Software description:
// A Xbox 360 homebrew application that applies essential patches to enable unsigned 
// code execution and remove various security restrictions. Uses SMBUI which makes the
// software simpler to use and provides an automated experience when using ABadAvatar.
// A SIMPLE SET AND FORGET USER EXPERIENCE.
//--------------------------------------------------------------------------------------


#include "stdafx.h"


FLOAT APP_VERS = 1.02;

// Variables for SMBUI
LPCWSTR buttons[2];
WCHAR dialog_text_buffer[512];
MESSAGEBOX_RESULT result;
XOVERLAPPED overlapped;

//--------------------------------------------------------------------------------------
// Name: Config File Initialization
// Desc: Writes config file at start if it does not exist.    
//--------------------------------------------------------------------------------------

VOID CreateDefaultConfigFile()
{
    HANDLE hFile = CreateFile("GAME:\\config.ini", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        const char* config = "DisableMessageBox=false\r\n";
        DWORD bytesWritten;
        WriteFile(hFile, config, (DWORD)strlen(config), &bytesWritten, NULL);
        CloseHandle(hFile);
    }
}

//--------------------------------------------------------------------------------------
// check if config file Is set to true or false 
//--------------------------------------------------------------------------------------

BOOL IsMessageBoxDisabled()
{
    HANDLE hFile = CreateFile("GAME:\\config.ini", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;  // If for some reason the config file doesn't write at start we automatically assume it is set to false
    }
    
    char buffer[256];
    DWORD bytesRead;
    BOOL isDisabled = FALSE;
    
    if (ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
    {
        buffer[bytesRead] = '\0';
        // if config file is set to true, disable message box
        if (strstr(buffer, "DisableMessageBox=true\r\n") != NULL ||
            strstr(buffer, "DisableMessageBox=true\n") != NULL)
        {
            isDisabled = TRUE;
        }
    }
    
    CloseHandle(hFile);
    return isDisabled;
}

//--------------------------------------------------------------------------------------
// Name: Toggle Message Box Setting
// Desc: Toggles the message box setting through the UI.    
//--------------------------------------------------------------------------------------

VOID ToggleMessageBoxSetting()
{
    BOOL currentlyDisabled = IsMessageBoxDisabled();
    
    HANDLE hFile = CreateFile("GAME:\\config.ini", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        const char* config;
        if (currentlyDisabled)
        {
            // If disabled, clicking "Show Message Box" will enable it
            config = "DisableMessageBox=false\r\n";
            ShowNotify(L"Message Box Will Now Show");
        }
        else
        {
            // If enabled, clicking "Never Show Message Box Again" will disable it
            config = "DisableMessageBox=true\r\n";
            ShowNotify(L"Message Box Will Never Show Again");
            ShowNotify(L"Edit Or Delete The 'config.ini' File To Re-Enable");
        }
        
        DWORD bytesWritten;
        WriteFile(hFile, config, (DWORD)strlen(config), &bytesWritten, NULL);
        CloseHandle(hFile);
    }
}


//--------------------------------------------------------------------------------------
// Name: SMBUI
// Desc: Shows the message box with SMBUI and the options to return to dashboard, 
//       save console info, and toggle message box setting.    
//--------------------------------------------------------------------------------------

void MessageBoxWithOptions(LPCWSTR message)
{
    // Use loop to ensure the SMBUI does not exit unless the user selects "Return to Dashboard"
    for (;;)
    {
        // Dynamic button text based on current setting for the message box toggle
        LPCWSTR thirdButtonText = IsMessageBoxDisabled() ? L"Show Message Box" : L"Never Show Message Box Again";
        LPCWSTR multiButtons[3] = { L"Return to Dashboard", L"Save Console Info", thirdButtonText };
        MESSAGEBOX_RESULT multiResult;
        XOVERLAPPED multiOverlapped;
        
        ZeroMemory(&multiResult, sizeof(multiResult));
        ZeroMemory(&multiOverlapped, sizeof(multiOverlapped));
        
        if (XShowMessageBoxUI(0, L"HexPatch - Beta", message, 3, multiButtons, 0, XMB_WARNINGICON, &multiResult, &multiOverlapped) == ERROR_IO_PENDING)
        {
            while (!XHasOverlappedIoCompleted(&multiOverlapped))
                Sleep(50);  
                //sleep to to clear cpu usage
            
            // Option handling
            if (XGetOverlappedResult(&multiOverlapped, NULL, TRUE) == ERROR_SUCCESS)
            {
                switch (multiResult.dwButtonPressed)
                {
                    case 0: // Return to Dashboard
                        XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
                        return; // Exit function
                    case 1: // Save Console Info
                        SaveConsoleDataToFile();
                        break; // Continue loop to show message box again
                    case 2: // Toggle Message Box Setting
                        ToggleMessageBoxSetting();
                        break; // Continue loop to show message box again
                    default:
                        XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
                        break;
                }
            }
            else
            {
                // If cancelled or error, continue loop to show message box again
                continue;
            }
        }
        else
        {
            // Failed to show message box, continue trying
            Sleep(1000); // Wait a second before retrying
            continue;
        }
    }
}



// Get global access to the main D3D device
extern D3DDevice* g_pd3dDevice;
DWORD YellowText = 0xFFFFFF00;
DWORD WhiteText = 0xFFFFFFFF;
WCHAR wTitleHeaderBuf[100];
WCHAR wCPUKeyBuf[150];
WCHAR wDVDKeyBuf[50];
WCHAR wConTypeBuf[50];

//--------------------------------------------------------------------------------------
// Name: class sample
// Desc: (Not needed for HexPatch, but kept it to prevent breaking the build)
// Could not get it to work without this class
//--------------------------------------------------------------------------------------
class HexPatch : public ATG::Application
{
    ATG::Timer m_Timer;
    ATG::Font m_Font;
    ATG::Help m_Help;
    BOOL m_bDrawHelp;

    BOOL m_bFailed;
    BOOL m_bFinalFailed;

public:
    // Destructor to clean up resources
    ~HexPatch() {}

private:
    virtual HRESULT Initialize();
    virtual HRESULT Update();
    virtual HRESULT Render();
};


//--------------------------------------------------------------------------------------
// Name: Initialize()
// Desc: This creates all device-dependent display objects. 
// (Not needed for HexPatch, but kept it since removing it broke the build)
//--------------------------------------------------------------------------------------
HRESULT HexPatch::Initialize()
{
    // Create the font
    if (FAILED(m_Font.Create("embed:\\FONT")))
        return ATGAPPERR_MEDIANOTFOUND;

    // Confine text drawing to the title safe area
    m_Font.SetWindow(ATG::GetTitleSafeArea());

    m_bFailed = FALSE;
    m_bFinalFailed = FALSE;

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Render()
// Desc: Sets up render states, clears the viewport, and renders the scene.
// (Not needed for HexPatch, but kept it since removing it broke the build)
//--------------------------------------------------------------------------------------
HRESULT HexPatch::Render()
{
    // Present the scene
    ATG::Application::m_pd3dDevice->Present(NULL, NULL, NULL, NULL);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: main()
// Desc: Entry point to the program
// Main patching process
//--------------------------------------------------------------------------------------
VOID __cdecl main()
{
    // Part 1 - We apply the HV patches here (if required)
    if (!Hvx::CheckPPExpHVAccess()) // If we have pp access then assume we have done this previously
    {
        if (!Hvx::DisableExpChecks()) // Stage 1 - Apply HV patches to disable checks on expansions. If this fails do not proceed
        {
            cprintf("[HexPatch] Stage 1 failed!");
            ShowErrorAndExit(1);
        }
        cprintf("[HexPatch] Stage 1 success!");
        if (!Hvx::SetupPPExpHVAccess()) // Stage 2 - Install the peek poke expansion. If this fails do not proceed
        {
            cprintf("[HexPatch] Stage 2 failed!");
            ShowErrorAndExit(2);
        }
        cprintf("[HexPatch] Stage 2 success!");
        if (!Hvx::CheckPPExpHVAccess()) // Stage 3 - Check if we now have HV access via Peek Poke expansion. If this fails do not proceed
        {
            cprintf("[HexPatch] Stage 3 failed!");
            ShowErrorAndExit(3);
        }
        cprintf("[HexPatch] Stage 3 success!");
        if (!ApplyFreebootHVPatches())
        {
            cprintf("[HexPatch] Stage 4 failed!");
            ShowErrorAndExit(4);
        }
        cprintf("[HexPatch] Stage 4 success!");

        // Relaunching before proceeding with Stage 5 is no longer required
        //RelaunchApp();

        // No more relaunching
        cprintf("[HexPatch] Calling KeFlushEntireTb");
        KeFlushEntireTb(); // This is called in XexpTitleTerminateNotification. Maybe this is why relaunching works.

    }
    // Part 2 - We end up here if part 1 succeeded in gaining HV access via expansions
    cprintf("[HexPatch] Checking kernel patch state");
    if (*(DWORD*)0x80108E70 != 0x48003134) // This is the last freeboot kernel patch applied. This determines whether we have applied them yet
    {
        if (!ApplyFreebootKernPatches())
        {
            cprintf("[HexPatch] Stage 5 failed!");
            ShowErrorAndExit(5);
        }
        ApplyAdditionalPatches(); // Other patches for general fixes

        RestoreRoL(); // Restore the default RoL state

        cprintf("[HexPatch] Calling KeFlushEntireTb");
        KeFlushEntireTb();

        cprintf("[HexPatch] Stage 5 success!");
    }
    // Part 3 - We should only ever begin here for any subsequent launches of the app

    // If Dashlaunch loaded successfully we can revert the patches done by BadUpdate. 
    // Needs to be like this due to Dashlaunch fixing Retail signed xex files that have been patched.
    // BadUpdate patches also allow this but prevent the Freeboot patches from functioning correctly
    // IMPORTANT NOTE: Dashlaunch doesn't appear to load the plugins until you exit to dash aka the next executable load.
    // 0 = FAILED
    // 1 = SUCCESS
    // 2 = Already loaded
    
    if (SysLoadDashlaunch() == 1) // We always call this here since it also sets up the wchar buffer to display in the app for Dashlaunch load status
    {

        RevertBadExploitPatches(); // Restore changes made by the exploit
    }

    cprintf("[HexPatch] All patches have been applied! Proceeding to init the ui...");

    // Grab some stuff for display in the ui
    ZeroMemory(wTitleHeaderBuf, sizeof(wTitleHeaderBuf));
    swprintf_s(wTitleHeaderBuf, L"%ls HexPatch v%.2f BETA %ls", GLYPH_RIGHT_TICK, APP_VERS, GLYPH_LEFT_TICK);
    // Motherboard type
    ZeroMemory(wConTypeBuf, sizeof(wConTypeBuf));
    swprintf_s(wConTypeBuf, L"Console type: %S", GetMoboByHWFlags().c_str());
    // cpu key
    QWORD fuse3 = Hvx::HvGetFuseline(3);
    QWORD fuse5 = Hvx::HvGetFuseline(5);
    ZeroMemory(wCPUKeyBuf, sizeof(wCPUKeyBuf));
    swprintf_s(wCPUKeyBuf, L"CPUKey: %08X%08X%08X%08X", fuse3 >> 32, fuse3 & 0xffffffff, fuse5 >> 32, fuse5 & 0xffffffff);
    // dvd key
    BYTE DVDKeyBytes[16];
    QWORD kvAddress = Hvx::HvPeekQWORD(0x00000002000163C0);
    Hvx::HvPeekBytes(kvAddress + 0x100, DVDKeyBytes, 16);
    ZeroMemory(wDVDKeyBuf, sizeof(wDVDKeyBuf));
    swprintf_s(wDVDKeyBuf, L"DVDKey: %08X%08X%08X%08X", *(DWORD*)(DVDKeyBytes), *(DWORD*)(DVDKeyBytes + 4), *(DWORD*)(DVDKeyBytes + 8), *(DWORD*)(DVDKeyBytes + 12));

    BackupOrigMAC(); // This will cause a notify to pop before the video has played completely but only if it hasn't been dumped previously

    // Check if config.ini exists, create with default value if not
    HANDLE hConfigCheck = CreateFile("GAME:\\config.ini", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hConfigCheck == INVALID_HANDLE_VALUE)
    {
        CreateDefaultConfigFile();
    }
    else
    {
        CloseHandle(hConfigCheck);
    }

    // --------------------------------------------------------------
    // Name: SMBUI MAIN CLASS
    // Desc: Shows the notification on exploitation success and the SMBUI if enabled
    // --------------------------------------------------------------

    ShowNotify(L"Exploit Successfull");

    // If message box is enabled, show it immediately
    if (!IsMessageBoxDisabled())
    {
        wsprintfW(dialog_text_buffer, L"DO NOT SIGN IN TO THE \"BadAvatar\" PROFILE\n\nConsole Information:\n\n%s\n%s\n%s", 
                  wConTypeBuf, wCPUKeyBuf, wDVDKeyBuf);
        MessageBoxWithOptions(dialog_text_buffer);
    }
    else
    {
        // If message boxes are disabled, exit to dashboard
        // User must manually edit config file to re-enable message boxes
        XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
    }
}
