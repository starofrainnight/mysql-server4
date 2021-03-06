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

#ifndef ERRORHANDLINGMACROS_H
#define ERRORHANDLINGMACROS_H

#include "ErrorReporter.hpp"
#include "Error.hpp"

extern const char programName[];

#define ERROR_SET_SIGNAL(messageCategory, messageID, problemData, objectRef) \
        ErrorReporter::handleError(messageCategory, messageID, problemData, objectRef, NST_ErrorHandlerSignal)
#define ERROR_SET(messageCategory, messageID, problemData, objectRef) \
        ErrorReporter::handleError(messageCategory, messageID, problemData, objectRef)
        // Description:
        //      Call ErrorHandler with the supplied arguments. The
        //      ErrorHandler decides how to report the error.
        // Parameters:
        //      messageCategory IN      A hint to the error handler how the 
        //                              error should be reported. Can be
        //                              error, fatal (or warning, use WARNING_SET instead).
        //      messageID       IN      Code identifying the error. If less
        //                              than 1000 a unix error is assumed. If
        //                              greater than 1000 the code is treated 
        //                              as the specific problem code.
        //      problemData     IN      A (short) text describing the error.
        //                              The context information is added to
        //                              this text.
        //      objectRef       IN      The name of the "victim" of the error.
        //                              Specify NULL if not applicable.
        // Return value:
        //      -
        // Reported errors:
        //      -
        // Additional information:
        //      -

#endif
