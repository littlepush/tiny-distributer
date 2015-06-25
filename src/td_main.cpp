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

#include "configs.h"
#include "thread.h"
#include "tcprelay.h"
#include "redirect.h"
#include "socks5.h"
#include "backdoor.h"

void tiny_distributer_worker( ) {
	register_this_thread();

	vector<sl_event> _event_list;
	// Loop to get all events
	while ( this_thread_is_running() ) {
		sl_poller::server().fetch_events(_event_list);
		for ( auto &_event : _event_list ) {
			if ( _event.event == SL_EVENT_FAILED ) {
				shared_ptr<td_service> _psvr = services_by_maintaining_socket(_event.so);
				if ( _psvr != nullptr ) _psvr->close_socket(_event.so);
			} else if ( _event.event == SL_EVENT_ACCEPT ) {
				shared_ptr<td_service> _psvr = services_by_socket(_event.source);
				if ( _psvr != nullptr ) _psvr->accept_new_incoming(_event.so);
			} else {
				shared_ptr<td_service> _psvr = services_by_maintaining_socket(_event.so);
				if ( _psvr != nullptr ) _psvr->socket_has_data_incoming(_event.so);
			}
		}
	}
}

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
	
	// Load config
	Json::Value _config_root;
	Json::Reader _config_reader;
	ifstream _config_stream(argv[1], std::ifstream::binary);
	if ( !_config_reader.parse(_config_stream, _config_root, false ) ) {
		cout << _config_reader.getFormattedErrorMessages() << endl;
		return 1;
	}

	for ( auto _it = _config_root.begin(); _it != _config_root.end(); ++_it ) {
		string _server_name = _it.key().asString();
		Json::Value _config_node = _config_root[_server_name];
		string _server_type_name = _config_node["server"].asString();
		if ( _server_type_name.size() == 0 ) {
			cout << "no server type in config " << _server_name << endl;
			return 2;
		}
		td_config::td_server _type = td_config::server_type_from_string(_server_type_name);
		if ( _type == td_config::td_server_unknow ) {
			cout << "unknow server type: " << _server_type_name << endl;
			return 3;
		}
		// Try to create services
		if ( _type == td_config::td_server_tcprelay ) {
			td_config_tcprelay _config(_server_name, _config_node);
			cout << _server_name << " is " << _server_type_name << 
				", listen on: " << _config.server_port() << endl;
		} else if ( _type == td_config::td_server_redirect ) {
			td_config_redirect _config(_server_name, _config_node);
			cout << _server_name << " is " << _server_type_name << 
				", listen on: " << _config.server_port() << endl;
		} else if ( _type == td_config::td_server_socks5 ) {
			td_config_socks5 _config(_server_name, _config_node);
			cout << _server_name << " is " << _server_type_name << 
				", listen on: " << _config.server_port() << endl;
		} else if ( _type == td_config::td_server_backdoor ) {
			td_config_backdoor _config(_server_name, _config_node);
			cout << _server_name << " is " << _server_type_name << 
				", listen on: " << _config.server_port() << endl;
		}
	}

	// Hang current process
	set_signal_handler();

	// Wait for kill signal
	wait_for_exit_signal();
	join_all_threads();

    return 0;
}

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
