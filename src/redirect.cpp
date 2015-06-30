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

#include "redirect.h"

td_service_redirect::td_service_redirect(const string &name, const Json::Value &config_node) {
	config_ = new td_config_redirect(name, config_node);
#ifdef USE_THREAD_SERVICE
	this->_initialize_thread_pool();
#endif
}
td_service_redirect::~td_service_redirect() {
	delete config_;
}
bool td_service_redirect::accept_new_incoming(SOCKET_T so) {
	sl_tcpsocket _wrap_src(so);
	if ( !td_service::accept_new_incoming(so) ) {
		_wrap_src.close();
		return false;
	}

	// Try to connect to socks5 proxy
	sl_tcpsocket _wrap_dst(true);
	td_config_redirect *_cfg = static_cast<td_config_redirect *>(config_);
	for ( auto &_dst : _cfg->destination_list() ) {
		for ( auto &_socks5 : _dst.socks5 ) {
			if ( _wrap_dst.setup_proxy(_socks5.first, _socks5.second) ) break;
		}
		if ( _wrap_dst.connect(_dst.ipaddr, _dst.port) ) break;
		_wrap_dst.close();
	}

	if ( SOCKET_NOT_VALIDATE(_wrap_dst.m_socket) ) {
		_wrap_src.close();
		return false;
	}

	this->_did_accept_sockets(_wrap_src.m_socket, _wrap_dst.m_socket);
	return true;
}

// tinydst.redirect.cpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
