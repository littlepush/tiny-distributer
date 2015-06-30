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
		_event_list.clear();
		sl_poller::server().fetch_events(_event_list);
		for ( auto &_event : _event_list ) {
			if ( _event.event == SL_EVENT_FAILED ) {
				shared_ptr<td_service> _psvr = service_by_maintaining_socket(_event.so);
				if ( _psvr != nullptr ) _psvr->close_socket(_event.so);
			} else if ( _event.event == SL_EVENT_ACCEPT ) {
				shared_ptr<td_service> _psvr = service_by_socket(_event.source);
				if ( _psvr != nullptr ) _psvr->accept_new_incoming(_event.so);
			} else {
				shared_ptr<td_service> _psvr = service_by_maintaining_socket(_event.so);
				if ( _psvr != nullptr ) _psvr->socket_has_data_incoming(_event.so);
			}
		}
	}
}

int main( int argc, char * argv[] ) {

	// Parse the parameters
	// Todo...

	string _config_path = "/etc/tinydst.json";
    if ( argc == 2 ) {
		_config_path = argv[1];
    }
	
	// Load config
	Json::Value _config_root;
	Json::Reader _config_reader;
	ifstream _config_stream(_config_path, std::ifstream::binary);
	if ( !_config_reader.parse(_config_stream, _config_root, false ) ) {
		cout << _config_reader.getFormattedErrorMessages() << endl;
		return 1;
	}

	bool _daemon = true;
	if ( _config_root.isMember("daemon") ) {
		_daemon = _config_root["daemon"].asBool();
	}

	if ( _config_root.isMember("services") == false ) {
		cerr << "No services configuration" << endl;
		return 1;
	}

    if ( _daemon ) {
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

	Json::Value &_service_node = _config_root["services"];

	for ( auto _it = _config_root.begin(); _it != _service_node.end(); ++_it ) {
		string _server_name = _it.key().asString();
		Json::Value _config_node = _service_node[_server_name];
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
		td_service *_service = NULL;
		if ( _type == td_config::td_server_tcprelay ) {
			_service = new td_service_tcprelay(_server_name, _config_node);
		} else if ( _type == td_config::td_server_redirect ) {
			_service = new td_service_redirect(_server_name, _config_node);
		} else if ( _type == td_config::td_server_socks5 ) {
			_service = new td_service_socks5(_server_name, _config_node);
		} else if ( _type == td_config::td_server_backdoor ) {
			_service = new td_service_backdoor(_server_name, _config_node);
		}

		if ( _service != NULL ) {
			register_new_service(shared_ptr<td_service>(_service));
		}
	}

	// Bind all backdoor service.
	td_service_backdoor::bind_backdoor_services();

	// Hang current process
	set_signal_handler();

	// create the main loop thread
	thread _main_loop(tiny_distributer_worker);

	// Wait for kill signal
	wait_for_exit_signal();
	join_all_threads();
	_main_loop.join();

    return 0;
}

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
