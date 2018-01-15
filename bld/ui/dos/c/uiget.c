/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include <dos.h>
#include "uidef.h"
#include "biosui.h"
#include "uiforce.h"


void UIAPI uiflush( void )
/*************************/
{
    uiflushevent();
    flushkey();
}

unsigned long uiclock( void )
{
    return( *(unsigned long __far *)firstmeg( BIOS_PAGE, SYSTEM_CLOCK ) );
}

ui_event UIAPI uieventsource( bool update )
/*****************************************/
{
    register ui_event       ui_ev;
    static   int            ReturnIdle = 1;
    unsigned long           start;

    start = uiclock();
    for( ; ; ) {
        ui_ev = forcedevent();
        if( ui_ev > EV_NO_EVENT )
            break;
        ui_ev = mouseevent();
        if( ui_ev > EV_NO_EVENT ) {
            break;
        }
        ui_ev = keyboardevent();
        if( ui_ev > EV_NO_EVENT ) {
            uihidemouse();
            break;
        }
        if( ReturnIdle ) {
            ReturnIdle--;
            ui_ev = EV_IDLE;
            goto done;
        } else {
            if( !UIData->no_idle_int )
                IdleInterrupt();
            if( update && !UIData->no_refresh )
                uirefresh();
            if( uiclock() - start >= UIData->tick_delay ) {
                ui_ev = EV_CLOCK_TICK;
                goto done;
            } else if( UIData->busy_wait ) {
                ui_ev = EV_SINK;
                goto done;
            }
        }
    }
    ReturnIdle = 1;
    UIData->no_refresh = false;
done:
    return( uieventsourcehook( ui_ev ) );
}

ui_event UIAPI uiget( void )
/**************************/
{
    return( uieventsource( true ) );
}
