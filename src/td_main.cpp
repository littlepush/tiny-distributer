/*
    tiny-distributer -- a C++ tcp distributer application for Linux/Windows/iOS
    Copyright (C) 2014  Push Chen

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    You can connect me by email: littlepush@gmail.com, 
    or @me on twitter: @littlepush
*/

#include <list>
#include <vector>
#include <map>
#include "configs.h"
#include "thread.h"
#include "tcprelay.h"
#include "redirect.h"
#include "socks5.h"
#include "backdoor.h"

int main( int argc, char * argv[] ) {

    if ( argc < 3 ) {
        pid_t _pid = fork();
        if ( _pid < 0 ) {
            cerr << "Failed to create child process." << endl;
            //close_config_file(_config);
            return 1;
        }
        if ( _pid > 0 ) {
            // Has create the child process.
            // close_config_file(_config);
            return 0;
        }

        if ( setsid() < 0 ) {
            cerr << "failed to set session leader for child process." << endl;
            // close_config_file(_config);
            return 3;
        }
    }

    if ( argc < 2 ) {
        cerr << "must specified the config file path." << endl;
        return 1;
    }
	
	// Hang current process
	set_signal_handler();

	// Wait for kill signal
	wait_for_exit_signal();

    return 0;
}

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
