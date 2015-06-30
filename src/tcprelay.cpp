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

#include "tcprelay.h"

td_service_tcprelay::td_service_tcprelay(const string &name, const Json::Value &config_node)
{
	config_ = new td_config_tcprelay(name, config_node);
#ifdef USE_THREAD_SERVICE
	this->_initialize_thread_pool();
#endif
}
td_service_tcprelay::~td_service_tcprelay() {
	delete config_;
}
bool td_service_tcprelay::accept_new_incoming(SOCKET_T so) {
	sl_tcpsocket _wrap_src(so);
	if ( !td_service::accept_new_incoming(so) ) {
		_wrap_src.close();
		return false;
	}

	// Get original destination
	string _org_addr;
	uint32_t _org_port;
	if ( !_wrap_src.get_original_dest(_org_addr, _org_port) ) {
		// The socket is not validate
		_wrap_src.close();
		return false;
	}

	sl_tcpsocket _wrap_dst(true);
	for ( auto &_socks5 : static_cast<td_config_tcprelay *>(config_)->proxy_list() ) {
		if ( _wrap_dst.setup_proxy(_socks5.first, _socks5.second) ) break;
	}
	if ( !_wrap_dst.connect(_org_addr, _org_port) ) {
		_wrap_dst.close();
		_wrap_src.close();
		return true;
	}

	// Add to monitor
	this->_did_accept_sockets(_wrap_src.m_socket, _wrap_dst.m_socket);

	return true;
}

// tinydst.tcprelay.cpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
