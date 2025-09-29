//--------------------------------------------------------------------------------------
// Based of XeUnshackle By Bryrom90
// Authors of HexPatch: Kryptal & Kryptik-Dev
// Contributors: Dread
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

BOOL bShouldPlaySuccessVid = FALSE;
BOOL bVideoWasPlayed = FALSE; // Track if video was actually played

//--------------------------------------------------------------------------------------
// Name: Config File Initialization
// Desc: Writes config file at start if it does not exist.    
//--------------------------------------------------------------------------------------

VOID CreateDefaultConfigFile()
{
    HANDLE hFile = CreateFile("GAME:\\config.ini", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        const char* config = "DisableSuccessVideo=false\r\nVideoPath=\r\n";
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
// Name: IsSuccessVideoDisabled()
// Desc: Check if success video is disabled in config
//--------------------------------------------------------------------------------------

BOOL IsSuccessVideoDisabled()
{
    HANDLE hFile = CreateFile("GAME:\\config.ini", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;  // By default, video is enabled
    }
    
    char buffer[512];
    DWORD bytesRead;
    BOOL isDisabled = FALSE;
    
    if (ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
    {
        buffer[bytesRead] = '\0';
        // if config file has DisableSuccessVideo=true, disable video
        if (strstr(buffer, "DisableSuccessVideo=true\r\n") != NULL ||
            strstr(buffer, "DisableSuccessVideo=true\n") != NULL)
        {
            isDisabled = TRUE;
        }
    }
    
    CloseHandle(hFile);
    return isDisabled;
}

//--------------------------------------------------------------------------------------
// Name: GetVideoPath()
// Desc: Get the video path from config, or return default
//--------------------------------------------------------------------------------------

BOOL GetCustomVideoPath(CHAR* path, DWORD pathSize)
{
    HANDLE hFile = CreateFile("GAME:\\config.ini", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE; // No config file
    }
    
    char buffer[512];
    DWORD bytesRead;
    BOOL hasCustomPath = FALSE;
    
    if (ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
    {
        buffer[bytesRead] = '\0';
        
        // Look for VideoPath= line
        char* videoPathStart = strstr(buffer, "VideoPath=");
        if (videoPathStart != NULL)
        {
            videoPathStart += 10; // Skip "VideoPath="
            char* videoPathEnd = strchr(videoPathStart, '\r');
            if (videoPathEnd == NULL)
                videoPathEnd = strchr(videoPathStart, '\n');
            if (videoPathEnd == NULL)
                videoPathEnd = buffer + bytesRead;
                
            // Check if there's actually a path specified (not just empty)
            if (videoPathEnd > videoPathStart)
            {
                DWORD length = min((DWORD)(videoPathEnd - videoPathStart), pathSize - 1);
                if (length > 0)
                {
                    // Remove any trailing whitespace
                    while (length > 0 && (videoPathStart[length - 1] == ' ' || videoPathStart[length - 1] == '\t'))
                    {
                        length--;
                    }
                    
                    if (length > 0)
                    {
                        // Handle different path formats and ensure it's in the GAME directory
                        if (strncmp(videoPathStart, "GAME:\\", 6) == 0 || strncmp(videoPathStart, "game:\\", 6) == 0)
                        {
                            // Path already includes GAME:\ prefix
                            strncpy_s(path, pathSize, videoPathStart, length);
                            path[length] = '\0';
                            hasCustomPath = TRUE;
                        }
                        else if (videoPathStart[0] == '/' || videoPathStart[0] == '\\')
                        {
                            // Path starts with / or \, prepend GAME:
                            DWORD gamePrefixLen = 5; // "GAME:"
                            if (length + gamePrefixLen < pathSize)
                            {
                                strcpy_s(path, pathSize, "GAME:");
                                strncat_s(path, pathSize, videoPathStart, length);
                                hasCustomPath = TRUE;
                            }
                        }
                        else if (strncmp(videoPathStart, "./", 2) == 0 || strncmp(videoPathStart, ".\\", 2) == 0)
                        {
                            // Path starts with ./ or .\, prepend GAME:
                            DWORD prefixLen = 6; // "GAME:\\"
                            if (length + prefixLen - 1 < pathSize) // -1 because we skip the dot slash
                            {
                                strcpy_s(path, pathSize, "GAME:\\");
                                strncat_s(path, pathSize, videoPathStart + 2, length - 2); // Skip the ./
                                hasCustomPath = TRUE;
                            }
                        }
                        else
                        {
                            // Path is relative, prepend GAME:
                            DWORD prefixLen = 6; // "GAME:\\"
                            if (length + prefixLen < pathSize)
                            {
                                strcpy_s(path, pathSize, "GAME:\\");
                                strncat_s(path, pathSize, videoPathStart, length);
                                hasCustomPath = TRUE;
                            }
                        }
                    }
                }
            }
        }
    }
    
    CloseHandle(hFile);
    return hasCustomPath;
}

//--------------------------------------------------------------------------------------
// Name: Toggle MessageBox Setting
// Desc: Toggles the message box setting through the UI.    
//--------------------------------------------------------------------------------------

VOID ToggleMessageBoxSetting()
{
    // This function is no longer needed since we always show the message box when Y is pressed
}

//--------------------------------------------------------------------------------------
// Name: ToggleSuccessVideoSetting (patched)
// Desc: Updates only DisableSuccessVideo while preserving other config keys
//--------------------------------------------------------------------------------------
VOID ToggleSuccessVideoSetting()
{
    BOOL currentlyDisabled = IsSuccessVideoDisabled();

    // Read existing config file
    char buffer[512] = {0};
    DWORD bytesRead = 0;
    HANDLE hFile = CreateFile("GAME:\\config.ini", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
        buffer[bytesRead] = '\0';
        CloseHandle(hFile);
    }

    std::string config(buffer);

    // Update or insert DisableSuccessVideo setting
    size_t pos = config.find("DisableSuccessVideo=");
    if (pos != std::string::npos)
    {
        // Replace existing line
        size_t end = config.find_first_of("\r\n", pos);
        if (end == std::string::npos) end = config.length();
        config.replace(pos, end - pos, currentlyDisabled ? "DisableSuccessVideo=false" : "DisableSuccessVideo=true");
    }
    else
    {
        // Add it at the end if missing
        config += (currentlyDisabled ? "\r\nDisableSuccessVideo=false" : "\r\nDisableSuccessVideo=true");
    }

    // Rewrite config file with updated content
    hFile = CreateFile("GAME:\\config.ini", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        WriteFile(hFile, config.c_str(), (DWORD)config.size(), &bytesWritten, NULL);
        CloseHandle(hFile);
    }

    // Show notify message
    if (currentlyDisabled)
        ShowNotify(L"Success Video Enabled");
    else
        ShowNotify(L"Success Video Disabled");
}



//--------------------------------------------------------------------------------------
// Name: ResetD3DStateForMessageBox
// Desc: Resets the D3D device state to a clean state for MessageBox UI
//--------------------------------------------------------------------------------------

void ResetD3DStateForMessageBox()
{
    if (ATG::g_pd3dDevice)
    {
        // Reset render states that might have been changed by XMV player
        ATG::g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        ATG::g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        ATG::g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
        ATG::g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
        ATG::g_pd3dDevice->SetRenderState(D3DRS_VIEWPORTENABLE, TRUE);
        ATG::g_pd3dDevice->SetVertexShader(0);
        ATG::g_pd3dDevice->SetPixelShader(0);
        ATG::g_pd3dDevice->SetVertexDeclaration(0);
        
        // Clear the frame buffer to ensure clean state
        ATG::g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
        ATG::g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
    }
}

//--------------------------------------------------------------------------------------
// Name: SMBUI
// Desc: Shows the message box with SMBUI and the options to return to dashboard, 
//       save console info, and toggle message box setting.    
//--------------------------------------------------------------------------------------

BOOL g_bMessageBoxActive = FALSE;
XOVERLAPPED g_messageBoxOverlapped;
MESSAGEBOX_RESULT g_messageBoxResult;

void MessageBoxWithOptions(LPCWSTR message)
{
    cprintf("[HexPatch] Entering MessageBoxWithOptions\n");
    DWORD retryCount = 0;
    const DWORD maxRetries = 5; // Safety net to prevent infinite loop
    
    // Use loop to ensure the SMBUI does not exit unless the user selects "Return to Dashboard"
    for (;;)
    {
        cprintf("[HexPatch] Attempting to show message box, retry count: %d\n", retryCount);
        // Reset D3D state before showing message box to ensure clean state
        ResetD3DStateForMessageBox();
        
        // Dynamic button text based on current setting for the success video toggle
        LPCWSTR thirdButtonText = IsSuccessVideoDisabled() ? L"Enable Success Video" : L"Disable Success Video";
        LPCWSTR multiButtons[3] = { L"Return to Dashboard", L"Save Console Info", thirdButtonText };
        MESSAGEBOX_RESULT multiResult;
        XOVERLAPPED multiOverlapped;
        
        ZeroMemory(&multiResult, sizeof(multiResult));
        ZeroMemory(&multiOverlapped, sizeof(multiOverlapped));
        
        HRESULT hr = XShowMessageBoxUI(0, L"HexPatch - Beta", message, 3, multiButtons, 0, XMB_WARNINGICON, &multiResult, &multiOverlapped);
        if (hr == ERROR_IO_PENDING)
        {
            cprintf("[HexPatch] XShowMessageBoxUI pending, waiting for completion\n");
            while (!XHasOverlappedIoCompleted(&multiOverlapped))
                Sleep(50);  
                //sleep to to clear cpu usage
            
            // Option handling
            if (XGetOverlappedResult(&multiOverlapped, NULL, TRUE) == ERROR_SUCCESS)
            {
                cprintf("[HexPatch] MessageBox completed successfully\n");
                switch (multiResult.dwButtonPressed)
                {
                    case 0: // Return to Dashboard
                        cprintf("[HexPatch] User selected Return to Dashboard\n");
                        XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
                        return; // Exit function
                    case 1: // Save Console Info
                        cprintf("[HexPatch] User selected Save Console Info\n");
                        SaveConsoleDataToFile();
                        break; // Continue loop to show message box again
                    case 2: // Toggle Success Video Setting
                        cprintf("[HexPatch] User selected Toggle Success Video Setting\n");
                        ToggleSuccessVideoSetting();
                        // Update button text immediately for next loop
                        thirdButtonText = IsSuccessVideoDisabled() ? L"Enable Success Video" : L"Disable Success Video";
                        break; // Continue loop to show message box again
                    default:
                        cprintf("[HexPatch] User selected unknown option, returning to dashboard\n");
                        XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
                        return; // Exit function
                }
            }
            else
            {
                // If cancelled or error, continue loop to show message box again
                cprintf("[HexPatch] MessageBox cancelled or error, retrying\n");
                retryCount++;
                if (retryCount >= maxRetries)
                {
                    cprintf("[HexPatch] MessageBox retry limit reached, exiting to dashboard\n");
                    ShowNotify(L"MessageBox failed - exiting to dashboard");
                    XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
                    return;
                }
                Sleep(50); // Short sleep to avoid hammering the CPU
                continue;
            }
        }
        else
        {
            // Failed to show message box, log error and continue trying
            cprintf("[HexPatch] XShowMessageBoxUI failed: %#X, retrying\n", hr);
            retryCount++;
            if (retryCount >= maxRetries)
            {
                cprintf("[HexPatch] MessageBox retry limit reached, exiting to dashboard\n");
                ShowNotify(L"MessageBox failed - exiting to dashboard");
                XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
                return;
            }
            Sleep(50); // Short sleep to avoid hammering the CPU
            continue;
        }
    }
}

// Get global access to the main D3D device
// extern D3DDevice* g_pd3dDevice; // This is already available through ATG::Application
DWORD YellowText = 0xFFFFFF00;
DWORD WhiteText = 0xFFFFFFFF;
WCHAR wTitleHeaderBuf[100];
WCHAR wCPUKeyBuf[150];
WCHAR wDVDKeyBuf[50];
WCHAR wConTypeBuf[50];

//--------------------------------------------------------------------------------------
// Name: class sample
// Desc: Main class to run this application. Most functionality is inherited
//       from the ATG::Application base class.
//--------------------------------------------------------------------------------------
class HexPatch : public ATG::Application
{
    // Pointer to XMV player object.
    IXMedia2XmvPlayer* m_xmvPlayer;
    // Structure for controlling where the movie is played.
    XMEDIA_VIDEO_SCREEN m_videoScreen;

    // Tell XMV player about scaling and rotation parameters.
    VOID            InitVideoScreen();

    // Buffer for holding XMV data when playing from memory.
    VOID* m_movieBuffer;

    // XAudio2 object.
    IXAudio2* m_pXAudio2;

    ATG::Timer m_Timer;
    ATG::Font m_Font;
    ATG::Help m_Help;
    BOOL m_bDrawHelp;

    BOOL m_bFailed;
    BOOL m_bFinalFailed;
    
    // Flag to track when video has finished and message box should be shown
    BOOL m_bShowMessageBox;
    
    // Flag to track when message box is open
    BOOL m_bMessageBoxOpen;
    
    // Overlapped structure for non-blocking message box
    XOVERLAPPED m_messageBoxOverlapped;
    
    // Result structure for message box
    MESSAGEBOX_RESULT m_messageBoxResult;

public:
    // Destructor to clean up resources
    ~HexPatch() {}
    
    // Override the Run method to handle video completion
    VOID Run();

private:
    virtual HRESULT Initialize();
    virtual HRESULT Update();
    virtual HRESULT Render();
};


//--------------------------------------------------------------------------------------
// Name: Initialize()
// Desc: This creates all device-dependent display objects.
//--------------------------------------------------------------------------------------
HRESULT HexPatch::Initialize()
{
    m_xmvPlayer = 0;
    m_movieBuffer = 0;

    // Initialize the XAudio2 Engine. The XAudio2 Engine is needed for movie playback.
    UINT32 flags = 0;
#ifdef _DEBUG
    flags |= XAUDIO2_DEBUG_ENGINE;
#endif

    HRESULT hr = XAudio2Create(&m_pXAudio2, flags);
    if (FAILED(hr))
        ATG::FatalError("Error %#X calling XAudio2Create\n", hr);

    IXAudio2MasteringVoice* pMasteringVoice = NULL;
    hr = m_pXAudio2->CreateMasteringVoice(&pMasteringVoice);
    if (FAILED(hr))
        ATG::FatalError("Error %#X calling CreateMasteringVoice\n", hr);

    // Create the font
    if (FAILED(m_Font.Create("embed:\\FONT")))
        return ATGAPPERR_MEDIANOTFOUND;

    // Confine text drawing to the title safe area
    m_Font.SetWindow(ATG::GetTitleSafeArea());

    m_bFailed = FALSE;
    m_bFinalFailed = FALSE;
    m_bShowMessageBox = FALSE;
    m_bMessageBoxOpen = FALSE;
    
    // Initialize overlapped structure
    ZeroMemory(&m_messageBoxOverlapped, sizeof(m_messageBoxOverlapped));
    ZeroMemory(&m_messageBoxResult, sizeof(m_messageBoxResult));

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Update()
// Desc: Called once per frame, the call is the entry point for animating the scene.
//       The movie is played from here.
//--------------------------------------------------------------------------------------
HRESULT HexPatch::Update()
{
    // Get the current gamepad state
    ATG::GAMEPAD* pGamepad = ATG::Input::GetMergedInput();

    // If video has finished playing or failed to load, exit the application loop
    if (m_bFinalFailed)
    {
        cprintf("[HexPatch] Video playback finished or failed, setting show message box flag\\n");
        m_bShowMessageBox = TRUE;
        m_bFinalFailed = FALSE; // Reset the flag
        return S_OK; // Continue running the loop
    }

    // Check if message box operation has completed (for Y button during video playback)
    if (m_bMessageBoxOpen && g_bMessageBoxActive && XHasOverlappedIoCompleted(&g_messageBoxOverlapped))
    {
        // Get the result
        if (XGetOverlappedResult(&g_messageBoxOverlapped, NULL, TRUE) == ERROR_SUCCESS)
        {
            // Handle message box result
            switch (g_messageBoxResult.dwButtonPressed)
            {
                case 0: // Return to Dashboard
                    cprintf("[HexPatch] User selected Return to Dashboard\n");
                    XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
                    break;
                case 1: // Save Console Info
                    cprintf("[HexPatch] User selected Save Console Info\n");
                    SaveConsoleDataToFile();
                    // Re-show the message box by resetting the state to allow another show
                    g_bMessageBoxActive = FALSE;
                    ZeroMemory(&g_messageBoxOverlapped, sizeof(g_messageBoxOverlapped));
                    // Keep m_bMessageBoxOpen as TRUE to maintain the black background
                    break;
                case 2: // Toggle Success Video Setting
                    cprintf("[HexPatch] User selected Toggle Success Video Setting\n");
                    ToggleSuccessVideoSetting();
                    // Re-show the message box by resetting the state to allow another show
                    g_bMessageBoxActive = FALSE;
                    ZeroMemory(&g_messageBoxOverlapped, sizeof(g_messageBoxOverlapped));
                    // Keep m_bMessageBoxOpen as TRUE to maintain the black background
                    break;
                default:
                    // For any other case, re-show the message box
                    g_bMessageBoxActive = FALSE;
                    ZeroMemory(&g_messageBoxOverlapped, sizeof(g_messageBoxOverlapped));
                    // Keep m_bMessageBoxOpen as TRUE to maintain the black background
                    break;
            }
        }
        else
        {
            // If there was an error, re-show the message box
            g_bMessageBoxActive = FALSE;
            ZeroMemory(&g_messageBoxOverlapped, sizeof(g_messageBoxOverlapped));
            // Keep m_bMessageBoxOpen as TRUE to maintain the black background
        }
    }

    // If message box is open but not active, show it again with 3 buttons
    if (m_bMessageBoxOpen && !g_bMessageBoxActive)
    {
        // Prepare message box text with console information
        WCHAR dialog_text_buffer[512];
        wsprintfW(dialog_text_buffer, L"Exploit Successful\n\nConsole Information:\n\n%s\n%s\n%s", 
               wConTypeBuf, wCPUKeyBuf, wDVDKeyBuf);
        
        // Reset overlapped structure
        ZeroMemory(&g_messageBoxOverlapped, sizeof(g_messageBoxOverlapped));
        ZeroMemory(&g_messageBoxResult, sizeof(g_messageBoxResult));
        
        // Set the active flag
        g_bMessageBoxActive = TRUE;
        
        // Show message box with 3 buttons
        LPCWSTR thirdButtonText = IsSuccessVideoDisabled() ? L"Enable Success Video" : L"Disable Success Video";
        LPCWSTR buttons[3] = { L"Return to Dashboard", L"Save Console Info", thirdButtonText };
        HRESULT hr = XShowMessageBoxUI(0, L"HexPatch - Beta", dialog_text_buffer, 3, buttons, 0, XMB_WARNINGICON, &g_messageBoxResult, &g_messageBoxOverlapped);
        if (hr == ERROR_IO_PENDING)
        {
            cprintf("[HexPatch] Message box shown asynchronously\n");
        }
        else
        {
            cprintf("[HexPatch] Failed to show message box: %#X\n", hr);
            g_bMessageBoxActive = FALSE;
            // Keep m_bMessageBoxOpen as TRUE to maintain the black background
        }
    }

    if (m_xmvPlayer)
    {
        // 'B' means cancel the movie.
        if (pGamepad->wPressedButtons & XINPUT_GAMEPAD_B)
        {
            m_xmvPlayer->Stop(XMEDIA_STOP_IMMEDIATE);
        }
        
        // 'Y' button to show message box during video playback
        if (pGamepad->wPressedButtons & XINPUT_GAMEPAD_Y)
        {
            if (m_xmvPlayer)
                m_xmvPlayer->Pause(); // Pause the video
            
            // Set flag to show black background and message box
            m_bMessageBoxOpen = TRUE;
            
            // Show message box asynchronously only if not already showing
            if (!g_bMessageBoxActive)
            {
                // Prepare message box text with console information
                WCHAR dialog_text_buffer[512];
                wsprintfW(dialog_text_buffer, L"HexPatch Exploit Successful\n\nConsole Information:\n\n%s\n%s\n%s", 
                       wConTypeBuf, wCPUKeyBuf, wDVDKeyBuf);
                
                // Reset overlapped structure
                ZeroMemory(&g_messageBoxOverlapped, sizeof(g_messageBoxOverlapped));
                ZeroMemory(&g_messageBoxResult, sizeof(g_messageBoxResult));
                
                // Set the active flag
                g_bMessageBoxActive = TRUE;
                
                // Show message box with 3 buttons
                LPCWSTR thirdButtonText = IsSuccessVideoDisabled() ? L"Enable Success Video" : L"Disable Success Video";
                LPCWSTR buttons[3] = { L"Return to Dashboard", L"Save Console Info", thirdButtonText };
                HRESULT hr = XShowMessageBoxUI(0, L"HexPatch - Beta", dialog_text_buffer, 3, buttons, 0, XMB_WARNINGICON, &g_messageBoxResult, &g_messageBoxOverlapped);
                if (hr == ERROR_IO_PENDING)
                {
                    cprintf("[HexPatch] Message box shown asynchronously\n");
                }
                else
                {
                    cprintf("[HexPatch] Failed to show message box: %#X\n", hr);
                    g_bMessageBoxActive = FALSE;
                    m_bMessageBoxOpen = FALSE;
                }
            }
        }
    }
    else
    {
        // Play the movie if required
        if (bShouldPlaySuccessVid)  //(pGamepad->wPressedButtons & XINPUT_GAMEPAD_A)
        {
            cprintf("[HexPatch] Attempting to play success video\n");
            XMEDIA_XMV_CREATE_PARAMETERS XmvParams;

            ZeroMemory(&XmvParams, sizeof(XmvParams));

            // Use the default audio and video streams.
            // If using a wmv file with multiple audio or video streams
            // (such as different audio streams for different languages)
            // the dwAudioStreamId & dwVideoStreamId parameters can be used 
            // to select which audio (or video) stream will be played back

            XmvParams.dwAudioStreamId = XMEDIA_STREAM_ID_USE_DEFAULT;
            XmvParams.dwVideoStreamId = XMEDIA_STREAM_ID_USE_DEFAULT;

            // Play the movie if required
            //if (bShouldPlaySuccessVid)  //(pGamepad->wPressedButtons & XINPUT_GAMEPAD_A)
            //{
            bShouldPlaySuccessVid = FALSE; // Reset so we don't play again
            // Start a movie playing from a file.
            m_bFailed = FALSE;

            // Check if video is disabled
            if (!IsSuccessVideoDisabled())
            {
                cprintf("[HexPatch] Video is not disabled, checking for custom path or embedded video\n");
                
                // Check if there's a custom video path in config
                CHAR videoPath[256];
                BOOL hasCustomPath = GetCustomVideoPath(videoPath, sizeof(videoPath));
                
                if (hasCustomPath)
                {
                    cprintf("[HexPatch] Custom video path found: %s\n", videoPath);
                    // Check if the video file exists before attempting to play it
                    HANDLE hFile = CreateFile(videoPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hFile != INVALID_HANDLE_VALUE)
                    {
                        CloseHandle(hFile);
                        cprintf("[HexPatch] Custom video file exists, attempting to create player\n");
                        
                        // Set the parameters to load the movie from a file on the game drive
                        XmvParams.createType = XMEDIA_CREATE_FROM_FILE;
                        XmvParams.createFromFile.szFileName = videoPath;

                        // Create from file instead of embedded resource
                        HRESULT hr = XMedia2CreateXmvPlayer(m_pd3dDevice, m_pXAudio2, &XmvParams, &m_xmvPlayer);
                        if (SUCCEEDED(hr))
                        {
                            cprintf("[HexPatch] Custom video player created successfully\n");
                            InitVideoScreen();
                            bVideoWasPlayed = TRUE; // Mark that video was played
                        }
                        else
                        {
                            cprintf("[HexPatch] Failed to create custom video player, hr: %#X\n", hr);
                            m_bFailed = TRUE;
                            m_bFinalFailed = TRUE; // Signal completion if video fails to load
                        }
                    }
                    else
                    {
                        cprintf("[HexPatch] Custom video file does not exist, falling back to embedded video\n");
                        // Custom video file doesn't exist, fall back to embedded video
                        goto PlayEmbeddedVideo;
                    }
                }
                else
                {
                    cprintf("[HexPatch] No custom video path, playing embedded video\n");
                    // No custom path specified, play embedded video
                    PlayEmbeddedVideo:
                    // Load embedded video resource using Xbox 360 method
                    VOID* pSectionData;
                    DWORD dwSectionSize;
                    HMODULE hModule = GetModuleHandle(NULL);
                    if (XGetModuleSection(hModule, "VID", &pSectionData, &dwSectionSize))
                    {
                        if (pSectionData && dwSectionSize > 0)
                        {
                            XmvParams.createType = XMEDIA_CREATE_FROM_MEMORY;
                            XmvParams.createFromMemory.pvBuffer = (BYTE*)pSectionData;
                            XmvParams.createFromMemory.dwBufferSize = dwSectionSize;

                            HRESULT hr = XMedia2CreateXmvPlayer(m_pd3dDevice, m_pXAudio2, &XmvParams, &m_xmvPlayer);
                            if (SUCCEEDED(hr))
                            {
                                cprintf("[HexPatch] Embedded video player created successfully\n");
                                InitVideoScreen();
                                bVideoWasPlayed = TRUE; // Mark that video was played
                            }
                            else
                            {
                                cprintf("[HexPatch] Failed to create embedded video player, hr: %#X\n", hr);
                                m_bFailed = TRUE;
                                m_bFinalFailed = TRUE; // Signal completion if video fails to load
                            }
                        }
                        else
                        {
                            cprintf("[HexPatch] Failed to lock embedded video resource\n");
                            m_bFailed = TRUE;
                            m_bFinalFailed = TRUE;
                        }
                    }
                    else
                    {
                        cprintf("[HexPatch] Failed to find embedded video resource\n");
                        m_bFailed = TRUE;
                        m_bFinalFailed = TRUE;
                    }
                }
            }
            else
            {
                cprintf("[HexPatch] Video is disabled, skipping video\n");
                // Video is disabled, skip video and continue with normal flow
                m_bFailed = TRUE;
                m_bFinalFailed = TRUE; // Signal completion to skip video
                bVideoWasPlayed = FALSE; // Mark that video was NOT played
            }
        }
        
        // Check if Y button is pressed to show message box when not playing video
        if (pGamepad->wPressedButtons & XINPUT_GAMEPAD_Y)
        {
            m_bMessageBoxOpen = TRUE;
        }
        
        if (!DisableButtons)
        {
            if (pGamepad->wPressedButtons & XINPUT_GAMEPAD_BACK)
            {
                XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
            }
        }
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Render()
// Desc: Sets up render states, clears the viewport, and renders the scene.
//--------------------------------------------------------------------------------------
HRESULT HexPatch::Render()
{
    // If we are currently playing a movie.
    if (m_xmvPlayer && !m_bMessageBoxOpen)
    {
        // If RenderNextFrame does not return S_OK then the frame was not
        // rendered (perhaps because it was cancelled) so a regular frame
        // buffer should be rendered before calling present.
        HRESULT hr = m_xmvPlayer->RenderNextFrame(0, NULL);

        // Reset our cached view of what pixel and vertex shaders are set, because
        // it is no longer accurate, since XMV will have set their own shaders.
        // This avoids problems when the shader cache thinks it knows what shader
        // is set and it is wrong.
        m_pd3dDevice->SetVertexShader(0);
        m_pd3dDevice->SetPixelShader(0);
        m_pd3dDevice->SetVertexDeclaration(0);

        if (FAILED(hr) || hr == (HRESULT)XMEDIA_W_EOF)
        {
            // Release the movie object
            m_xmvPlayer->Release();
            m_xmvPlayer = 0;
            // Movie playback changes various D3D states, so you should reset the
            // states that you need after movie playback is finished.
            m_pd3dDevice->SetRenderState(D3DRS_VIEWPORTENABLE, TRUE);
            m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
            m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            // m_pd3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE); // Not a valid render state

            // Free up any memory allocated for playing from memory.
            if (m_movieBuffer)
            {
                free(m_movieBuffer);
                m_movieBuffer = 0;
            }
            
            // Signal that video has finished by setting a flag
            cprintf("[HexPatch] Video playback completed, setting final failed flag\\n");
            m_bFinalFailed = TRUE;
            return S_FALSE; // Signal to exit the render loop
        }

    }
    else if (!m_bFailed)
    {
        // Only render UI if we're not in a failed state (which means we're playing video normally)
        // But since the message box IS our UI, we don't render anything here
        // The message box will be shown after the video loop exits
        
        // Check if we should show the message box
        if (m_bShowMessageBox)
        {
            cprintf("[HexPatch] Showing message box after video completion\\n");
            // Signal that we want to show the message box
            m_bShowMessageBox = FALSE; // Reset the flag
            return S_FALSE; // Exit the render loop
        }
        else if (m_bMessageBoxOpen)
        {
            // Render a black background when message box is open
            ATG::RenderBackground(0xFF000000, 0xFF000000);
        }
    }
    // If m_bFailed is TRUE, we're skipping video, so don't render anything

    // Present the scene
    m_pd3dDevice->Present(NULL, NULL, NULL, NULL);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: InitVideoScreen()
// Desc: Adjust how the movie is displayed on the screen. Horizontal and vertical
//      scaling and rotation are applied.
//--------------------------------------------------------------------------------------
VOID HexPatch::InitVideoScreen()
{
    const int width = m_d3dpp.BackBufferWidth;
    const int height = m_d3dpp.BackBufferHeight;
    const int hWidth = width / 2;
    const int hHeight = height / 2;

    // Parameters to control scaling and rotation of video.
    float m_angle = 0.0;
    float m_xScale = 1.0;
    float m_yScale = 1.0;

    // Scale the output width.
    float left = -hWidth * m_xScale;
    float right = hWidth * m_xScale;
    float top = -hHeight * m_yScale;
    float bottom = hHeight * m_yScale;

    float cosTheta = cos(m_angle);
    float sinTheta = sin(m_angle);

    // Apply the scaling and rotation.
    m_videoScreen.aVertices[0].fX = hWidth + (left * cosTheta - top * sinTheta);
    m_videoScreen.aVertices[0].fY = hHeight + (top * cosTheta + left * sinTheta);
    m_videoScreen.aVertices[0].fZ = 0;

    m_videoScreen.aVertices[1].fX = hWidth + (right * cosTheta - top * sinTheta);
    m_videoScreen.aVertices[1].fY = hHeight + (top * cosTheta + right * sinTheta);
    m_videoScreen.aVertices[1].fZ = 0;

    m_videoScreen.aVertices[2].fX = hWidth + (left * cosTheta - bottom * sinTheta);
    m_videoScreen.aVertices[2].fY = hHeight + (bottom * cosTheta + left * sinTheta);
    m_videoScreen.aVertices[2].fZ = 0;

    m_videoScreen.aVertices[3].fX = hWidth + (right * cosTheta - bottom * sinTheta);
    m_videoScreen.aVertices[3].fY = hHeight + (bottom * cosTheta + right * sinTheta);
    m_videoScreen.aVertices[3].fZ = 0;

    // Always leave the UV coordinates at the default values.
    m_videoScreen.aVertices[0].fTu = 0;
    m_videoScreen.aVertices[0].fTv = 0;
    m_videoScreen.aVertices[1].fTu = 1;
    m_videoScreen.aVertices[1].fTv = 0;
    m_videoScreen.aVertices[2].fTu = 0;
    m_videoScreen.aVertices[2].fTv = 1;
    m_videoScreen.aVertices[3].fTu = 1;
    m_videoScreen.aVertices[3].fTv = 1;

    // Tell the XMV player to use the new settings.
    // This locks the vertex buffer so it may cause stalls if called every frame.
    m_xmvPlayer->SetVideoScreen(&m_videoScreen);
}

//--------------------------------------------------------------------------------------
// Name: Run()
// Desc: Override the base Run method to handle video completion
//--------------------------------------------------------------------------------------
VOID HexPatch::Run()
{
    // Check if video is disabled before creating D3D device
    if (IsSuccessVideoDisabled())
    {
        cprintf("[HexPatch] Video is disabled, skipping D3D device creation\n");
        // Set flags to indicate video was not played
        bVideoWasPlayed = FALSE;
        m_bFailed = TRUE;
        m_bFinalFailed = TRUE;
        return;
    }
    
    HRESULT hr;

    // Create Direct3D
    LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);

    // Create the D3D device
    if (FAILED(hr = pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL,
        m_dwDeviceCreationFlags,
        &m_d3dpp, (::D3DDevice**)&m_pd3dDevice)))
    {
        ATG::FatalError("Could not create D3D device!\n");
    }

    pD3D->Release();

    // Allow global access to the device
    ATG::g_pd3dDevice = m_pd3dDevice; // Use the ATG namespace version

    // Initialize the app's device-dependent objects
    if (FAILED(hr = Initialize()))
    {
        ATG::FatalError("Call to Initialize() failed!\n");
    }

    // Run the game loop
    for (; ; )
    {
        // Update the scene
        Update();

        // Render the scene
        HRESULT renderResult = Render();
        
        // Check if we should exit the loop (video finished)
        if (renderResult == S_FALSE)
        {
            break; // Exit the loop when video is finished
        }
    }
    
    cprintf("[HexPatch] Video loop exited, exiting application immediately\n");
    // Exit immediately when video finishes, don't show any message box
    XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
}

//--------------------------------------------------------------------------------------
// Name: main()
// Desc: Entry point to the program
// Main patching process
//--------------------------------------------------------------------------------------

bool buttonFinished = false;

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

    // Run the ui portion of the app with video etc...
    HexPatch atgApp;

    // For movie playback we want to synchronize to the monitor.
    atgApp.m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    ATG::GetVideoSettings(&atgApp.m_d3dpp.BackBufferWidth, &atgApp.m_d3dpp.BackBufferHeight);
    bShouldPlaySuccessVid = TRUE;
    
    // Run the application until the video finishes
    cprintf("[HexPatch] Starting video playback loop\n");
    atgApp.Run();
    cprintf("[HexPatch] Video playback loop finished\n");
    
    // Only show message box prompt if video was not played (disabled/skipped)
    if (!bVideoWasPlayed)
    {
        // After video finishes, show the existing message box functionality
        // --------------------------------------------------------------
        // Name: SMBUI MAIN CLASS
        // Desc: Shows the notification on exploitation success and 
        //       the SMBUI if enabled
        // --------------------------------------------------------------

        cprintf("[HexPatch] Showing success notification with Y button prompt\n");
        ShowNotify(L"Exploit Successful: Press Y to show options");

        // Always give user a 5 second window to press Y to show the message box
        // User can enable message boxes by pressing Y even when they're disabled
        DWORD startTime = GetTickCount();
        DWORD elapsedTime = 0;
        const DWORD waitTime = 5000; // 5 seconds in milliseconds
        
        while (elapsedTime < waitTime)
        {
            // Check for gamepad input
            XINPUT_STATE inputState;
            if (XInputGetState(0, &inputState) == ERROR_SUCCESS)
            {
                short button = inputState.Gamepad.wButtons;

                if (button != 0)
                {
                    // Check if Y button is pressed (XINPUT_GAMEPAD_Y = 0x0020)
                    if (button & XINPUT_GAMEPAD_Y)
                    {
                        cprintf("[HexPatch] Y button pressed, showing message box\n");
                        // Y button pressed, show the message box
                        wsprintfW(dialog_text_buffer, L"HexPatch Exploit Successful\n\nConsole Information:\n\n%s\n%s\n%s", 
                               wConTypeBuf, wCPUKeyBuf, wDVDKeyBuf);
                        MessageBoxWithOptions(dialog_text_buffer);
                        // Exit to dashboard after message box
                        XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
                        return; // Exit the function
                    }

                    buttonFinished = true;
                }
                else if (buttonFinished)
                {
                    buttonFinished = false;
                }
            }
            
            // Update elapsed time
            elapsedTime = GetTickCount() - startTime;
            
            // Small delay to prevent excessive CPU usage
            Sleep(50);
        }
        
        cprintf("[HexPatch] 5 second wait period ended, exiting to dashboard\n");
        // If we reached here without showing the message box, exit to dashboard
        XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
    }
    else
    {
        // Video was played, exit immediately
        cprintf("[HexPatch] Video was played, exiting immediately\n");
        XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
    }


}
