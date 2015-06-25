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

class td_service_tcprelay : public td_service
{
protected:
	td_config_tcprelay		config_;
	map<SOCKET_T, SOCKET_T>	so_map_;
public:
	virtual const string& server_name() const;
	td_service_tcprelay(const string &name, const Json::Value &config_node);

	virtual bool start_service();
	virtual void accept_new_incoming(SOCKET_T so);
	virtual void close_socket(SOCKET_T so);
	virtual void socket_has_data_incoming(SOCKET_T so);
};

// tinydst.tcprelay.h

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
