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

#ifndef REL_TABMEM_HPP
#define REL_TABMEM_HPP

#include "SignalData.hpp"

class RelTabMemReq {
  /**
   * Sender(s) and Receiver(s)
   */
  friend class Dbdict;

  /**
   * Receiver(s)
   */
  friend class Dbacc;
  friend class Dbdih;
  friend class Dblqh;
  friend class Dbtc;
  friend class Dbtup;
public:
  STATIC_CONST( SignalLength = 4 );

private:
  Uint32 userPtr;
  Uint32 userRef;
  Uint32 primaryTableId;
  Uint32 secondaryTableId;
};

class RelTabMemConf {
  /**
   * Sender(s) and Receiver(s)
   */
  friend class Dbdict;

  /**
   * Sender(s)
   */
  friend class Dbacc;
  friend class Dbdih;
  friend class Dblqh;
  friend class Dbtc;
  friend class Dbtup;
public:
  STATIC_CONST( SignalLength = 2 );

private:
  Uint32 userPtr;
  Uint32 senderRef;
  Uint32 nodeId;
};

#endif
