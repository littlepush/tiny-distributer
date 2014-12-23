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

#include <sl/tcpsocket.h>
#include <tl/thread.h>
#include "config_section.h"
#include <list>
#include <vector>

typedef struct {
    sl_tcpsocket        listener;
    sl_tcpsocket        *client;
} relay_pair, *lreplay_pair;

tl_mutex __g_status_mutex;
bool __g_status = true;

int __g_port = 1025;
config_section *__g_config = NULL;

void _td_distributer_worker( tl_thread **thread )
{
    tl_thread *_pthread = *thread;
    sl_tcpsocket *_client = (sl_tcpsocket *)_pthread->user_info;

    bool _status = true;

    vector<string> __redirect_setting;
    list<sl_tcpsocket *> _nodes;
    __g_config->get_sub_section_names(__redirect_setting);
    for ( int i = 0; i < __redirect_setting.size(); ++i ) {
        config_section *_setting = __g_config->sub_section(__redirect_setting[i]);

        if ( _setting->contains_key("ip") == false ) {
            cerr << "no redirect ip." << endl;
            continue;
        }
        string _address = (*_setting)["ip"];
        unsigned int _port = __g_port;
        if ( _setting->contains_key("port") ) {
            _port = atoi(((*_setting)["port"]).c_str());
        }

        sl_tcpsocket *_so = new sl_tcpsocket();
        if ( _setting->contains_key("socks5") ) {
            string _socks5 = (*_setting)["socks5"];
            vector<string> _com;
            split_string(_socks5, ":", _com);
            if ( _com.size() != 2 ) {
                cerr << "socks5 format error. will not connect to socks5 proxy." << endl;
            } else {
                _so->setup_proxy(_com[0], atoi(_com[1].c_str()));
            }
        }
        if ( !_so->connect(_address, _port) ) {
            cerr << "failed to connect to " << _address << ":" << _port << endl;
            continue;
        }

        if ( _setting->contains_key("master") ) {
            _nodes.insert(_nodes.begin(), _so);
        } else {
            _nodes.push_back(_so);
        }
    }

    if ( _nodes.size() == 0 ) {
        cerr << "No redirect config." << endl;
    }

    while ( _pthread->thread_status() && _nodes.size() != 0 )
    {
        do {
            tl_lock _l(__g_status_mutex);
            _status = __g_status;
        } while ( false );

        // Close all sub thread
        if ( _status == false ) {
            delete _pthread;
            _pthread = NULL;
            break;
        }

        SOCKETSTATUE _ss = socket_check_status(_client->m_socket);
        if ( _ss == SO_INVALIDATE ) break;

        // Do things...
        SOCKET_T _max_so = _client->m_socket;
        fd_set _fds;
        FD_ZERO(&_fds);
        FD_SET(_client->m_socket, &_fds);
        // 100ms
        struct timeval _tv = {100 / 1000, 100 % 1000 * 1000};
        for ( list<sl_tcpsocket *>::iterator li = _nodes.begin(); li != _nodes.end(); ++li )
        {
            FD_SET((*li)->m_socket, &_fds);
            if ( (*li)->m_socket > _max_so ) _max_so = (*li)->m_socket;
        }
        int _ret = 0;
        do {
            _ret = select(_max_so + 1, &_fds, NULL, NULL, &_tv);
        } while ( _ret < 0 && errno == EINTR );
        // No data
        if ( _ret == 0 ) continue;

        // Check client
        if ( FD_ISSET(_client->m_socket, &_fds) ) {
            string _buffer;
            if ( !_client->read_data(_buffer) ) break;
            for ( list<sl_tcpsocket *>::iterator li = _nodes.begin(); li != _nodes.end(); ++li )
            {
                (*li)->write_data(_buffer);
            }
        }
        for ( list<sl_tcpsocket *>::iterator li = _nodes.begin(); li != _nodes.end(); ++li )
        {
            if ( !FD_ISSET((*li)->m_socket, &_fds) ) continue;
            string _buffer;
            // cout << "node has data incoming" << endl;
            if ( !(*li)->read_data(_buffer) ) continue;
            // cout << "get response: " << _buffer << endl;
            if ( li == _nodes.begin() ) {
                // Write back
                _client->write_data(_buffer);
            }
        }
    }

    delete _client;
    for ( list<sl_tcpsocket *>::iterator li = _nodes.begin(); li != _nodes.end(); ++li )
    {
        delete (*li);
    }
    delete (*thread);
    *thread = NULL;
}

void _td_listener_worker(tl_thread **thread)
{
    tl_thread *_pthread = *thread;
    sl_tcpsocket _lso;

    bool _is_listened = false;
    do {
        if ( _lso.listen(__g_port) ) {
            _is_listened = true;
            break;
        }
        cerr << "failed to listen on " << __g_port << ", " << strerror(errno) << endl;
        usleep(1000000);
    } while( _pthread->thread_status() );

    if ( !_is_listened ) return;

    __g_status = true;
    while( _pthread->thread_status() )
    {
        sl_tcpsocket *_client = (sl_tcpsocket *)_lso.get_client();
        if ( _client == NULL ) continue;
        tl_thread *_distributer = new tl_thread(_td_distributer_worker);
        _distributer->user_info = _client;
        _distributer->start_thread();
    }

    tl_lock _lock(__g_status_mutex);
    __g_status = false;
}

int main( int argc, char * argv[] ) {

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

    if ( argc != 2 ) {
        cerr << "must specified the config file path." << endl;
        return 1;
    }
    config_section *_cf_root = open_config_file(argv[1]);
    if ( _cf_root == NULL ) {
        cerr << "cannot read the config file." << endl;
        return 2;
    }

    // Check config
    if ( _cf_root->contains_key("port") == false ) {
        cerr << "missing \'port\'" << endl; 
        close_config_file(_cf_root);
        return 3;
    }
    __g_port = atoi((*_cf_root)["port"].c_str());

    config_section *_redirect = _cf_root->sub_section("redirect");
    if ( _redirect == NULL ) {
        cerr << "missing section \'redirect\'" << endl;
        close_config_file(_cf_root);
        return 4;
    }
    __g_config = _redirect;

    // All clear, go on.
    set_signal_handler();

    tl_thread *_listener = new tl_thread( _td_listener_worker );
    _listener->start_thread();

    wait_for_exit_signal();
    _listener->stop_thread();
    close_config_file(_cf_root);

    return 0;
}