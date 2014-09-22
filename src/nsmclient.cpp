/*
 *  Copyright (c) 2013 John Hammen
 *
 *  This file is part of SDDM.
 *
 *  SDDM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SDDM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with SDDM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "nsmclient.h"
#include <iostream>

using namespace std;

int sddm_nsm_open ( const char *name, const char *display_name,
                    const char *client_id, char **out_msg, void *userdata )
{
    NSMClient *nsmClient = static_cast<NSMClient *> (userdata);
    nsmClient->open(name, display_name, client_id);
    return ERR_OK;
}

int sddm_nsm_save (char **out_msg, void *userdata )
{
    NSMClient *nsmClient = static_cast<NSMClient *> (userdata);
    nsmClient->save();
    return ERR_OK;
}

bool NSMClient::connectToSession()
{
    const char *nsm_url = getenv( "NSM_URL" );

    if ( nsm_url )
    {
        nsm = nsm_new();

        nsm_set_open_callback( nsm, sddm_nsm_open, this );
        nsm_set_save_callback( nsm, sddm_nsm_save, this );

        if ( 0 == nsm_init_thread(nsm, nsm_url ) )
        {
            nsm_thread_start(nsm);           
            return true;
        }
        else
        {
            nsm_free( nsm );
            nsm = 0;
        }
    }
    return false;
}

void NSMClient::announce(char *argv0)
{
    nsm_send_announce( nsm, "sddm", ":dirty:", argv0);
}

void NSMClient::open(const char *name, const char *displayName, const char *clientId)
{  
    emit nsmOpen(name, displayName, clientId);
}


