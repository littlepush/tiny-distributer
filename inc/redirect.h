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

#pragma once

#ifndef __TINY_DISTRIBUTER_REDIRECT_H__
#define __TINY_DISTRIBUTER_REDIRECT_H__

#include "services.h"

class td_service_redirect : public td_service_tunnel
{
public:
	td_service_redirect(const string &name, const Json::Value &config_node);
	~td_service_redirect();

	virtual bool accept_new_incoming(SOCKET_T so);
};

#endif // tinydst.redirect.h

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
