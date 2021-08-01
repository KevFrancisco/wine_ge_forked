/*  DirectInput HID Joystick device
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
 */

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"

#include "ddk/hidsdi.h"
#include "setupapi.h"
#include "devguid.h"
#include "dinput.h"
#include "setupapi.h"

#include "wine/debug.h"

#include "dinput_private.h"
#include "device_private.h"
#include "joystick_private.h"

#include "initguid.h"
#include "devpkey.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

DEFINE_GUID( hid_joystick_guid, 0x9e573edb, 0x7734, 0x11d2, 0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7 );
DEFINE_DEVPROPKEY( DEVPROPKEY_HID_HANDLE, 0xbc62e415, 0xf4fe, 0x405c, 0x8e, 0xda, 0x63, 0x6f, 0xb5, 0x9f, 0x08, 0x98, 2 );

struct hid_joystick
{
    IDirectInputDeviceImpl base;

    HANDLE device;
    PHIDP_PREPARSED_DATA preparsed;
};

static inline struct hid_joystick *impl_from_IDirectInputDevice8W( IDirectInputDevice8W *iface )
{
    return CONTAINING_RECORD( CONTAINING_RECORD( iface, IDirectInputDeviceImpl, IDirectInputDevice8W_iface ),
                              struct hid_joystick, base );
}

static ULONG WINAPI hid_joystick_Release( IDirectInputDevice8W *iface )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    struct hid_joystick tmp = *impl;
    ULONG res;

    if (!(res = IDirectInputDevice2WImpl_Release( iface )))
    {
        HidD_FreePreparsedData( tmp.preparsed );
        CloseHandle( tmp.device );
    }

    return res;
}

static HRESULT WINAPI hid_joystick_GetCapabilities( IDirectInputDevice8W *iface, DIDEVCAPS *caps )
{
    FIXME( "iface %p, caps %p stub!\n", iface, caps );

    if (!caps) return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI hid_joystick_EnumObjects( IDirectInputDevice8W *iface, LPDIENUMDEVICEOBJECTSCALLBACKW callback,
                                                void *ref, DWORD flags )
{
    FIXME( "iface %p, callback %p, ref %p, flags %#x stub!\n", iface, callback, ref, flags );

    if (!callback) return DIERR_INVALIDPARAM;

    return E_NOTIMPL;
}

static HRESULT WINAPI hid_joystick_GetProperty( IDirectInputDevice8W *iface, REFGUID guid, DIPROPHEADER *header )
{
    FIXME( "iface %p, guid %s, header %p stub!\n", iface, debugstr_guid( guid ), header );

    if (!header) return DIERR_INVALIDPARAM;
    if (!IS_DIPROP( guid )) return DI_OK;

    switch (LOWORD( guid ))
    {
    default: return IDirectInputDevice2WImpl_GetProperty( iface, guid, header );
    }
}

static HRESULT WINAPI hid_joystick_SetProperty( IDirectInputDevice8W *iface, REFGUID guid, const DIPROPHEADER *header )
{
    FIXME( "iface %p, guid %s, header %p stub!\n", iface, debugstr_guid( guid ), header );

    if (!header) return DIERR_INVALIDPARAM;
    if (!IS_DIPROP( guid )) return DI_OK;

    switch (LOWORD( guid ))
    {
    default: return IDirectInputDevice2WImpl_SetProperty( iface, guid, header );
    }
}

static HRESULT WINAPI hid_joystick_Acquire( IDirectInputDevice8W *iface )
{
    HRESULT hr;

    TRACE( "iface %p.\n", iface );

    if ((hr = IDirectInputDevice2WImpl_Acquire( iface )) != DI_OK) return hr;

    return DI_OK;
}

static HRESULT WINAPI hid_joystick_Unacquire( IDirectInputDevice8W *iface )
{
    HRESULT hr;

    TRACE( "iface %p.\n", iface );

    if ((hr = IDirectInputDevice2WImpl_Unacquire( iface )) != DI_OK) return hr;

    return DI_OK;
}

static HRESULT WINAPI hid_joystick_GetDeviceState( IDirectInputDevice8W *iface, DWORD len, void *ptr )
{
    FIXME( "iface %p, len %u, ptr %p stub!\n", iface, len, ptr );

    if (!ptr) return DIERR_INVALIDPARAM;

    return E_NOTIMPL;
}

static HRESULT WINAPI hid_joystick_GetObjectInfo( IDirectInputDevice8W *iface, DIDEVICEOBJECTINSTANCEW *instance,
                                                  DWORD obj, DWORD how )
{
    FIXME( "iface %p, instance %p, obj %#x, how %#x stub!\n", iface, instance, obj, how );

    if (!instance) return E_POINTER;
    if (instance->dwSize != sizeof(DIDEVICEOBJECTINSTANCE_DX3W) &&
        instance->dwSize != sizeof(DIDEVICEOBJECTINSTANCEW))
        return DIERR_INVALIDPARAM;

    return E_NOTIMPL;
}

static HRESULT WINAPI hid_joystick_GetDeviceInfo( IDirectInputDevice8W *iface, DIDEVICEINSTANCEW *instance )
{
    FIXME( "iface %p, instance %p stub!\n", iface, instance );

    if (!instance) return E_POINTER;
    if (instance->dwSize != sizeof(DIDEVICEINSTANCE_DX3W) &&
        instance->dwSize != sizeof(DIDEVICEINSTANCEW))
        return DIERR_INVALIDPARAM;

    return E_NOTIMPL;
}

static HRESULT WINAPI hid_joystick_CreateEffect( IDirectInputDevice8W *iface, REFGUID rguid,
                                                 const DIEFFECT *effect, IDirectInputEffect **out,
                                                 IUnknown *outer )
{
    FIXME( "iface %p, rguid %s, effect %p, out %p, outer %p stub!\n", iface, debugstr_guid( rguid ),
           effect, out, outer );

    if (!out) return E_POINTER;
    if (!rguid || !effect) return DI_NOEFFECT;

    return E_NOTIMPL;
}

static HRESULT WINAPI hid_joystick_EnumEffects( IDirectInputDevice8W *iface, LPDIENUMEFFECTSCALLBACKW callback,
                                                void *ref, DWORD type )
{
    FIXME( "iface %p, callback %p, ref %p, type %#x stub!\n", iface, callback, ref, type );

    if (!callback) return DIERR_INVALIDPARAM;

    return E_NOTIMPL;
}

static HRESULT WINAPI hid_joystick_GetEffectInfo( IDirectInputDevice8W *iface, DIEFFECTINFOW *info, REFGUID guid )
{
    FIXME( "iface %p, info %p, guid %s stub!\n", iface, info, debugstr_guid( guid ) );

    if (!info) return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI hid_joystick_GetForceFeedbackState( IDirectInputDevice8W *iface, DWORD *out )
{
    FIXME( "iface %p, out %p stub!\n", iface, out );

    if (!out) return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI hid_joystick_SendForceFeedbackCommand( IDirectInputDevice8W *iface, DWORD flags )
{
    FIXME( "iface %p, flags %x stub!\n", iface, flags );
    return E_NOTIMPL;
}

static HRESULT WINAPI hid_joystick_EnumCreatedEffectObjects( IDirectInputDevice8W *iface,
                                                             LPDIENUMCREATEDEFFECTOBJECTSCALLBACK callback,
                                                             void *ref, DWORD flags )
{
    FIXME( "iface %p, callback %p, ref %p, flags %#x stub!\n", iface, callback, ref, flags );

    if (!callback) return DIERR_INVALIDPARAM;

    return E_NOTIMPL;
}

static HRESULT WINAPI hid_joystick_BuildActionMap( IDirectInputDevice8W *iface, DIACTIONFORMATW *format,
                                                   const WCHAR *username, DWORD flags )
{
    FIXME( "iface %p, format %p, username %s, flags %#x stub!\n", iface, format, debugstr_w(username), flags );

    if (!format) return DIERR_INVALIDPARAM;

    return E_NOTIMPL;
}

static HRESULT WINAPI hid_joystick_SetActionMap( IDirectInputDevice8W *iface, DIACTIONFORMATW *format,
                                                 const WCHAR *username, DWORD flags )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, format %p, username %s, flags %#x.\n", iface, format, debugstr_w(username), flags );

    if (!format) return DIERR_INVALIDPARAM;

    return _set_action_map( iface, format, username, flags, impl->base.data_format.wine_df );
}

static const IDirectInputDevice8WVtbl hid_joystick_vtbl =
{
    /*** IUnknown methods ***/
    IDirectInputDevice2WImpl_QueryInterface,
    IDirectInputDevice2WImpl_AddRef,
    hid_joystick_Release,
    /*** IDirectInputDevice methods ***/
    hid_joystick_GetCapabilities,
    hid_joystick_EnumObjects,
    hid_joystick_GetProperty,
    hid_joystick_SetProperty,
    hid_joystick_Acquire,
    hid_joystick_Unacquire,
    hid_joystick_GetDeviceState,
    IDirectInputDevice2WImpl_GetDeviceData,
    IDirectInputDevice2WImpl_SetDataFormat,
    IDirectInputDevice2WImpl_SetEventNotification,
    IDirectInputDevice2WImpl_SetCooperativeLevel,
    hid_joystick_GetObjectInfo,
    hid_joystick_GetDeviceInfo,
    IDirectInputDevice2WImpl_RunControlPanel,
    IDirectInputDevice2WImpl_Initialize,
    /*** IDirectInputDevice2 methods ***/
    hid_joystick_CreateEffect,
    hid_joystick_EnumEffects,
    hid_joystick_GetEffectInfo,
    hid_joystick_GetForceFeedbackState,
    hid_joystick_SendForceFeedbackCommand,
    hid_joystick_EnumCreatedEffectObjects,
    IDirectInputDevice2WImpl_Escape,
    IDirectInputDevice2WImpl_Poll,
    IDirectInputDevice2WImpl_SendDeviceData,
    /*** IDirectInputDevice7 methods ***/
    IDirectInputDevice7WImpl_EnumEffectsInFile,
    IDirectInputDevice7WImpl_WriteEffectToFile,
    /*** IDirectInputDevice8 methods ***/
    hid_joystick_BuildActionMap,
    hid_joystick_SetActionMap,
    IDirectInputDevice8WImpl_GetImageInfo,
};

static BOOL hid_joystick_device_try_open( UINT32 handle, const WCHAR *path, HANDLE *device,
                                          PHIDP_PREPARSED_DATA *preparsed, HIDD_ATTRIBUTES *attrs,
                                          HIDP_CAPS *caps, DIDEVICEINSTANCEW *instance, DWORD version )
{
    PHIDP_PREPARSED_DATA preparsed_data = NULL;
    HANDLE device_file;

    device_file = CreateFileW( path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, OPEN_EXISTING, 0, 0 );
    if (device_file == INVALID_HANDLE_VALUE) return FALSE;

    if (!HidD_GetPreparsedData( device_file, &preparsed_data )) goto failed;
    if (!HidD_GetAttributes( device_file, attrs )) goto failed;
    if (HidP_GetCaps( preparsed_data, caps ) != HIDP_STATUS_SUCCESS) goto failed;

    if (caps->UsagePage == HID_USAGE_PAGE_GAME) FIXME( "Unimplemented HID game usage page!\n" );
    if (caps->UsagePage == HID_USAGE_PAGE_SIMULATION) FIXME( "Unimplemented HID simulation usage page!\n" );
    if (caps->UsagePage != HID_USAGE_PAGE_GENERIC) goto failed;
    if (caps->Usage != HID_USAGE_GENERIC_GAMEPAD && caps->Usage != HID_USAGE_GENERIC_JOYSTICK) goto failed;

    if (!HidD_GetProductString( device_file, instance->tszInstanceName, MAX_PATH )) goto failed;
    if (!HidD_GetProductString( device_file, instance->tszProductName, MAX_PATH )) goto failed;

    instance->guidInstance = hid_joystick_guid;
    instance->guidInstance.Data1 ^= handle;
    instance->guidProduct = DInput_PIDVID_Product_GUID;
    instance->guidProduct.Data1 = MAKELONG( attrs->VendorID, attrs->ProductID );
    instance->dwDevType = get_device_type( version, caps->Usage != HID_USAGE_GENERIC_GAMEPAD ) | DIDEVTYPE_HID;
    instance->guidFFDriver = GUID_NULL;
    instance->wUsagePage = caps->UsagePage;
    instance->wUsage = caps->Usage;

    *device = device_file;
    *preparsed = preparsed_data;
    return TRUE;

failed:
    CloseHandle( device_file );
    HidD_FreePreparsedData( preparsed_data );
    return FALSE;
}

static HRESULT hid_joystick_device_open( int index, DIDEVICEINSTANCEW *filter, WCHAR *device_path,
                                         HANDLE *device, PHIDP_PREPARSED_DATA *preparsed,
                                         HIDD_ATTRIBUTES *attrs, HIDP_CAPS *caps, DWORD version )
{
    char buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + MAX_PATH * sizeof(WCHAR)];
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail = (void *)buffer;
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVINFO_DATA devinfo = {sizeof(devinfo)};
    DIDEVICEINSTANCEW instance = *filter;
    UINT32 i = 0, handle;
    HDEVINFO set;
    DWORD type;
    GUID hid;

    TRACE( "index %d, product %s, instance %s\n", index, debugstr_guid( &filter->guidProduct ),
           debugstr_guid( &filter->guidInstance ) );

    HidD_GetHidGuid( &hid );

    set = SetupDiGetClassDevsW( &hid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );
    if (set == INVALID_HANDLE_VALUE) return DIERR_DEVICENOTREG;

    *device = NULL;
    *preparsed = NULL;
    while (SetupDiEnumDeviceInterfaces( set, NULL, &hid, i++, &iface ))
    {
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW( set, &iface, detail, sizeof(buffer), NULL, &devinfo ))
            continue;
        if (!SetupDiGetDevicePropertyW( set, &devinfo, &DEVPROPKEY_HID_HANDLE, &type,
                                        (BYTE *)&handle, sizeof(handle), NULL, 0 ) ||
            type != DEVPROP_TYPE_UINT32)
            continue;
        if (!hid_joystick_device_try_open( handle, detail->DevicePath, device, preparsed,
                                           attrs, caps, &instance, version ))
            continue;

        /* enumerate device by GUID */
        if (index < 0 && IsEqualGUID( &filter->guidProduct, &instance.guidProduct )) break;
        if (index < 0 && IsEqualGUID( &filter->guidInstance, &instance.guidInstance )) break;

        /* enumerate all devices */
        if (index >= 0 && !index--) break;

        CloseHandle( *device );
        HidD_FreePreparsedData( *preparsed );
        *device = NULL;
        *preparsed = NULL;
    }

    SetupDiDestroyDeviceInfoList( set );
    if (!*device || !*preparsed) return DIERR_DEVICENOTREG;

    lstrcpynW( device_path, detail->DevicePath, MAX_PATH );
    *filter = instance;
    return DI_OK;
}

static HRESULT hid_joystick_enum_device( DWORD type, DWORD flags, DIDEVICEINSTANCEW *instance,
                                         DWORD version, int index )
{
    HIDD_ATTRIBUTES attrs = {sizeof(attrs)};
    PHIDP_PREPARSED_DATA preparsed;
    WCHAR device_path[MAX_PATH];
    HIDP_CAPS caps;
    HANDLE device;
    HRESULT hr;

    TRACE( "type %x, flags %#x, instance %p, version %04x, index %d\n", type, flags, instance, version, index );

    hr = hid_joystick_device_open( index, instance, device_path, &device, &preparsed,
                                   &attrs, &caps, version );
    if (hr != DI_OK) return hr;

    HidD_FreePreparsedData( preparsed );
    CloseHandle( device );

    if (instance->dwSize != sizeof(DIDEVICEINSTANCEW))
        return S_FALSE;
    if (version < 0x0800 && type != DIDEVTYPE_JOYSTICK)
        return S_FALSE;
    if (version >= 0x0800 && type != DI8DEVCLASS_ALL && type != DI8DEVCLASS_GAMECTRL)
        return S_FALSE;

    if (device_disabled_registry( "HID", TRUE ))
        return DIERR_DEVICENOTREG;

    TRACE( "Found device %s, usage %04x:%04x, product %s, instance %s, name %s\n", debugstr_w(device_path),
           instance->wUsagePage, instance->wUsage, debugstr_guid( &instance->guidProduct ),
           debugstr_guid( &instance->guidInstance ), debugstr_w(instance->tszInstanceName) );

    return DI_OK;
}

static HRESULT hid_joystick_create_device( IDirectInputImpl *dinput, REFGUID guid, IDirectInputDevice8W **out )
{
    DIDEVICEINSTANCEW instance = {.dwSize = sizeof(instance), .guidProduct = *guid, .guidInstance = *guid};
    DWORD size = sizeof(struct hid_joystick);
    HIDD_ATTRIBUTES attrs = {sizeof(attrs)};
    struct hid_joystick *impl = NULL;
    PHIDP_PREPARSED_DATA preparsed;
    DIDATAFORMAT *format = NULL;
    WCHAR device_path[MAX_PATH];
    HIDP_CAPS caps;
    HANDLE device;
    HRESULT hr;

    TRACE( "dinput %p, guid %s, out %p\n", dinput, debugstr_guid( guid ), out );

    *out = NULL;
    instance.guidProduct.Data1 = DInput_PIDVID_Product_GUID.Data1;
    instance.guidInstance.Data1 = hid_joystick_guid.Data1;
    if (IsEqualGUID( &DInput_PIDVID_Product_GUID, &instance.guidProduct ))
        instance.guidProduct = *guid;
    else if (IsEqualGUID( &hid_joystick_guid, &instance.guidInstance ))
        instance.guidInstance = *guid;
    else
        return DIERR_DEVICENOTREG;

    hr = hid_joystick_device_open( -1, &instance, device_path, &device, &preparsed,
                                   &attrs, &caps, dinput->dwVersion );
    if (hr != DI_OK) return hr;

    hr = direct_input_device_alloc( size, &hid_joystick_vtbl, guid, dinput, (void **)&impl );
    if (FAILED(hr)) goto failed;

    impl->base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": hid_joystick.base.crit");
    impl->base.dwCoopLevel = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;

    impl->device = device;
    impl->preparsed = preparsed;

    if (!(format = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*format) ))) goto failed;
    impl->base.data_format.wine_df = format;

    TRACE( "Created %p\n", impl );

    *out = &impl->base.IDirectInputDevice8W_iface;
    return DI_OK;

failed:
    HeapFree( GetProcessHeap(), 0, format );
    HeapFree( GetProcessHeap(), 0, impl );
    HidD_FreePreparsedData( preparsed );
    CloseHandle( device );
    return hr;
}

const struct dinput_device joystick_hid_device =
{
    "Wine HID joystick driver",
    hid_joystick_enum_device,
    hid_joystick_create_device,
};
