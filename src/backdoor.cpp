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

#include "backdoor.h"

static map<td_service_backdoor *, bool>& g_backdoor_map() {
	static map<td_service_backdoor *, bool> _m;
	return _m;
}

void td_service_backdoor::bind_backdoor_services() {
	for( auto &_it : g_backdoor_map() ) {
		_it.first->register_backdoor_();
	}
}

void td_service_backdoor::register_backdoor_() {
	td_config_backdoor *_cfg = static_cast<td_config_backdoor *>(config_);
	shared_ptr<td_service> _pservice = service_by_name(_cfg->related_server_name());
	if ( _pservice == NULL ) return;
	if ( _cfg->is_redirect_request() ) {
		_pservice->register_request_redirect(
				[this](const string &data){ this->request_redirector_(data); }
				);
	}
	if ( _cfg->is_redirect_response() ) {
		_pservice->register_response_redirect(
				[this](const string &data){ this->response_redirector_(data); }
				);
	}
}

void td_service_backdoor::request_redirector_(const string &data) {
	for ( auto &_soit : request_so_ ) {
		sl_tcpsocket _wso(_soit.first);
		_wso.write_data(data);
	}
}

void td_service_backdoor::response_redirector_(const string &data ) {
	for ( auto &_soit : request_so_ ) {
		sl_tcpsocket _wso(_soit.first);
		_wso.write_data(data);
	}
}

td_service_backdoor::td_service_backdoor(const string &name, const Json::Value& config_node) {
	config_ = new td_config_backdoor(name, config_node);
	g_backdoor_map()[this] = true;
}

td_service_backdoor::~td_service_backdoor() {
	g_backdoor_map().erase(this);
	delete config_;
}

bool td_service_backdoor::accept_new_incoming(SOCKET_T so) {
	// Check IP range
	sl_tcpsocket _wso(so);
	if ( !td_service::accept_new_incoming(so) ) {
		_wso.close();
		return false;
	}
	
	// We accept all request.
	request_so_[so] = true;
	return true;
}

void td_service_backdoor::close_socket(SOCKET_T so) {
	request_so_.erase(so);
	close(so);
}

void td_service_backdoor::socket_has_data_incoming(SOCKET_T so) {
	sl_tcpsocket _wso(so);
	string _buffer;
	_wso.read_data(_buffer);

	SOCKETSTATUE _st = socket_check_status(so, SO_CHECK_READ);
	if ( _st == SO_INVALIDATE ) {
		this->close_socket(so);
	}
}

// tinydst.backdoor.cpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
