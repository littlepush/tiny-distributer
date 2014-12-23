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

#include "config_section.h"

// trim from start
std::string &ltrim(std::string &s) {
    s.erase(
        s.begin(), 
        std::find_if(
            s.begin(), 
            s.end(), 
            std::not1(std::ptr_fun<int, int>(std::isspace))
            )
        );
    return s;
}

// trim from end
std::string &rtrim(std::string &s) {
    s.erase(
        std::find_if(
            s.rbegin(), 
            s.rend(), 
            std::not1(std::ptr_fun<int, int>(std::isspace))
            ).base(), 
        s.end()
        );
    return s;
}

// trim from both ends
std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

// Split a string with the char in the carry.
void split_string( const std::string &value, 
    const std::string &carry, std::vector<std::string> &component )
{
    if ( value.size() == 0 ) return;
    std::string::size_type _pos = 0;
    do {
        std::string::size_type _lastPos = std::string::npos;
        for ( std::string::size_type i = 0; i < carry.size(); ++i ) {
            std::string::size_type _nextCarry = value.find( carry[i], _pos );
            _lastPos = (_nextCarry < _lastPos) ? _nextCarry : _lastPos;
        }
        if ( _lastPos == std::string::npos ) _lastPos = value.size();
        if ( _lastPos > _pos ) {
            std::string _com = value.substr( _pos, _lastPos - _pos );
            component.push_back(_com);
        }
        _pos = _lastPos + 1;
    } while( _pos < value.size() );
}

bool config_section::_internal_process_until_find_section_header(ifstream &in, string &head_string)
{
    while( in.eof() == false ) {
        getline(in, head_string);
        head_string = trim(head_string);
        if ( head_string.size() == 0 ) continue;
        if ( head_string[0] == '#' ) continue;    // comment
        if ( head_string[0] == '[' && head_string[head_string.size() - 1] == ']' ) {
            // Just find the section header
            return true;
        }
        // Just node
        vector<string> _com;
        split_string(head_string, "=", _com);
        if ( _com.size() == 1 ) {
            m_config_nodes[trim(head_string)] = "1";
        } else {
            m_config_nodes[trim(_com[0])] = trim(_com[1]);
        }
    }
    return true;
}

config_section::config_section()
:m_config_level(0), m_config_status(true), name(m_name)
{

}

config_section::config_section(ifstream &in)
:m_config_level(0), m_config_status(true), name(m_name)
{
    string _line;
    config_section *_section = this;
    do {
        if ( _section->_internal_process_until_find_section_header(in, _line) == false ) {
            m_config_status = false;
            break;
        }
        // did find section
        if ( in.eof() ) {
            // already to the end of file.
            break;
        }

        // Check new section level.
        config_section *_sub_section = NULL;

        vector<string> _com;
        split_string(_line, "[:]", _com);
        if ( _com[0] == "default" ) {
            _com.erase(_com.begin());
        }
        if ( _com.size() == 0 ) {
            _section = this;
            continue;
        }
        // Try to find the parent section
        config_section *_parent_section = this;
        for ( int i = 0; i < (int)_com.size(); ++i ) {
            string _section_name = trim(_com[i]);
            map<string, config_section *>::iterator _i_sec = _parent_section->m_sub_sections.find(_section_name);
            if ( _i_sec == _parent_section->m_sub_sections.end() ) {
                // Create new section
                _sub_section = new config_section();
                _sub_section->m_config_level = _com.size();
                _sub_section->m_name = _section_name;
                _parent_section->m_sub_sections[_section_name] = _sub_section;
            } else {
                // Use old section object
                _sub_section = _i_sec->second;
            }
            _parent_section = _sub_section;
        }
        _section = _sub_section;
    } while ( ( in.eof() == false ) );
    m_name = "default";
}

config_section::~config_section()
{
    for ( map<string, config_section *>::iterator i = m_sub_sections.begin();
        i != m_sub_sections.end(); ++i )
    {
        delete i->second;
    }
}

void config_section::begin_loop()
{
    m_iterator = m_config_nodes.begin();
}
map<string, string>::iterator config_section::end()
{
    return m_config_nodes.end();
}
map<string, string>::iterator config_section::current_node()
{
    return m_iterator;
}
void config_section::next_node()
{
    m_iterator++;
}

config_section::operator bool() const
{
    return m_config_status;
}

bool config_section::contains_key(const string &k)
{
    return m_config_nodes.find(k) != m_config_nodes.end();
}
string & config_section::operator[](const string &k)
{
    return m_config_nodes[k];
}
string & config_section::operator[](const char *k)
{
    return m_config_nodes[k];
}
void config_section::get_sub_section_names(vector<string> &names)
{
    for ( map<string, config_section*>::iterator _i = m_sub_sections.begin();
        _i != m_sub_sections.end(); ++_i ) {
        names.push_back(_i->first);
    }
}

config_section * config_section::sub_section(const string &name)
{
    map<string, config_section *>::iterator _i_sec = m_sub_sections.find(name);
    if ( _i_sec == m_sub_sections.end() ) return NULL;
    return _i_sec->second;
}

config_section* open_config_file(const char* fp)
{
    ifstream _config_stream;
    _config_stream.open(fp, ios_base::in);
    if ( _config_stream ) {
        config_section *_cf = new config_section(_config_stream);
        if ( *_cf ) return _cf;
        else {
            delete _cf;
            return NULL;
        }
    } else return NULL;
}
void close_config_file(config_section* cs)
{
    delete cs;
}

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */