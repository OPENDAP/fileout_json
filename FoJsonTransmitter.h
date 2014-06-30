// FoJsonTransmitter.h

// This file is part of BES Netcdf File Out Module

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef A_FoJsonTransmitter_h
#define A_FoJsonTransmitter_h 1

#include <DataDDS.h>
#include <ConstraintEvaluator.h>

#include <BESBasicTransmitter.h>

class BESContainer;

using namespace libdap;

/** @brief BESTransmitter class named "netcdf" that transmits an OPeNDAP
 * data object as a netcdf file
 *
 * The FoJsonTransmitter transforms an OPeNDAP DataDDS object into a
 * netcdf file and streams the new (temporary) netcdf file back to the
 * client.
 *
 * @see BESBasicTransmitter
 */
class FoJsonTransmitter: public BESBasicTransmitter {
private:
    static void return_temp_stream(const string &filename,
				   ostream &strm,
				   const string &ncVersion);
    static string temp_dir;


public:
    FoJsonTransmitter();
    virtual ~FoJsonTransmitter()
    {
    }

    static void send_data(BESResponseObject *obj, BESDataHandlerInterface &dhi);
    static void send_data(DataDDS *dds, ConstraintEvaluator &eval, BESDataHandlerInterface &dhi);

};

#endif // A_FoJsonTransmitter_h
