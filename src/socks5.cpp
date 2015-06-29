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

#include "socks5.h"

td_service_socks5::td_service_socks5(const string &name, const Json::Value &config_node) {
	config_ = new td_config_socks5(name, config_node);
	this->_initialize_thread_pool();
	sl_socks5_set_supported_method(sl_method_noauth);
	sl_socks5_set_supported_method(sl_method_userpwd);
}
td_service_socks5::~td_service_socks5() {
	delete config_;
}

bool td_service_socks5::accept_new_incoming(SOCKET_T so) {
	sl_tcpsocket _wrap_src(so);
	if ( !td_service::accept_new_incoming(so) ) {
		_wrap_src.close();
		return false;
	}

	sl_methods _m = sl_socks5_handshake_handler(so);
	if ( _m == sl_method_nomethod ) {
		close(so); return false;
	}

	if ( _m == sl_method_userpwd ) {
		auto _fauth = [this](const string &u, const string &p){
			return ((td_config_socks5 *)config_)->is_validate_user(u, p);
		};
		bool _auth = sl_socks5_auth_by_username(so, _fauth);
		if ( _auth == false ) {
			_wrap_src.close();
			return false;
		}
	}

	string _addr;
	uint16_t _port;
	if ( !sl_socks5_get_connect_info(so, _addr, _port) ) {
		close(so); return false;
	}

	sl_tcpsocket _wrap_dst(true);
	td_config_socks5 *_cfg = (td_config_socks5 *)config_;
	for ( auto &_socks5 : _cfg->proxy_list() ) {
		if ( _wrap_dst.setup_proxy(_socks5.first, _socks5.second) ) break;
	}
	if ( !_wrap_dst.connect(_addr, _port) ) {
		sl_socks5_failed_connect_to_peer(so, sl_socks5rep_unreachable);
		close(so);
		_wrap_dst.close();
		return false;
	}
	sl_socks5_did_connect_to_peer(so, network_domain_to_inaddr(_addr.c_str()), _port);
	this->_did_accept_sockets(so, _wrap_dst.m_socket);
	return true;
}

// tinydst.socks5.cpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
