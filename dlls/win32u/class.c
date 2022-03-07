/*
 * Window classes functions
 *
 * Copyright 1993, 1996, 2003 Alexandre Julliard
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 Juergen Schmied (jsch)
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

#if 0
#pragma makedep unix
#endif

#include <pthread.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(class);
WINE_DECLARE_DEBUG_CHANNEL(win);

#define MAX_WINPROCS  4096
#define WINPROC_PROC16  ((WINDOWPROC *)1)  /* placeholder for 16-bit window procs */

static WINDOWPROC winproc_array[MAX_WINPROCS];
static UINT winproc_used = NB_BUILTIN_WINPROCS;
static pthread_mutex_t winproc_lock = PTHREAD_MUTEX_INITIALIZER;

static struct list class_list = LIST_INIT( class_list );

static HINSTANCE user32_module;

/* find an existing winproc for a given function and type */
/* FIXME: probably should do something more clever than a linear search */
static WINDOWPROC *find_winproc( WNDPROC func, BOOL ansi )
{
    unsigned int i;

    for (i = 0; i < NB_BUILTIN_AW_WINPROCS; i++)
    {
        /* match either proc, some apps confuse A and W */
        if (winproc_array[i].procA != func && winproc_array[i].procW != func) continue;
        return &winproc_array[i];
    }
    for (i = NB_BUILTIN_AW_WINPROCS; i < winproc_used; i++)
    {
        if (ansi && winproc_array[i].procA != func) continue;
        if (!ansi && winproc_array[i].procW != func) continue;
        return &winproc_array[i];
    }
    return NULL;
}

/* return the window proc for a given handle, or NULL for an invalid handle,
 * or WINPROC_PROC16 for a handle to a 16-bit proc. */
WINDOWPROC *get_winproc_ptr( WNDPROC handle )
{
    UINT index = LOWORD(handle);
    if ((ULONG_PTR)handle >> 16 != WINPROC_HANDLE) return NULL;
    if (index >= MAX_WINPROCS) return WINPROC_PROC16;
    if (index >= winproc_used) return NULL;
    return &winproc_array[index];
}

/* create a handle for a given window proc */
static inline WNDPROC proc_to_handle( WINDOWPROC *proc )
{
    return (WNDPROC)(ULONG_PTR)((proc - winproc_array) | (WINPROC_HANDLE << 16));
}

/* allocate and initialize a new winproc */
static inline WINDOWPROC *alloc_winproc_ptr( WNDPROC func, BOOL ansi )
{
    WINDOWPROC *proc;

    /* check if the function is already a win proc */
    if (!func) return NULL;
    if ((proc = get_winproc_ptr( func ))) return proc;

    pthread_mutex_lock( &winproc_lock );

    /* check if we already have a winproc for that function */
    if (!(proc = find_winproc( func, ansi )))
    {
        if (winproc_used < MAX_WINPROCS)
        {
            proc = &winproc_array[winproc_used++];
            if (ansi) proc->procA = func;
            else proc->procW = func;
            TRACE_(win)( "allocated %p for %c %p (%d/%d used)\n",
                         proc_to_handle(proc), ansi ? 'A' : 'W', func,
                         winproc_used, MAX_WINPROCS );
        }
        else WARN_(win)( "too many winprocs, cannot allocate one for %p\n", func );
    }
    else TRACE_(win)( "reusing %p for %p\n", proc_to_handle(proc), func );

    pthread_mutex_unlock( &winproc_lock );
    return proc;
}

/**********************************************************************
 *	     alloc_winproc
 *
 * Allocate a window procedure for a window or class.
 *
 * Note that allocated winprocs are never freed; the idea is that even if an app creates a
 * lot of windows, it will usually only have a limited number of window procedures, so the
 * array won't grow too large, and this way we avoid the need to track allocations per window.
 */
WNDPROC alloc_winproc( WNDPROC func, BOOL ansi )
{
    WINDOWPROC *proc;

    if (!(proc = alloc_winproc_ptr( func, ansi ))) return func;
    if (proc == WINPROC_PROC16) return func;
    return proc_to_handle( proc );
}

/* Get a window procedure pointer that can be passed to the Windows program. */
static WNDPROC get_winproc( WNDPROC proc, BOOL ansi )
{
    WINDOWPROC *ptr = get_winproc_ptr( proc );

    if (!ptr || ptr == WINPROC_PROC16) return proc;
    if (ansi)
    {
        if (ptr->procA) return ptr->procA;
        return proc;
    }
    else
    {
        if (ptr->procW) return ptr->procW;
        return proc;
    }
}

/***********************************************************************
 *	     NtUserInitializeClientPfnArrays   (win32u.@)
 */
NTSTATUS WINAPI NtUserInitializeClientPfnArrays( const struct user_client_procs *client_procsA,
                                                 const struct user_client_procs *client_procsW,
                                                 const void *client_workers, HINSTANCE user_module )
{
    winproc_array[WINPROC_BUTTON].procA = client_procsA->pButtonWndProc;
    winproc_array[WINPROC_BUTTON].procW = client_procsW->pButtonWndProc;
    winproc_array[WINPROC_COMBO].procA = client_procsA->pComboWndProc;
    winproc_array[WINPROC_COMBO].procW = client_procsW->pComboWndProc;
    winproc_array[WINPROC_DEFWND].procA = client_procsA->pDefWindowProc;
    winproc_array[WINPROC_DEFWND].procW = client_procsW->pDefWindowProc;
    winproc_array[WINPROC_DIALOG].procA = client_procsA->pDefDlgProc;
    winproc_array[WINPROC_DIALOG].procW = client_procsW->pDefDlgProc;
    winproc_array[WINPROC_EDIT].procA = client_procsA->pEditWndProc;
    winproc_array[WINPROC_EDIT].procW = client_procsW->pEditWndProc;
    winproc_array[WINPROC_LISTBOX].procA = client_procsA->pListBoxWndProc;
    winproc_array[WINPROC_LISTBOX].procW = client_procsW->pListBoxWndProc;
    winproc_array[WINPROC_MDICLIENT].procA = client_procsA->pMDIClientWndProc;
    winproc_array[WINPROC_MDICLIENT].procW = client_procsW->pMDIClientWndProc;
    winproc_array[WINPROC_SCROLLBAR].procA = client_procsA->pScrollBarWndProc;
    winproc_array[WINPROC_SCROLLBAR].procW = client_procsW->pScrollBarWndProc;
    winproc_array[WINPROC_STATIC].procA = client_procsA->pStaticWndProc;
    winproc_array[WINPROC_STATIC].procW = client_procsW->pStaticWndProc;
    winproc_array[WINPROC_IME].procA = client_procsA->pImeWndProc;
    winproc_array[WINPROC_IME].procW = client_procsW->pImeWndProc;
    winproc_array[WINPROC_DESKTOP].procA = client_procsA->pDesktopWndProc;
    winproc_array[WINPROC_DESKTOP].procW = client_procsW->pDesktopWndProc;
    winproc_array[WINPROC_ICONTITLE].procA = client_procsA->pIconTitleWndProc;
    winproc_array[WINPROC_ICONTITLE].procW = client_procsW->pIconTitleWndProc;
    winproc_array[WINPROC_MENU].procA = client_procsA->pPopupMenuWndProc;
    winproc_array[WINPROC_MENU].procW = client_procsW->pPopupMenuWndProc;
    winproc_array[WINPROC_MESSAGE].procA = client_procsA->pMessageWndProc;
    winproc_array[WINPROC_MESSAGE].procW = client_procsW->pMessageWndProc;

    user32_module = user_module;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           get_int_atom_value
 */
static ATOM get_int_atom_value( UNICODE_STRING *name )
{
    const WCHAR *ptr = name->Buffer;
    const WCHAR *end = ptr + name->Length / sizeof(WCHAR);
    UINT ret = 0;

    if (IS_INTRESOURCE(ptr)) return LOWORD(ptr);

    if (*ptr++ != '#') return 0;
    while (ptr < end)
    {
        if (*ptr < '0' || *ptr > '9') return 0;
        ret = ret * 10 + *ptr++ - '0';
        if (ret > 0xffff) return 0;
    }
    return ret;
}

/***********************************************************************
 *           get_class_ptr
 */
static CLASS *get_class_ptr( HWND hwnd, BOOL write_access )
{
    WND *ptr = get_win_ptr( hwnd );

    if (ptr)
    {
        if (ptr != WND_OTHER_PROCESS && ptr != WND_DESKTOP) return ptr->class;
        if (!write_access) return OBJ_OTHER_PROCESS;

        /* modifying classes in other processes is not allowed */
        if (ptr == WND_DESKTOP || is_window( hwnd ))
        {
            SetLastError( ERROR_ACCESS_DENIED );
            return NULL;
        }
    }
    SetLastError( ERROR_INVALID_WINDOW_HANDLE );
    return NULL;
}

/***********************************************************************
 *           release_class_ptr
 */
static void release_class_ptr( CLASS *ptr )
{
    user_unlock();
}

static CLASS *find_class( HINSTANCE module, UNICODE_STRING *name )
{
    ATOM atom = get_int_atom_value( name );
    ULONG_PTR instance = (UINT_PTR)module & ~0xffff;
    CLASS *class;

    user_lock();
    LIST_FOR_EACH_ENTRY( class, &class_list, CLASS, entry )
    {
        if (atom)
        {
            if (class->atomName != atom) continue;
        }
        else
        {
            if (wcsnicmp( class->name, name->Buffer, name->Length / sizeof(WCHAR) )) continue;
        }
        if (!class->local || !module || (class->instance & ~0xffff) == instance)
        {
            TRACE( "%s %lx -> %p\n", debugstr_us(name), instance, class );
            return class;
        }
    }
    user_unlock();
    return NULL;
}

/***********************************************************************
 *	     NtUserRegisterClassExWOW   (win32u.@)
 */
ATOM WINAPI NtUserRegisterClassExWOW( const WNDCLASSEXW *wc, UNICODE_STRING *name, UNICODE_STRING *version,
                                      struct client_menu_name *client_menu_name, DWORD fnid,
                                      DWORD flags, DWORD *wow )
{
    const BOOL is_builtin = fnid, ansi = flags;
    HINSTANCE instance;
    CLASS *class;
    ATOM atom;
    BOOL ret;

    /* create the desktop window to trigger builtin class registration */
    if (!is_builtin && user_callbacks) user_callbacks->pGetDesktopWindow();

    if (wc->cbSize != sizeof(*wc) || wc->cbClsExtra < 0 || wc->cbWndExtra < 0 ||
        (!is_builtin && wc->hInstance == user32_module))  /* we can't register a class for user32 */
    {
         SetLastError( ERROR_INVALID_PARAMETER );
         return 0;
    }
    if (!(instance = wc->hInstance)) instance = NtCurrentTeb()->Peb->ImageBaseAddress;

    TRACE( "name=%s hinst=%p style=0x%x clExtr=0x%x winExtr=0x%x\n",
           debugstr_us(name), instance, wc->style, wc->cbClsExtra, wc->cbWndExtra );

    /* Fix the extra bytes value */

    if (wc->cbClsExtra > 40)  /* Extra bytes are limited to 40 in Win32 */
        WARN( "Class extra bytes %d is > 40\n", wc->cbClsExtra);
    if (wc->cbWndExtra > 40)  /* Extra bytes are limited to 40 in Win32 */
        WARN("Win extra bytes %d is > 40\n", wc->cbWndExtra );

    if (!(class = calloc( 1, sizeof(CLASS) + wc->cbClsExtra ))) return 0;

    class->atomName = get_int_atom_value( name );
    class->basename = class->name;
    if (!class->atomName && name)
    {
        memcpy( class->name, name->Buffer, name->Length );
        class->name[name->Length / sizeof(WCHAR)] = 0;
        class->basename += version->Length / sizeof(WCHAR);
    }
    else
    {
        UNICODE_STRING str = { .MaximumLength = sizeof(class->name), .Buffer = class->name };
        NtUserGetAtomName( class->atomName, &str );
    }

    class->style      = wc->style;
    class->local      = !is_builtin && !(wc->style & CS_GLOBALCLASS);
    class->cbWndExtra = wc->cbWndExtra;
    class->cbClsExtra = wc->cbClsExtra;
    class->instance   = (UINT_PTR)instance;

    SERVER_START_REQ( create_class )
    {
        req->local      = class->local;
        req->style      = class->style;
        req->instance   = class->instance;
        req->extra      = class->cbClsExtra;
        req->win_extra  = class->cbWndExtra;
        req->client_ptr = wine_server_client_ptr( class );
        req->atom       = class->atomName;
        req->name_offset = version->Length / sizeof(WCHAR);
        if (!req->atom && name) wine_server_add_data( req, name->Buffer, name->Length );
        ret = !wine_server_call_err( req );
        class->atomName = reply->atom;
    }
    SERVER_END_REQ;
    if (!ret)
    {
        free( class );
        return 0;
    }

    /* Other non-null values must be set by caller */

    user_lock();
    if (class->local) list_add_head( &class_list, &class->entry );
    else list_add_tail( &class_list, &class->entry );

    atom = class->atomName;

    TRACE( "name=%s->%s atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d class=%p\n",
           debugstr_w(wc->lpszClassName), debugstr_us(name), atom, wc->lpfnWndProc, instance,
           wc->hbrBackground, wc->style, wc->cbClsExtra, wc->cbWndExtra, class );

    class->hIcon         = wc->hIcon;
    class->hIconSm       = wc->hIconSm;
    class->hCursor       = wc->hCursor;
    class->hbrBackground = wc->hbrBackground;
    class->winproc       = alloc_winproc( wc->lpfnWndProc, ansi );
    class->menu_name     = *client_menu_name;
    if (wc->hIcon && !wc->hIconSm && user_callbacks)
        class->hIconSmIntern = user_callbacks->pCopyImage( wc->hIcon, IMAGE_ICON,
                                                           get_system_metrics( SM_CXSMICON ),
                                                           get_system_metrics( SM_CYSMICON ),
                                                           LR_COPYFROMRESOURCE );
    release_class_ptr( class );
    return atom;
}

/***********************************************************************
 *	     NtUserUnregisterClass   (win32u.@)
 */
BOOL WINAPI NtUserUnregisterClass( UNICODE_STRING *name, HINSTANCE instance,
                                   struct client_menu_name *client_menu_name )
{
    CLASS *class = NULL;

    if (user_callbacks) /* create the desktop window to trigger builtin class registration */
        user_callbacks->pGetDesktopWindow();

    SERVER_START_REQ( destroy_class )
    {
        req->instance = wine_server_client_ptr( instance );
        if (!(req->atom = get_int_atom_value( name )) && name->Length)
            wine_server_add_data( req, name->Buffer, name->Length );
        if (!wine_server_call_err( req )) class = wine_server_get_ptr( reply->client_ptr );
    }
    SERVER_END_REQ;
    if (!class) return FALSE;

    TRACE( "%p\n", class );

    user_lock();
    if (class->dce && user_callbacks) user_callbacks->free_dce( class->dce, 0 );
    list_remove( &class->entry );
    if (class->hbrBackground > (HBRUSH)(COLOR_GRADIENTINACTIVECAPTION + 1))
        NtGdiDeleteObjectApp( class->hbrBackground );
    NtUserDestroyCursor( class->hIconSmIntern, 0 );
    free( class );
    user_unlock();
    return TRUE;
}

/***********************************************************************
 *	     NtUserGetClassInfo   (win32u.@)
 */
ATOM WINAPI NtUserGetClassInfoEx( HINSTANCE instance, UNICODE_STRING *name, WNDCLASSEXW *wc,
                                  struct client_menu_name *menu_name, BOOL ansi )
{
    static const WCHAR messageW[] = {'M','e','s','s','a','g','e'};
    CLASS *class;
    ATOM atom;

    /* create the desktop window to trigger builtin class registration */
    if (name->Buffer != (const WCHAR *)DESKTOP_CLASS_ATOM &&
        (IS_INTRESOURCE(name->Buffer) || name->Length != sizeof(messageW) ||
         wcsnicmp( name->Buffer, messageW, ARRAYSIZE(messageW) )))
        user_callbacks->pGetDesktopWindow();

    if (!(class = find_class( instance, name ))) return 0;

    if (wc)
    {
        wc->style         = class->style;
        wc->lpfnWndProc   = get_winproc( class->winproc, ansi );
        wc->cbClsExtra    = class->cbClsExtra;
        wc->cbWndExtra    = class->cbWndExtra;
        wc->hInstance     = (instance == user32_module) ? 0 : instance;
        wc->hIcon         = class->hIcon;
        wc->hIconSm       = class->hIconSm ? class->hIconSm : class->hIconSmIntern;
        wc->hCursor       = class->hCursor;
        wc->hbrBackground = class->hbrBackground;
        wc->lpszMenuName  = ansi ? (const WCHAR *)class->menu_name.nameA : class->menu_name.nameW;
        wc->lpszClassName = name->Buffer;
    }

    if (menu_name) *menu_name = class->menu_name;
    atom = class->atomName;
    release_class_ptr( class );
    return atom;
}

/***********************************************************************
 *	     NtUserGetAtomName   (win32u.@)
 */
ULONG WINAPI NtUserGetAtomName( ATOM atom, UNICODE_STRING *name )
{
    char buf[sizeof(ATOM_BASIC_INFORMATION) + MAX_ATOM_LEN * sizeof(WCHAR)];
    ATOM_BASIC_INFORMATION *abi = (ATOM_BASIC_INFORMATION *)buf;
    UINT size;

    if (!set_ntstatus( NtQueryInformationAtom( atom, AtomBasicInformation,
                                               buf, sizeof(buf), NULL )))
        return 0;

    if (name->MaximumLength < sizeof(WCHAR))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    size = min( abi->NameLength, name->MaximumLength - sizeof(WCHAR) );
    if (size) memcpy( name->Buffer, abi->Name, size );
    name->Buffer[size / sizeof(WCHAR)] = 0;
    return size / sizeof(WCHAR);
}

/***********************************************************************
 *	     NtUserGetClassName   (win32u.@)
 */
INT WINAPI NtUserGetClassName( HWND hwnd, BOOL real, UNICODE_STRING *name )
{
    CLASS *class;
    int ret;

    TRACE( "%p %x %p\n", hwnd, real, name );

    if (name->MaximumLength <= sizeof(WCHAR))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    if (!(class = get_class_ptr( hwnd, FALSE ))) return 0;

    if (class == OBJ_OTHER_PROCESS)
    {
        ATOM atom = 0;

        SERVER_START_REQ( set_class_info )
        {
            req->window = wine_server_user_handle( hwnd );
            req->flags = 0;
            req->extra_offset = -1;
            req->extra_size = 0;
            if (!wine_server_call_err( req ))
                atom = reply->base_atom;
        }
        SERVER_END_REQ;

        return NtUserGetAtomName( atom, name );
    }

    ret = min( name->MaximumLength / sizeof(WCHAR) - 1, lstrlenW(class->basename) );
    if (ret) memcpy( name->Buffer, class->basename, ret * sizeof(WCHAR) );
    name->Buffer[ret] = 0;
    release_class_ptr( class );
    return ret;
}