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
#include <map>

int __g_port = 1025;
config_section *__g_config = NULL;

// Temp struct in v0.2
tl_mutex __g_td_mutex;
typedef map<sl_tcpsocket *, sl_tcpsocket *> _t_td_cache;
_t_td_cache __g_td_cache;
_t_td_cache __g_td_reverse_cache;

list<sl_tcpsocket *> __g_td_incoming_list;
tl_mutex __g_td_incoming_mutex;
tl_semaphore __g_td_incoming_sem;

tl_semaphore __g_td_main_sem;
tl_semaphore __g_td_reverse_sem;

void __internal_relay_worker( tl_thread **thread, _t_td_cache & main_cache, _t_td_cache & reverse_cache, tl_semaphore & main_sem)
{
    tl_thread *_pthread = *thread;
    // Get all incoming package, 
    // how to process these packages?
    fd_set _fds;
    while ( _pthread->thread_status() )
    {
        if ( !(main_cache.size() == 0 && main_sem.get(100)) ) {
            continue;
        }
        FD_ZERO(&_fds);
        int _max_so = 0;
        do {
            _t_td_cache _disconnected;
            tl_lock _l(__g_td_mutex);
            for ( _t_td_cache::iterator i = main_cache.begin();
                  i != main_cache.end(); 
                  ++i )
            {
                if ( socket_check_status(i->first->m_socket, SO_CHECK_CONNECT) == SO_INVALIDATE ) {
                    _disconnected[i->first] = i->second;
                    continue;
                }
                FD_SET(i->first->m_socket, &_fds);
                if ( i->first->m_socket > _max_so ) _max_so = i->first->m_socket;
            }
            for ( _t_td_cache::iterator i = _disconnected.begin();
                  i != _disconnected.end();
                  ++i )
            {
                sl_tcpsocket *_client = i->first;
                sl_tcpsocket *_recirect = i->second;
                main_cache.erase(i->first);
                reverse_cache.erase(i->second);
                delete _client;
                delete _recirect;
            }
        } while ( false );
        if (_max_so == 0 ) continue;

        struct timeval _tv = {100 / 1000, 100 % 1000 * 1000};
        int _ret = -1;
        do {
            _ret = select(_max_so + 1, &_fds, NULL, NULL, &_tv);
        } while ( _ret < 0 && errno == EINTR );
        
        if ( _ret < 0 ) {
            cerr << "failed to get incoming data from client." << endl;
            continue;
        }
        if ( _ret == 0 ) {
            cout << "timeout..." << endl;
            continue;
        }
        do {
            string _buffer;
            tl_lock _l(__g_td_mutex);
            for ( _t_td_cache::iterator i = main_cache.begin();
                  i != main_cache.end();
                  ++i )
            {
                if ( !FD_ISSET(i->first->m_socket, &_fds) ) continue;
                // Do something...
                i->first->read_data(_buffer);
                i->second->write_data(_buffer);
            }
        } while( false );
    }
}

void _td_distributer_receiver( tl_thread **thread )
{
    __internal_relay_worker(thread, __g_td_cache, __g_td_reverse_cache, __g_td_main_sem);
}

void _td_distributer_redirecter( tl_thread **thread )
{
    // Redirect data to original client?
    __internal_relay_worker(thread, __g_td_reverse_cache, __g_td_cache, __g_td_reverse_sem);
}

void _td_distributer_incoming(tl_thread ** thread)
{
    tl_thread *_pthread = *thread;
    while( _pthread->thread_status() )
    {
        if ( __g_td_incoming_sem.get(100) == false ) continue;
        sl_tcpsocket *_client = NULL;
        do {
            tl_lock _l(__g_td_incoming_mutex);
            _client = __g_td_incoming_list.front();
            __g_td_incoming_list.pop_front();
        } while ( false );
    
        vector<string> __redirect_setting;
        list<sl_tcpsocket *> _nodes;
        __g_config->get_sub_section_names(__redirect_setting);

        bool _all_correct = false;
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
                delete _so;
                continue;
            }

            tl_lock _l(__g_td_mutex);
            __g_td_cache[_client] = _so;
            __g_td_reverse_cache[_so] = _client;
            _all_correct = true;
            __g_td_main_sem.initialize(1);
            __g_td_reverse_sem.initialize(1);
            break;
        }
        if ( !_all_correct ) {
            delete _client;
        }
    }
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

    // Initialize the sem
    __g_td_incoming_sem.initialize(0);
    __g_td_main_sem.initialize(0);
    __g_td_reverse_sem.initialize(0);

    tl_thread *_receiver = new tl_thread(_td_distributer_receiver);
    _receiver->start_thread();
    tl_thread *_redirector = new tl_thread(_td_distributer_redirecter);
    _redirector->start_thread();
    tl_thread *_incoming = new tl_thread(_td_distributer_incoming);
    _incoming->start_thread();

    while( _pthread->thread_status() )
    {
        sl_tcpsocket *_client = (sl_tcpsocket *)_lso.get_client();
        if ( _client == NULL ) continue;

        tl_lock _l(__g_td_incoming_mutex);
        __g_td_incoming_list.push_back(_client);
        __g_td_incoming_sem.give();
    }

    _incoming->stop_thread();
    _receiver->stop_thread();
    _redirector->stop_thread();
    // Clean all data
    for ( list<sl_tcpsocket *>::iterator incoming_iterator = __g_td_incoming_list.begin();
          incoming_iterator != __g_td_incoming_list.end();
          ++incoming_iterator)
    {
        _lso.release_client(*incoming_iterator);
    }
    for ( _t_td_cache::iterator i = __g_td_cache.begin();
          i != __g_td_cache.end();
          ++i )
    {
        delete i->first;
        delete i->second;
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