/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#define COBJMACROS

#include <wine/test.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "mshtml.h"
#include "initguid.h"
#include "optary.h"

static void test_HTMLLoadOptions(void)
{
    IHtmlLoadOptions *loadopts;
    BYTE buf[100];
    DWORD size, i, data = 0xdeadbeef;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_HTMLLoadOptions, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IHtmlLoadOptions, (void**)&loadopts);
    ok(hres == S_OK, "creating HTMLLoadOptions failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    for(i=0; i <= HTMLLOADOPTION_FRAMELOAD+3; i++) {
        size = 0xdeadbeef;
        memset(buf, 0xdd, sizeof(buf));
        hres = IHtmlLoadOptions_QueryOption(loadopts, i, NULL, &size);
        ok(hres == S_OK, "QueryOption failed: %08lx\n", hres);
        ok(size == 0, "size = %ld\n", size);
        ok(buf[0] == 0xdd, "buf changed\n");
    }

    size = 0xdeadbeef;
    hres = IHtmlLoadOptions_QueryOption(loadopts, HTMLLOADOPTION_CODEPAGE, NULL, &size);
    ok(hres == S_OK, "QueryOption failed: %08lx\n", hres);
    ok(size == 0, "size = %ld\n", size);

    hres = IHtmlLoadOptions_SetOption(loadopts, HTMLLOADOPTION_CODEPAGE, &data, sizeof(data));
    ok(hres == S_OK, "SetOption failed: %08lx\n", hres);

    size = sizeof(data);
    memset(buf, 0xdd, sizeof(buf));
    hres = IHtmlLoadOptions_QueryOption(loadopts, HTMLLOADOPTION_CODEPAGE, buf, &size);
    ok(hres == S_OK, "QueryOption failed: %08lx\n", hres);
    ok(size == sizeof(data), "size = %ld\n", size);
    ok(*(DWORD*)buf == data, "unexpected buf\n");

    size = sizeof(data)-1;
    memset(buf, 0xdd, sizeof(buf));
    hres = IHtmlLoadOptions_QueryOption(loadopts, HTMLLOADOPTION_CODEPAGE, buf, &size);
    ok(hres == E_FAIL, "QueryOption failed: %08lx\n", hres);
    ok(size == sizeof(data) || !size, "size = %ld\n", size);
    ok(buf[0] == 0xdd, "buf changed\n");

    data = 100;
    hres = IHtmlLoadOptions_SetOption(loadopts, HTMLLOADOPTION_CODEPAGE, &data, 0);
    ok(hres == S_OK, "SetOption failed: %08lx\n", hres);

    size = 0xdeadbeef; 
    memset(buf, 0xdd, sizeof(buf));
    hres = IHtmlLoadOptions_QueryOption(loadopts, HTMLLOADOPTION_CODEPAGE, buf, &size);
    ok(hres == S_OK, "QueryOption failed: %08lx\n", hres);
    ok(size == 0, "size = %ld\n", size);
    ok(buf[0] == 0xdd, "buf changed\n");

    hres = IHtmlLoadOptions_SetOption(loadopts, HTMLLOADOPTION_CODEPAGE, NULL, 0);
    ok(hres == S_OK, "SetOption failed: %08lx\n", hres);

    hres = IHtmlLoadOptions_SetOption(loadopts, 1000, &data, sizeof(data));
    ok(hres == S_OK, "SetOption failed: %08lx\n", hres);

    size = sizeof(data);
    memset(buf, 0xdd, sizeof(buf));
    hres = IHtmlLoadOptions_QueryOption(loadopts, 1000, buf, &size);
    ok(hres == S_OK, "QueryOption failed: %08lx\n", hres);
    ok(size == sizeof(data), "size = %ld\n", size);
    ok(*(DWORD*)buf == data, "unexpected buf\n");

    hres = IHtmlLoadOptions_SetOption(loadopts, 1000, buf, sizeof(buf));
    ok(hres == S_OK, "SetOption failed: %08lx\n", hres);

    size = 0xdeadbeef;
    hres = IHtmlLoadOptions_QueryOption(loadopts, 1000, buf, &size);
    ok(hres == S_OK, "QueryOption failed: %08lx\n", hres);
    ok(size == sizeof(buf), "size = %ld\n", size);

    IHtmlLoadOptions_Release(loadopts);
}

static IHTMLDocument2 *create_doc_from_url(const WCHAR *start_url)
{
    BSTR url;
    IBindCtx *bc;
    IMoniker *url_mon;
    IPersistMoniker *persist_mon;
    IHTMLDocument2 *doc;
    HRESULT hres;

    hres = CreateBindCtx(0, &bc);
    ok(hres == S_OK, "CreateBindCtx failed: 0x%08lx\n", hres);

    url = SysAllocString(start_url);
    hres = CreateURLMoniker(NULL, url, &url_mon);
    ok(hres == S_OK, "CreateURLMoniker failed: 0x%08lx\n", hres);

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER, &IID_IHTMLDocument2,
            (void**)&doc);
    ok(hres == S_OK, "CoCreateInstance failed: 0x%08lx\n", hres);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistMoniker,
            (void**)&persist_mon);
    ok(hres == S_OK, "IHTMLDocument2_QueryInterface failed: 0x%08lx\n", hres);

    hres = IPersistMoniker_Load(persist_mon, FALSE, url_mon, bc,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE);
    ok(hres == S_OK, "IPersistMoniker_Load failed: 0x%08lx\n", hres);

    IPersistMoniker_Release(persist_mon);
    IMoniker_Release(url_mon);
    IBindCtx_Release(bc);
    SysFreeString(url);
    return doc;
}

static HRESULT get_localstorage(IHTMLDocument2 *doc, IHTMLStorage **storage)
{
    IHTMLWindow2 *window;
    IHTMLWindow6 *window6;
    HRESULT hres;

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08lx\n", hres);
    ok(window != NULL, "window == NULL\n");

    hres = IHTMLWindow2_QueryInterface(window, &IID_IHTMLWindow6, (void**)&window6);
    IHTMLWindow2_Release(window);
    if(FAILED(hres)) {
        win_skip("IHTMLWindow6 not supported\n");
        return hres;
    }

    hres = IHTMLWindow6_get_localStorage(window6, storage);
    ok(hres == S_OK, "get_localStorage failed: %08lx\n", hres);
    ok(*storage != NULL, "*storage == NULL\n");

    IHTMLWindow6_Release(window6);
    return hres;
}

static void test_HTMLStorage(void)
{
    IHTMLDocument2 *doc, *doc2;
    IHTMLStorage *storage, *storage2;
    VARIANT var;
    BSTR key, value;
    HRESULT hres;

    doc = create_doc_from_url(L"http://www.codeweavers.com/");
    doc2 = create_doc_from_url(L"http://www.codeweavers.com/");

    hres = get_localstorage(doc, &storage);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = get_localstorage(doc2, &storage2);
    ok(hres == S_OK, "got %08lx\n", hres);

    key = SysAllocString(L"");
    V_VT(&var) = 0xdead;
    hres = IHTMLStorage_getItem(storage, key, &var);
    ok(hres == S_OK, "getItem failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_NULL, "got %d\n", V_VT(&var));

    key = SysAllocString(L"undefined");
    hres = IHTMLStorage_removeItem(storage, key);
    ok(hres == S_OK, "removeItem failed: %08lx\n", hres);

    value = SysAllocString(L"null");
    hres = IHTMLStorage_setItem(storage, key, value);
    ok(hres == S_OK, "setItem failed: %08lx\n", hres);

    V_VT(&var) = 0xdead;
    hres = IHTMLStorage_getItem(storage, key, &var);
    ok(hres == S_OK, "getItem failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_BSTR, "got %d\n", V_VT(&var));
    if (V_VT(&var) == VT_BSTR) ok(!wcscmp(V_BSTR(&var), L"null"), "got %s\n", wine_dbgstr_w(V_BSTR(&var)));
    hres = IHTMLStorage_removeItem(storage, key);
    ok(hres == S_OK, "removeItem failed: %08lx\n", hres);
    SysFreeString(key);
    SysFreeString(value);

    key = SysAllocString(L"winetest");
    value = SysAllocString(L"winetest");
    hres = IHTMLStorage_setItem(storage, key, value);
    ok(hres == S_OK, "setItem failed: %08lx\n", hres);

    /* retrieve value from different storage instance */
    V_VT(&var) = 0xdead;
    hres = IHTMLStorage_getItem(storage2, key, &var);
    ok(hres == S_OK, "getItem failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_BSTR, "got %d\n", V_VT(&var));
    if (V_VT(&var) == VT_BSTR) ok(!wcscmp(V_BSTR(&var), L"winetest"), "got %s\n", wine_dbgstr_w(V_BSTR(&var)));

    hres = IHTMLStorage_removeItem(storage, key);
    ok(hres == S_OK, "removeItem failed: %08lx\n", hres);

    V_VT(&var) = 0xdead;
    hres = IHTMLStorage_getItem(storage2, key, &var);
    ok(hres == S_OK, "getItem failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_NULL, "got %d\n", V_VT(&var));
    SysFreeString(key);
    SysFreeString(value);

    /* item doesn't exist */
    key = SysAllocString(L"nosuch");
    hres = IHTMLStorage_removeItem(storage, key);
    ok(hres == S_OK, "removeItem failed: %08lx\n", hres);

    V_VT(&var) = 0xdead;
    hres = IHTMLStorage_getItem(storage, key, &var);
    ok(hres == S_OK, "getItem failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_NULL, "got %d\n", V_VT(&var));

    hres = IHTMLStorage_getItem(storage, NULL, NULL);
    ok(hres == E_POINTER, "getItem returned: %08lx\n", hres);

    hres = IHTMLStorage_getItem(storage, key, NULL);
    ok(hres == E_POINTER, "getItem failed: %08lx\n", hres);
    SysFreeString(key);

    V_VT(&var) = 0xdead;
    hres = IHTMLStorage_getItem(storage, NULL, &var);
    ok(hres == S_OK, "getItem returned: %08lx\n", hres);
    ok(V_VT(&var) == VT_NULL, "got %d\n", V_VT(&var));

    hres = IHTMLStorage_setItem(storage, NULL, NULL);
    ok(hres == S_OK, "setItem failed: %08lx\n", hres);

    key = SysAllocString(L"winetest2");
    hres = IHTMLStorage_setItem(storage, key, NULL);
    ok(hres == S_OK, "setItem failed: %08lx\n", hres);

    hres = IHTMLStorage_setItem(storage, NULL, key);
    ok(hres == S_OK, "setItem failed: %08lx\n", hres);

    hres = IHTMLStorage_removeItem(storage, key);
    ok(hres == S_OK, "removeItem failed: %08lx\n", hres);
    SysFreeString(key);

    hres = IHTMLStorage_removeItem(storage, NULL);
    ok(hres == S_OK, "removeItem failed: %08lx\n", hres);

    /* Unicode characters */
    key = SysAllocString(L"winetest");
    value = SysAllocString(L"\x20ac");
    hres = IHTMLStorage_setItem(storage, key, value);
    ok(hres == S_OK, "setItem failed: %08lx\n", hres);
    hres = IHTMLStorage_removeItem(storage, key);
    ok(hres == S_OK, "removeItem failed: %08lx\n", hres);
    SysFreeString(value);
    SysFreeString(key);

    key = SysAllocString(L"\x20ac");
    value = SysAllocString(L"winetest");
    hres = IHTMLStorage_setItem(storage, key, value);
    ok(hres == S_OK, "setItem failed: %08lx\n", hres);
    hres = IHTMLStorage_removeItem(storage, key);
    ok(hres == S_OK, "removeItem failed: %08lx\n", hres);
    SysFreeString(value);
    SysFreeString(key);

    IHTMLStorage_Release(storage);
    IHTMLStorage_Release(storage2);
    IHTMLDocument2_Release(doc);
    IHTMLDocument2_Release(doc2);
}

START_TEST(misc)
{
    CoInitialize(NULL);

    test_HTMLLoadOptions();
    test_HTMLStorage();

    CoUninitialize();
}
