/* Copyright 2022 Rémi Bernon for CodeWeavers
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

#include "gst_private.h"

#include "mfapi.h"
#include "mferror.h"
#include "mfobjects.h"
#include "mftransform.h"
#include "wmcodecdsp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static const GUID *const input_types[] =
{
    &MFVideoFormat_IYUV,
    &MFVideoFormat_YV12,
    &MFVideoFormat_NV12,
    &MFVideoFormat_420O,
    &MFVideoFormat_UYVY,
    &MFVideoFormat_YUY2,
    &MEDIASUBTYPE_P208,
    &MFVideoFormat_NV11,
    &MFVideoFormat_AYUV,
    &MFVideoFormat_ARGB32,
    &MFVideoFormat_RGB32,
    &MFVideoFormat_RGB24,
    &MFVideoFormat_I420,
    &MFVideoFormat_YVYU,
    &MFVideoFormat_RGB555,
    &MFVideoFormat_RGB565,
    &MFVideoFormat_RGB8,
    &MFVideoFormat_Y216,
    &MFVideoFormat_v410,
    &MFVideoFormat_Y41P,
    &MFVideoFormat_Y41T,
    &MFVideoFormat_Y42T,
};
static const GUID *const output_types[] =
{
    &MFVideoFormat_YUY2,
    &MFVideoFormat_IYUV,
    &MFVideoFormat_I420,
    &MFVideoFormat_NV12,
    &MFVideoFormat_RGB24,
    &MFVideoFormat_ARGB32,
    &MFVideoFormat_RGB32,
    &MFVideoFormat_YV12,
    &MFVideoFormat_AYUV,
    &MFVideoFormat_RGB555,
    &MFVideoFormat_RGB565,
};

struct video_processor
{
    IMFTransform IMFTransform_iface;
    LONG refcount;

    IMFMediaType *input_type;
    IMFMediaType *output_type;
    IMFAttributes *attributes;
    IMFAttributes *output_attributes;

    struct wg_transform *wg_transform;
    struct wg_sample_queue *wg_sample_queue;
};

static HRESULT try_create_wg_transform(struct video_processor *impl)
{
    struct wg_format input_format, output_format;

    if (impl->wg_transform)
        wg_transform_destroy(impl->wg_transform);
    impl->wg_transform = NULL;

    mf_media_type_to_wg_format(impl->input_type, &input_format);
    if (input_format.major_type == WG_MAJOR_TYPE_UNKNOWN)
        return MF_E_INVALIDMEDIATYPE;

    mf_media_type_to_wg_format(impl->output_type, &output_format);
    if (output_format.major_type == WG_MAJOR_TYPE_UNKNOWN)
        return MF_E_INVALIDMEDIATYPE;

    if (!(impl->wg_transform = wg_transform_create(&input_format, &output_format)))
        return E_FAIL;

    return S_OK;
}

static struct video_processor *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct video_processor, IMFTransform_iface);
}

static HRESULT WINAPI video_processor_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IMFTransform))
        *out = &impl->IMFTransform_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI video_processor_AddRef(IMFTransform *iface)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&impl->refcount);

    TRACE("iface %p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI video_processor_Release(IMFTransform *iface)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&impl->refcount);

    TRACE("iface %p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (impl->wg_transform)
            wg_transform_destroy(impl->wg_transform);
        if (impl->input_type)
            IMFMediaType_Release(impl->input_type);
        if (impl->output_type)
            IMFMediaType_Release(impl->output_type);
        if (impl->attributes)
            IMFAttributes_Release(impl->attributes);
        if (impl->output_attributes)
            IMFAttributes_Release(impl->output_attributes);

        wg_sample_queue_destroy(impl->wg_sample_queue);
        free(impl);
    }

    return refcount;
}

static HRESULT WINAPI video_processor_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    TRACE("iface %p, input_minimum %p, input_maximum %p, output_minimum %p, output_maximum %p.\n",
            iface, input_minimum, input_maximum, output_minimum, output_maximum);
    *input_minimum = *input_maximum = *output_minimum = *output_maximum = 1;
    return S_OK;
}

static HRESULT WINAPI video_processor_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    TRACE("iface %p, inputs %p, outputs %p.\n", iface, inputs, outputs);
    *inputs = *outputs = 1;
    return S_OK;
}

static HRESULT WINAPI video_processor_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    FIXME("iface %p, input_size %lu, inputs %p, output_size %lu, outputs %p stub!\n", iface,
            input_size, inputs, output_size, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    UINT32 sample_size;
    UINT64 framesize;
    GUID subtype;
    HRESULT hr;

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    if (impl->input_type && SUCCEEDED(hr = IMFMediaType_GetGUID(impl->input_type, &MF_MT_SUBTYPE, &subtype))
            && SUCCEEDED(hr = IMFMediaType_GetUINT64(impl->input_type, &MF_MT_FRAME_SIZE, &framesize)))
        MFCalculateImageSize(&subtype, framesize >> 32, (UINT32)framesize, &sample_size);
    else
        sample_size = 0;

    info->dwFlags = 0;
    info->cbSize = sample_size;
    info->cbAlignment = 0;
    info->hnsMaxLatency = 0;
    info->cbMaxLookahead = 0;

    return S_OK;
}

static HRESULT WINAPI video_processor_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    UINT32 sample_size;
    UINT64 framesize;
    GUID subtype;
    HRESULT hr;

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    if (impl->output_type && SUCCEEDED(hr = IMFMediaType_GetGUID(impl->output_type, &MF_MT_SUBTYPE, &subtype))
            && SUCCEEDED(hr = IMFMediaType_GetUINT64(impl->output_type, &MF_MT_FRAME_SIZE, &framesize)))
        MFCalculateImageSize(&subtype, framesize >> 32, (UINT32)framesize, &sample_size);
    else
        sample_size = 0;

    info->dwFlags = 0;
    info->cbSize = sample_size;
    info->cbAlignment = 0;

    return S_OK;
}

static HRESULT WINAPI video_processor_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);

    FIXME("iface %p, attributes %p stub!\n", iface, attributes);

    IMFAttributes_AddRef((*attributes = impl->attributes));
    return S_OK;
}

static HRESULT WINAPI video_processor_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    FIXME("iface %p, id %#lx, attributes %p stub!\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);

    FIXME("iface %p, id %#lx, attributes %p stub!\n", iface, id, attributes);

    IMFAttributes_AddRef((*attributes = impl->output_attributes));
    return S_OK;
}

static HRESULT WINAPI video_processor_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    FIXME("iface %p, id %#lx stub!\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    FIXME("iface %p, streams %lu, ids %p stub!\n", iface, streams, ids);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    IMFMediaType *media_type;
    const GUID *subtype;
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    *type = NULL;

    if (index >= ARRAY_SIZE(input_types))
        return MF_E_NO_MORE_TYPES;
    subtype = input_types[index];

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, subtype)))
        goto done;

    IMFMediaType_AddRef((*type = media_type));

done:
    IMFMediaType_Release(media_type);
    return hr;
}

static HRESULT WINAPI video_processor_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    IMFMediaType *media_type;
    UINT64 frame_size;
    GUID subtype;
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    *type = NULL;

    if (!impl->input_type)
        return MF_E_NO_MORE_TYPES;

    if (FAILED(hr = IMFMediaType_GetGUID(impl->input_type, &MF_MT_SUBTYPE, &subtype))
            || FAILED(hr = IMFMediaType_GetUINT64(impl->input_type, &MF_MT_FRAME_SIZE, &frame_size)))
        return hr;

    if (index > ARRAY_SIZE(output_types))
        return MF_E_NO_MORE_TYPES;
    if (index > 0)
        subtype = *output_types[index - 1];

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &subtype)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, frame_size)))
        goto done;

    IMFMediaType_AddRef((*type = media_type));

done:
    IMFMediaType_Release(media_type);
    return hr;
}

static HRESULT WINAPI video_processor_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    GUID major, subtype;
    UINT64 frame_size;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major))
            || !IsEqualGUID(&major, &MFMediaType_Video))
        return E_INVALIDARG;
    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(hr = IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
        return hr;

    for (i = 0; i < ARRAY_SIZE(input_types); ++i)
        if (IsEqualGUID(&subtype, input_types[i]))
            break;
    if (i == ARRAY_SIZE(input_types))
        return MF_E_INVALIDMEDIATYPE;

    if (impl->input_type)
        IMFMediaType_Release(impl->input_type);
    IMFMediaType_AddRef((impl->input_type = type));

    if (impl->output_type && FAILED(hr = try_create_wg_transform(impl)))
    {
        IMFMediaType_Release(impl->input_type);
        impl->input_type = NULL;
    }

    return hr;
}

static HRESULT WINAPI video_processor_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    GUID major, subtype;
    UINT64 frame_size;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major))
            || !IsEqualGUID(&major, &MFMediaType_Video))
        return E_INVALIDARG;
    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return MF_E_INVALIDMEDIATYPE;
    if (FAILED(hr = IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
        return hr;

    for (i = 0; i < ARRAY_SIZE(output_types); ++i)
        if (IsEqualGUID(&subtype, output_types[i]))
            break;
    if (i == ARRAY_SIZE(output_types))
        return MF_E_INVALIDMEDIATYPE;

    if (impl->output_type)
        IMFMediaType_Release(impl->output_type);
    IMFMediaType_AddRef((impl->output_type = type));

    if (impl->input_type && FAILED(hr = try_create_wg_transform(impl)))
    {
        IMFMediaType_Release(impl->output_type);
        impl->output_type = NULL;
    }

    return hr;
}

static HRESULT WINAPI video_processor_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p.\n", iface, id, type);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (!impl->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    if (FAILED(hr = IMFMediaType_CopyAllItems(impl->input_type, (IMFAttributes *)*type)))
        IMFMediaType_Release(*type);

    return hr;
}

static HRESULT WINAPI video_processor_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p.\n", iface, id, type);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (!impl->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    if (FAILED(hr = IMFMediaType_CopyAllItems(impl->output_type, (IMFAttributes *)*type)))
        IMFMediaType_Release(*type);

    return hr;
}

static HRESULT WINAPI video_processor_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);

    FIXME("iface %p, id %#lx, flags %p stub!\n", iface, id, flags);

    if (!impl->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *flags = MFT_INPUT_STATUS_ACCEPT_DATA;
    return S_OK;
}

static HRESULT WINAPI video_processor_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);

    FIXME("iface %p, flags %p stub!\n", iface, flags);

    if (!impl->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    FIXME("iface %p, lower %I64d, upper %I64d stub!\n", iface, lower, upper);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("iface %p, id %#lx, event %p stub!\n", iface, id, event);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    FIXME("iface %p, message %#x, param %#Ix stub!\n", iface, message, param);
    return S_OK;
}

static HRESULT WINAPI video_processor_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    struct wg_sample *wg_sample;
    MFT_INPUT_STREAM_INFO info;
    HRESULT hr;

    TRACE("iface %p, id %#lx, sample %p, flags %#lx.\n", iface, id, sample, flags);

    if (FAILED(hr = IMFTransform_GetInputStreamInfo(iface, 0, &info)))
        return hr;

    if (!impl->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = wg_sample_create_mf(sample, &wg_sample)))
        return hr;

    return wg_transform_push_mf(impl->wg_transform, wg_sample,
            impl->wg_sample_queue);
}

static HRESULT WINAPI video_processor_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct video_processor *impl = impl_from_IMFTransform(iface);
    MFT_OUTPUT_STREAM_INFO info;
    struct wg_sample *wg_sample;
    HRESULT hr;

    TRACE("iface %p, flags %#lx, count %lu, samples %p, status %p.\n", iface, flags, count, samples, status);

    if (count != 1)
        return E_INVALIDARG;

    if (FAILED(hr = IMFTransform_GetOutputStreamInfo(iface, 0, &info)))
        return hr;

    if (!impl->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    samples[0].dwStatus = 0;
    if (!samples[0].pSample) return E_INVALIDARG;

    if (FAILED(hr = wg_sample_create_mf(samples[0].pSample, &wg_sample)))
        return hr;

    if (wg_sample->max_size < info.cbSize)
    {
        wg_sample_release(wg_sample);
        return MF_E_BUFFERTOOSMALL;
    }

    if (SUCCEEDED(hr = wg_transform_read_mf(impl->wg_transform, wg_sample, NULL)))
        wg_sample_queue_flush(impl->wg_sample_queue, false);
    wg_sample_release(wg_sample);

    if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
    {
        FIXME("Unexpected stream format change!\n");
        samples[0].dwStatus |= MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE;
    }

    return hr;
}

static const IMFTransformVtbl video_processor_vtbl =
{
    video_processor_QueryInterface,
    video_processor_AddRef,
    video_processor_Release,
    video_processor_GetStreamLimits,
    video_processor_GetStreamCount,
    video_processor_GetStreamIDs,
    video_processor_GetInputStreamInfo,
    video_processor_GetOutputStreamInfo,
    video_processor_GetAttributes,
    video_processor_GetInputStreamAttributes,
    video_processor_GetOutputStreamAttributes,
    video_processor_DeleteInputStream,
    video_processor_AddInputStreams,
    video_processor_GetInputAvailableType,
    video_processor_GetOutputAvailableType,
    video_processor_SetInputType,
    video_processor_SetOutputType,
    video_processor_GetInputCurrentType,
    video_processor_GetOutputCurrentType,
    video_processor_GetInputStatus,
    video_processor_GetOutputStatus,
    video_processor_SetOutputBounds,
    video_processor_ProcessEvent,
    video_processor_ProcessMessage,
    video_processor_ProcessInput,
    video_processor_ProcessOutput,
};

HRESULT video_processor_create(REFIID riid, void **ret)
{
    static const struct wg_format input_format =
    {
        .major_type = WG_MAJOR_TYPE_VIDEO,
        .u.video =
        {
            .format = WG_VIDEO_FORMAT_I420,
            .width = 1920,
            .height = 1080,
        },
    };
    static const struct wg_format output_format =
    {
        .major_type = WG_MAJOR_TYPE_VIDEO,
        .u.video =
        {
            .format = WG_VIDEO_FORMAT_NV12,
            .width = 1920,
            .height = 1080,
        },
    };
    struct wg_transform *transform;
    struct video_processor *impl;
    HRESULT hr;

    TRACE("riid %s, ret %p.\n", debugstr_guid(riid), ret);

    if (!(transform = wg_transform_create(&input_format, &output_format)))
    {
        ERR_(winediag)("GStreamer doesn't support video conversion, please install appropriate plugins.\n");
        return E_FAIL;
    }
    wg_transform_destroy(transform);

    if (!(impl = calloc(1, sizeof(*impl))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = MFCreateAttributes(&impl->attributes, 0)))
        goto failed;
    if (FAILED(hr = MFCreateAttributes(&impl->output_attributes, 0)))
        goto failed;
    if (FAILED(hr = wg_sample_queue_create(&impl->wg_sample_queue)))
        goto failed;

    impl->IMFTransform_iface.lpVtbl = &video_processor_vtbl;
    impl->refcount = 1;

    *ret = &impl->IMFTransform_iface;
    TRACE("Created %p\n", *ret);
    return S_OK;

failed:
    free(impl);
    return hr;
}
