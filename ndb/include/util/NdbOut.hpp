/* Copyright (C) 2003 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifndef NDBOUT_H
#define NDBOUT_H

#ifdef	__cplusplus

#include <ndb_types.h>
#include <util/BaseString.hpp>

/**
 * Class used for outputting logging messages to screen.
 * Since the output capabilities are different on different platforms
 * this middle layer class should be used for all output messages
 */

/* 
   Example usage:
   
   #include "NdbOut.hpp"
   
   / *  Use ndbout as you would use cout:
   
        ndbout << "Hello World! "<< 1 << " Hello again" 
               << 67 << anIntegerVar << "Hup << endl;
   
   
   / * Use ndbout_c as you would use printf:
   
       ndbout_c("Hello World %d\n", 1);
*/

class NdbOut;
class OutputStream;
class NullOutputStream;

/*  Declare a static variable of NdbOut as ndbout */
extern NdbOut ndbout;

class NdbOut
{
public:
  NdbOut& operator<<(NdbOut& (* _f)(NdbOut&));
  NdbOut& operator<<(Int8);
  NdbOut& operator<<(Uint8);
  NdbOut& operator<<(Int16);
  NdbOut& operator<<(Uint16);
  NdbOut& operator<<(Int32);
  NdbOut& operator<<(Uint32);
  NdbOut& operator<<(Int64);
  NdbOut& operator<<(Uint64);
  NdbOut& operator<<(long unsigned int);
  NdbOut& operator<<(const char*);
  NdbOut& operator<<(const unsigned char*);
  NdbOut& operator<<(BaseString &);
  NdbOut& operator<<(const void*);
  NdbOut& operator<<(float);
  NdbOut& operator<<(double);
  NdbOut& endline(void);
  NdbOut& flushline(void);
  NdbOut& setHexFormat(int _format);
  
  NdbOut(OutputStream &);
  virtual ~NdbOut();

  void print(const char * fmt, ...);
  void println(const char * fmt, ...);
  
  OutputStream * m_out;
private:
  int isHex;
};

inline NdbOut& NdbOut::operator<<(NdbOut& (* _f)(NdbOut&)) {
  (* _f)(*this); 
  return * this; 
}

inline NdbOut&  endl(NdbOut& _NdbOut) { 
  return _NdbOut.endline(); 
}

inline NdbOut&  flush(NdbOut& _NdbOut) { 
  return _NdbOut.flushline(); 
}

inline  NdbOut& hex(NdbOut& _NdbOut) {
  return _NdbOut.setHexFormat(1);
}

inline NdbOut& dec(NdbOut& _NdbOut) {
  return _NdbOut.setHexFormat(0);
}
extern "C"
void ndbout_c(const char * fmt, ...);

class FilteredNdbOut : public NdbOut {
public:
  FilteredNdbOut(OutputStream &, int threshold = 0, int level = 0);
  virtual ~FilteredNdbOut();

  void setLevel(int i);
  void setThreshold(int i);

  int getLevel() const;
  int getThreshold() const;
  
private:
  int m_threshold, m_level;
  OutputStream * m_org;
  NullOutputStream * m_null;
};

#else
void ndbout_c(const char * fmt, ...);
#endif

#endif
