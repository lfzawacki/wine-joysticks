#include <windows.h>
#include <dinput.h>
#include <stdio.h>

IDirectInput8A *lpdi = NULL;
IDirectInputDevice8A *device = NULL;

BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, void* context)
{
    int id = 0, vid = 0, pid = 0;
    HRESULT hr;
    DIPROPDWORD dipw;

    hr = IDirectInput_CreateDevice(lpdi, &instance->guidInstance, &device, NULL);

    ZeroMemory(&dipw, sizeof(dipw));

    dipw.diph.dwSize       = sizeof(DIPROPDWORD);
    dipw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipw.diph.dwObj        = 0;
    dipw.diph.dwHow        = DIPH_DEVICE;

    printf("Looking at %s\n",instance->tszInstanceName);

    hr = IDirectInputDevice_GetProperty(device, DIPROP_VIDPID, &dipw.diph);

    if (hr != DI_OK)
    {
        printf("Failed retrieving VIDPID\n");
        return DIENUM_CONTINUE;
    }

    vid = LOWORD(dipw.dwData);
    pid = HIWORD(dipw.dwData);

    hr = IDirectInputDevice_GetProperty(device, DIPROP_JOYSTICKID, &dipw.diph);

    if (hr != DI_OK)
    {
        printf("Failed retrieving ID\n");
        return DIENUM_CONTINUE;
    }

    id = dipw.dwData;

    printf("ID: %d\nVID: %03d PID: %03d\n", id, vid, pid);

    hr = IDirectInputDevice_Release(device);

    return DIENUM_CONTINUE;
}

int main()
{
    HRESULT hr;

    hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void**)&lpdi, NULL);

    hr = IDirectInput8_EnumDevices(lpdi, DI8DEVCLASS_GAMECTRL, enumCallback, NULL, DIEDFL_ATTACHEDONLY);

    return 0;
}
