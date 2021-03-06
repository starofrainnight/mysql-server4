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

#define DBTUX_CMP_CPP
#include "Dbtux.hpp"

/*
 * Search key vs node prefix or entry
 *
 * The comparison starts at given attribute position.  The position is
 * updated by number of equal initial attributes found.  The entry data
 * may be partial in which case CmpUnknown may be returned.
 */
int
Dbtux::cmpSearchKey(const Frag& frag, unsigned& start, ConstData searchKey, ConstData entryData, unsigned maxlen)
{
  const unsigned numAttrs = frag.m_numAttrs;
  const DescEnt& descEnt = getDescEnt(frag.m_descPage, frag.m_descOff);
  // number of words of attribute data left
  unsigned len2 = maxlen;
  // skip to right position in search key only
  for (unsigned i = 0; i < start; i++) {
    jam();
    searchKey += AttributeHeaderSize + searchKey.ah().getDataSize();
  }
  int ret = 0;
  while (start < numAttrs) {
    if (len2 <= AttributeHeaderSize) {
      jam();
      ret = NdbSqlUtil::CmpUnknown;
      break;
    }
    len2 -= AttributeHeaderSize;
    if (! searchKey.ah().isNULL()) {
      if (! entryData.ah().isNULL()) {
        jam();
        // current attribute
        const DescAttr& descAttr = descEnt.m_descAttr[start];
        // full data size
        const unsigned size1 = AttributeDescriptor::getSizeInWords(descAttr.m_attrDesc);
        ndbrequire(size1 != 0 && size1 == entryData.ah().getDataSize());
        const unsigned size2 = min(size1, len2);
        len2 -= size2;
        // compare
        NdbSqlUtil::Cmp* const cmp = c_sqlCmp[start];
        const Uint32* const p1 = &searchKey[AttributeHeaderSize];
        const Uint32* const p2 = &entryData[AttributeHeaderSize];
        ret = (*cmp)(0, p1, p2, size1, size2);
        if (ret != 0) {
          jam();
          break;
        }
      } else {
        jam();
        // not NULL > NULL
        ret = +1;
        break;
      }
    } else {
      if (! entryData.ah().isNULL()) {
        jam();
        // NULL < not NULL
        ret = -1;
        break;
      }
    }
    searchKey += AttributeHeaderSize + searchKey.ah().getDataSize();
    entryData += AttributeHeaderSize + entryData.ah().getDataSize();
    start++;
  }
  return ret;
}

/*
 * Scan bound vs node prefix or entry.
 *
 * Compare lower or upper bound and index entry data.  The entry data
 * may be partial in which case CmpUnknown may be returned.  Otherwise
 * returns -1 if the bound is to the left of the entry and +1 if the
 * bound is to the right of the entry.
 *
 * The routine is similar to cmpSearchKey, but 0 is never returned.
 * Suppose all attributes compare equal.  Recall that all bounds except
 * possibly the last one are non-strict.  Use the given bound direction
 * (0-lower 1-upper) and strictness of last bound to return -1 or +1.
 *
 * Following example illustrates this.  We are at (a=2, b=3).
 *
 * dir  bounds                  strict          return
 * 0    a >= 2 and b >= 3       no              -1
 * 0    a >= 2 and b >  3       yes             +1
 * 1    a <= 2 and b <= 3       no              +1
 * 1    a <= 2 and b <  3       yes             -1
 */
int
Dbtux::cmpScanBound(const Frag& frag, unsigned dir, ConstData boundInfo, unsigned boundCount, ConstData entryData, unsigned maxlen)
{
  const DescEnt& descEnt = getDescEnt(frag.m_descPage, frag.m_descOff);
  // direction 0-lower 1-upper
  ndbrequire(dir <= 1);
  // number of words of data left
  unsigned len2 = maxlen;
  // in case of no bounds, init last type to something non-strict
  unsigned type = 4;
  while (boundCount != 0) {
    if (len2 <= AttributeHeaderSize) {
      jam();
      return NdbSqlUtil::CmpUnknown;
    }
    len2 -= AttributeHeaderSize;
    // get and skip bound type (it is used after the loop)
    type = boundInfo[0];
    boundInfo += 1;
    if (! boundInfo.ah().isNULL()) {
      if (! entryData.ah().isNULL()) {
        jam();
        // current attribute
        const unsigned index = boundInfo.ah().getAttributeId();
        ndbrequire(index < frag.m_numAttrs);
        const DescAttr& descAttr = descEnt.m_descAttr[index];
        ndbrequire(entryData.ah().getAttributeId() == descAttr.m_primaryAttrId);
        // full data size
        const unsigned size1 = boundInfo.ah().getDataSize();
        ndbrequire(size1 != 0 && size1 == entryData.ah().getDataSize());
        const unsigned size2 = min(size1, len2);
        len2 -= size2;
        // compare
        NdbSqlUtil::Cmp* const cmp = c_sqlCmp[index];
        const Uint32* const p1 = &boundInfo[AttributeHeaderSize];
        const Uint32* const p2 = &entryData[AttributeHeaderSize];
        int ret = (*cmp)(0, p1, p2, size1, size2);
        if (ret != 0) {
          jam();
          return ret;
        }
      } else {
        jam();
        // not NULL > NULL
        return +1;
      }
    } else {
      jam();
      if (! entryData.ah().isNULL()) {
        jam();
        // NULL < not NULL
        return -1;
      }
    }
    boundInfo += AttributeHeaderSize + boundInfo.ah().getDataSize();
    entryData += AttributeHeaderSize + entryData.ah().getDataSize();
    boundCount -= 1;
  }
  // all attributes were equal
  const int strict = (type & 0x1);
  return (dir == 0 ? (strict == 0 ? -1 : +1) : (strict == 0 ? +1 : -1));
}
