/*
 * JoystickTest
 *
 * Copyright 2012 Lucas Fialho Zawacki
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#define WIN32_LEAN_AND_MEAN

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windows.h>
#include <stdio.h>
#include <dinput.h>

#define NUMELEMS(array) (sizeof(array)/sizeof(array[0]))

BOOL gVerbose = FALSE;

struct Joystick {
    IDirectInputDevice8A *device;
    DIDEVICEINSTANCEA instance;
    int num_buttons;
    int num_axes;
};

struct JoystickData {
    HWND hwnd;
    IDirectInput8A *di;
    struct Joystick *joysticks;
    int num_joysticks;
    int cur_joystick;
    int poll_time;
    int chosen_joystick;
    int axes_max;
    int axes_min;
    BOOL buffered;
};

static void PrintUsage(void)
{
    /* Options are { 'poll', 'max-range', 'min-range', 'list', 'joystick chosen', 'verbose', help' } */
    puts("  usage: wine joystick");
    puts("");
    puts("    -h");
    puts("       Prints this help message and exits");
    puts("    -p <number>");
    puts("       Poll for joystick input every <number> microseconds");
    puts("    -l");
    puts("       Lists all connected joysticks");
    puts("    -a <range>");
    puts("       Sets max value for axis range");
    puts("    -i <range>");
    puts("       Sets min value for axis range");
    puts("    -j <number>");
    puts("       Selects the joystick to poll for input. Defaults to 0");
    puts("       Joystick numbers can be seen with the -l command");
    puts("    -v");
    puts("       Verbose output\n");
}

static void DumpState(DIJOYSTATE* st, int num_buttons)
{
    int i;
    printf("----------------------------------\n");
    printf("Ax: (% 5d,% 5d,% 5d)\tRAx: (% 5d,% 5d,% 5d)\nSlider: (% 5d,% 5d)\tPov: (% 5d,% 5d,% 5d,% 5d)\n\n",
                st->lX,st->lY,st->lZ,st->lRx,st->lRy,st->lRz,
                st->rglSlider[0],st->rglSlider[1],
                st->rgdwPOV[0],st->rgdwPOV[1],st->rgdwPOV[2],st->rgdwPOV[3]
           );

    for(i=0; i < num_buttons; i++) {
        printf("% 3d", i);
    }
    printf("\n");
    for(i=0; i < num_buttons; i++) {
        printf("  %c",st->rgbButtons[i] ? 'x' : 'o');
    }

    printf("\n");
}

void DumpDeviceObject(const DIDEVICEOBJECTINSTANCEA *instance)
{
    int i;

    static REFGUID guidTypes[] =
        { &GUID_Button, &GUID_XAxis, &GUID_YAxis, &GUID_ZAxis, &GUID_RxAxis, &GUID_RyAxis, &GUID_RzAxis, &GUID_Slider};
    static const char *guidStrings[] = { "Button", "X", "Y", "Z", "Rx", "Ry", "Rz", "Slider"};

    /* Print information about buttons and axes */
    if (gVerbose) {
        for (i=0; i < NUMELEMS(guidTypes); i++)
            if (IsEqualGUID(&instance->guidType, guidTypes[i]))
                printf ("%s (%s)\n", instance->tszName, guidStrings[i]);
    }
}

/*
    EnumObjectCallback

    Counts buttons and axes and sets some
    object properties.
*/
static BOOL CALLBACK EnumObjectsCallback(const DIDEVICEOBJECTINSTANCEA *instance, void *context)
{
    struct JoystickData *data = context;
    struct Joystick *joystick = &data->joysticks[data->cur_joystick];
    DIPROPRANGE propRange;

    DumpDeviceObject(instance);

    if (instance->dwType & DIDFT_AXIS) {
        joystick->num_axes += 1;

        /* Set axis range */
        propRange.diph.dwSize = sizeof(DIPROPRANGE);
        propRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        propRange.diph.dwHow = DIPH_BYID;
        propRange.diph.dwObj = instance->dwType;
        propRange.lMin = data->axes_min;
        propRange.lMax = data->axes_max;

        IDirectInputDevice_SetProperty(joystick->device, DIPROP_RANGE, &propRange.diph);
    }

    if (instance->dwType & DIDFT_BUTTON) joystick->num_buttons += 1;

    return DIENUM_CONTINUE;
}

/*
    EnumCallback
    Enumerate all the joystick devices
*/
static BOOL CALLBACK EnumCallback(const DIDEVICEINSTANCEA *instance, void *context)
{
    struct JoystickData *data = context;
    struct Joystick *joystick;

    if (data->joysticks == NULL)
    {
        data->num_joysticks += 1;
        return DIENUM_CONTINUE;
    }

    joystick = &data->joysticks[data->cur_joystick];
    data->cur_joystick += 1;

    IDirectInput8_CreateDevice(data->di, &instance->guidInstance, &joystick->device, NULL);
    IDirectInputDevice8_SetDataFormat(joystick->device, &c_dfDIJoystick);
    joystick->instance = *instance;
    joystick->num_buttons = 0;
    joystick->num_axes = 0;

    return DIENUM_CONTINUE;
}

static HRESULT Poll(IDirectInputDevice8A *joystick, DIJOYSTATE *state)
{
    HRESULT  hr;

    hr = IDirectInputDevice8_Poll(joystick);

    /* Try to acquire the joystick */
    if (FAILED(hr)) {

        hr = IDirectInputDevice8_Acquire(joystick);

        while (hr == DIERR_INPUTLOST)
            hr = IDirectInputDevice8_Acquire(joystick);

        if (hr == DIERR_INVALIDPARAM || hr == DIERR_NOTINITIALIZED) return E_FAIL;

        if (hr == DIERR_OTHERAPPHASPRIO) return S_OK;
    }

    hr = IDirectInputDevice8_GetDeviceState(joystick, sizeof(DIJOYSTATE), state);

    /* Should not fail, the device has just been acquired */
    if (FAILED(hr)) return hr;

    return S_OK;
}

static void WaitForInput(const struct JoystickData* data)
{
    int i = 0;
    int chosen = data->chosen_joystick;
    int num_buttons = data->joysticks[chosen].num_buttons;

    struct Joystick *joystick = &data->joysticks[chosen];
    DIJOYSTATE state;
    BOOL pressed = FALSE;

    ZeroMemory(&state, sizeof(state));

    printf("Polling input from '%s'\n", joystick->instance.tszInstanceName);
    printf("Joystick has %d buttons and %d axes\n", joystick->num_buttons, joystick->num_axes);
    printf("Press any joystick key\n");

    while (!Poll(joystick->device, &state) && !pressed)
        for (i=0; i < num_buttons && !pressed; i++)
            pressed = pressed || state.rgbButtons[i];

    /* Wait for input and dump */
    while (!Poll(joystick->device, &state)) {
        DumpState(&state, num_buttons);
        Sleep(data->poll_time);
    }
}

static void ProcessCmdLine(struct JoystickData *params, LPSTR lpCmdLine)
{
    int i, j, buffer_index;
    /* Options are { 'poll', 'max-range', 'min-range', 'joystick chosen', 'verbose', help' } */
    char options[] = { 'p', 'a', 'i', 'j', 'v', 'h' };
    char buffer[32];
    char command;

    for (i=0; lpCmdLine[i] != '\0'; i++)
    {
        if (lpCmdLine[i] == '/' || lpCmdLine[i] == '-')
        {
            /* Find valid command */
            BOOL valid = FALSE;
            command = lpCmdLine[++i];
            for (j=0; j < NUMELEMS(options) && !valid; j++)
                if (options[j] == command)
                    valid = TRUE;

            if (!valid) goto invalid;

            /* Skip extra spaces */
            while (lpCmdLine[++i] == ' ');

            /* If the next word is a parameter, copy it.
               Parameters dont start with - or /, that's for commands.
               The isdigit call is to test for negative numbers (-1 for example)
               and to not confuse them with commands */
            buffer_index = 0;
            if ((lpCmdLine[i] != '-' || isdigit(lpCmdLine[i+1])) && lpCmdLine[i] != '/')
            {
                /* Copy the word and don't let it overflow */
                while (lpCmdLine[i] != ' ' && lpCmdLine[i] != '\0' && buffer_index < sizeof(buffer))
                {
                    buffer[buffer_index] = lpCmdLine[i];
                    buffer_index++;
                    i++;
                }
            }
            buffer[buffer_index] = '\0';

            /* Process command and extract parameter */
            switch (command)
            {
                case 'p':
                    if (strlen(buffer) == 0) goto invalid;
                    params->poll_time = atoi(buffer);
                break;

                case 'a':
                    if (strlen(buffer) == 0) goto invalid;
                    params->axes_max = atoi(buffer);
                break;

                case 'i':
                    if (strlen(buffer) == 0) goto invalid;
                    params->axes_min = atoi(buffer);
                break;

                case 'j':
                    if (strlen(buffer) == 0) goto invalid;
                    params->chosen_joystick = atoi(buffer);
                break;

                case 'v':
                    gVerbose = TRUE;

                case 'h':
                    PrintUsage();
                    exit(0);
                break;
            }
        }
    }

    return;

    invalid:
    printf("Invalid parameters or command line option '%c'\n", command);
    printf("Try using -h for help with commands and syntax\n");
    exit(1);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR szCmdLine, int nShow)
{
    /* Structure with the data and settings for the application */
    /* data is: hwnd, lpdi, joy[], num_joy, cur_joy, chosen_joy, poll_time, axes_max, axes_min, buffered */
    struct JoystickData data = { NULL, NULL, NULL, 0, 0, 0, 0, 1000, -1000, FALSE };
    HRESULT hr;

    hr = DirectInput8Create(GetModuleHandleA(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void**) &data.di, NULL);

    if (FAILED(hr)) {
        printf("Failed to initialize DirectInput: 0x%08x\n", hr);
        return 1;
    }

    /* First count how many joysticks are there */
    hr = IDirectInput8_EnumDevices(data.di, DI8DEVCLASS_GAMECTRL, EnumCallback, &data, DIEDFL_ATTACHEDONLY);
    data.joysticks = malloc(sizeof(struct Joystick) * data.num_joysticks);

    /* Get all the joysticks */
    hr = IDirectInput8_EnumDevices(data.di, DI8DEVCLASS_GAMECTRL, EnumCallback, &data, DIEDFL_ATTACHEDONLY);

    /* Get settings from the command line */
    ProcessCmdLine(&data, szCmdLine);

    /* Apply settings for all joysticks */
    for (data.cur_joystick = 0; data.cur_joystick < data.num_joysticks; data.cur_joystick++)
    {
        IDirectInputDevice8_EnumObjects(
            data.joysticks[data.cur_joystick].device, EnumObjectsCallback, &data, DIDFT_AXIS | DIDFT_BUTTON);
    }

    printf("Found %d joysticks.\n", data.num_joysticks);

    /* Default case just lists the joysticks */
    if (data.poll_time == 0) {

        int i = 0;
        for (i=0; i < data.num_joysticks; i++)
            printf("%d: %s\n", i, data.joysticks[i].instance.tszInstanceName);
    }
    else {

        /* If we'll poll the joystick for input */
        if (data.num_joysticks > 0) {

            if (data.chosen_joystick >= data.num_joysticks || data.chosen_joystick < 0)
            {
                printf("Joystick '%d' is not connected\n", data.chosen_joystick);
                exit(1);
            }

            WaitForInput(&data);
        }
    }
    return 0;
}
