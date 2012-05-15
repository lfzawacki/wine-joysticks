/*
 * Force Feedback Test
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

struct Effect {
    IDirectInputEffect *effect;
    DIEFFECTINFO info;
};

struct Joystick {
    IDirectInputDevice8A *device;
    DIDEVICEINSTANCEA instance;
    int num_effects;
    int cur_effect;
    struct Effect *effects;
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

void dump_effect(const DIEFFECTINFO *pdei) {
    unsigned int i;
    REFGUID guid = &pdei->guid;
    static const struct {
    const GUID *guid;
    const char *name;
    } guids[] = {
#define FE(x) { &x, #x}
    FE(GUID_ConstantForce), FE(GUID_RampForce), FE(GUID_Square),
    FE(GUID_Sine), FE(GUID_Triangle), FE(GUID_SawtoothUp),
    FE(GUID_SawtoothDown), FE(GUID_Spring), FE(GUID_Damper),
    FE(GUID_Inertia), FE(GUID_Friction), FE(GUID_CustomForce)
#undef FE
    };

#define X(x) if (pdei->dwEffType & x) printf("\tEffectType |= "#x"\n");
    X(DIEFT_CONSTANTFORCE);
    X(DIEFT_PERIODIC);
    X(DIEFT_RAMPFORCE);
    X(DIEFT_CONDITION);
#undef X

    for (i = 0; i < (sizeof(guids) / sizeof(guids[0])); i++)
        if (IsEqualGUID(guids[i].guid, guid))
            printf("%s\n", guids[i].name);
}

BOOL CALLBACK EffectsCallback(const DIEFFECTINFO *pdei, LPVOID pvRef)
{
    struct Joystick *joystick = pvRef;
    HRESULT hr;
    DIEFFECT diEffect;

    if (joystick->effects == NULL)
    {
        joystick->num_effects += 1;
        return DIENUM_CONTINUE;
    }

    dump_effect(pdei);

    DWORD dwAxes[1] = { DIJOFS_X };
    LONG lDirection[1] = { 200 };

    DIPERIODIC  diPeriodic;
    diPeriodic.dwMagnitude = 5000;
    diPeriodic.lOffset = 0;
    diPeriodic.dwPhase = 0;
    diPeriodic.dwPeriod = DI_SECONDS;

    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.dwFlags = DIEFF_CARTESIAN;
    diEffect.dwDuration = INFINITE;
    diEffect.dwSamplePeriod = 0;
    diEffect.dwGain = DI_FFNOMINALMAX;
    diEffect.dwTriggerButton =  DIEB_NOTRIGGER;
    diEffect.dwTriggerRepeatInterval = 0;
    diEffect.cAxes = 1;
    diEffect.rgdwAxes = dwAxes;
    diEffect.rglDirection = &lDirection[0];
    diEffect.lpEnvelope = NULL;
    diEffect.cbTypeSpecificParams = sizeof(diPeriodic);
    diEffect.lpvTypeSpecificParams = &diPeriodic;

    hr = IDirectInputDevice8_Acquire(joystick->device);

    if (FAILED(hr)) return DIENUM_CONTINUE;

    hr = IDirectInputDevice2_CreateEffect(
        joystick->device, &pdei->guid, &diEffect, &joystick->effects[joystick->cur_effect].effect, NULL);

    joystick->cur_effect += 1;

    return DIENUM_CONTINUE;
}

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
    joystick->num_effects = 0;

    return DIENUM_CONTINUE;
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrev, LPSTR szCmdLine, int nShow)
{
    /* Structure with the data and settings for the application */
    /* data is: hwnd, lpdi, joy[], num_joy, cur_joy, chosen_joy, poll_time, axes_max, axes_min, buffered */
    struct JoystickData data = { NULL, NULL, NULL, 0, 0, 0, 0, 1000, -1000, FALSE };
    HRESULT hr;

    hr = DirectInput8Create(GetModuleHandleA(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void**) &data.di, NULL);

    if (FAILED(hr))
    {
        printf("Failed to initialize DirectInput: 0x%08x\n", hr);
        return 1;
    }

    /* First count how many joysticks are there */
    hr = IDirectInput8_EnumDevices(data.di, DI8DEVCLASS_GAMECTRL, EnumCallback, &data, DIEDFL_FORCEFEEDBACK);
    data.joysticks = malloc(sizeof(struct Joystick) * data.num_joysticks);

    /* Get all the joysticks */
    hr = IDirectInput8_EnumDevices(data.di, DI8DEVCLASS_GAMECTRL, EnumCallback, &data, DIEDFL_FORCEFEEDBACK);

    printf("Found %d force feedback enabled joysticks.\n", data.num_joysticks);

    /* Default case just lists the joysticks */
    int i = 0;
    for (i=0; i < data.num_joysticks; i++)
        printf("%d: %s\n", i, data.joysticks[i].instance.tszInstanceName);

    if (data.num_joysticks > 0)
    {
        struct Joystick *joystick = &data.joysticks[0];

        printf("Enumerating effects\n");
        IDirectInputDevice2_EnumEffects(joystick->device, EffectsCallback, (void*) joystick, 0);
        joystick->effects = malloc(sizeof(struct Effect) * joystick->num_effects);

        joystick->cur_effect = 0;
        IDirectInputDevice2_EnumEffects(joystick->device, EffectsCallback, (void*) joystick, 0);

        hr = IDirectInputEffect_Start(joystick->effects[1].effect, 1, 0);

        Sleep(1000);
    }

    return 0;
}
