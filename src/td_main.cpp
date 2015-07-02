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
#include "log.h"
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

void help() {
	cout << "tinydst [config_file]" << endl;
	cout << "	default config file is /etc/tinydst.json" << endl;
	cout << "tinydst --help" << endl;
	cout << "	print this message" << endl;
	cout << "tinydst --version" << endl;
	cout << "	print version information" << endl;
}

void version() {
	cout << "tinydst -- Version: " << VERSION << endl;
	cout << "Build Target: " << TARGET << endl;
	cout << "Build info: " << endl;
#ifdef USE_THREAD_SERVICE
	cout << "	+thread-service" << endl;
#else
	cout << "	-thread-service" << endl;
#endif
#ifdef RELEASE
	cout << "	+release" << endl;
#endif
#ifdef DEBUG
	cout << "	+debug" << endl;
#endif
#ifdef WITHPG
	cout << "	+withpg" << endl;
#endif

}
int main( int argc, char * argv[] ) {
	string _config_path = "/etc/tinydst.json";
	// Parse the parameters
	bool _processing_command = false;
	string _last_command = "";
	for ( int _argindex = 1; _argindex < argc; ++_argindex ) {
		if ( argv[_argindex][0] == '-' ) {	// this is a command
			_processing_command = true;
			int _cmdlength = strlen(argv[_argindex]);
			int _sp = 1;
			for ( ; _sp < _cmdlength; ++_sp ) {
				if ( argv[_argindex][_sp] == '-' ) continue;
				break;
			}
			_last_command = string(argv[_argindex] + _sp, _cmdlength - _sp);
			if ( _last_command == "help" ) {
				help();
				return 0;
			}
			if ( _last_command == "version" ) {
				version();
				return 0;
			}
			cout << "Invalidate command: " << _last_command << endl;
			help();
			return 1;
		} else {
			_config_path = argv[_argindex];
		}
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

	string _logpath = "syslog";
	if ( _config_root.isMember("logpath") ) {
		_logpath = _config_root["logpath"].asString();
	}
	string _loglv = "info";
	if ( _config_root.isMember("loglevel") ) {
		_loglv = _config_root["loglevel"].asString();
	}

	td_log_level _lv = log_info;
	if ( _loglv == "emergancy" ) {
		_lv = log_emergancy;
	} else if ( _loglv == "alert" ) {
		_lv = log_alert;
	} else if ( _loglv == "critical" ) {
		_lv = log_critical;
	} else if ( _loglv == "error" ) {
		_lv = log_error;
	} else if ( _loglv == "warning" ) {
		_lv = log_warning;
	} else if ( _loglv == "notice" ) {
		_lv = log_notice;
	} else if ( _loglv == "info" ) {
		_lv = log_info;
	} else if ( _loglv == "debug" ) {
		_lv = log_debug;
	}

	if ( _logpath == "syslog" ) {
		td_log_start(_lv);
	} else if ( _logpath == "stdout" ) {
		td_log_start(stdout, _lv);
	} else if ( _logpath == "stderr" ) {
		td_log_start(stderr, _lv);
	} else {
		td_log_start(_logpath, _lv);
	}

	Json::Value &_service_node = _config_root["services"];

	for ( auto _it = _service_node.begin(); _it != _service_node.end(); ++_it ) {
		string _server_name = _it.key().asString();
		Json::Value _config_node = _service_node[_server_name];
		string _server_type_name = _config_node["server"].asString();
		if ( _server_type_name.size() == 0 ) {
			td_log(log_critical, "no server type \'%s\' in config", _server_name.c_str());
			return 2;
		}
		td_config::td_server _type = td_config::server_type_from_string(_server_type_name);
		if ( _type == td_config::td_server_unknow ) {
			td_log(log_critical, "unknow server type: \'%s\'", _server_type_name.c_str());
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
	td_log(log_info, "tinydst receive terminate signal");

	join_all_threads();

	_main_loop.join();
	stop_all_services();
	td_log_stop();

	td_log(log_info, "tinydst terminated");

    return 0;
}

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
