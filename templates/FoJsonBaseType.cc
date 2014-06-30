// FoJsonBaseType.cc

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

#include <BESDebug.h>

#include "FoJsonBaseType.h"
#include "FoJsonUtils.h"

#define RETURNAS_NETCDF "netcdf"
#define RETURNAS_NETCDF4 "netcdf-4"

void
FoJsonBaseType::convert( vector<string> embed )
{
    _embed = embed ;
    _varname = name() ;
}

/** @brief Define the variable in the netcdf file
 *
 * This method creates this variable in the netcdf file. This
 * implementation is used for only simple types (byte, short, int,
 * float, double), and not for the complex types (str, structure, array,
 * grid, sequence)
 *
 * @param ncid Id of the NetCDF file
 * @throws BESInternalError if defining the variable fails
 */
void
FoJsonBaseType::define( int ncid )
{
    if( !_defined )
    {
	_varname = FoJsonUtils::gen_name( _embed, _varname, _orig_varname ) ;
	BESDEBUG( "fonc", "FoJsonBaseType::define - defining "
			  << _varname << endl ) ;
	int stax = nc_def_var( ncid, _varname.c_str(), type(),
			       0, NULL, &_varid ) ;
	if( stax != NC_NOERR )
	{
	    string err = (string)"fileout.netcdf - "
			 + "Failed to define variable "
			 + _varname ;
	    FoJsonUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
	}

	BESDEBUG( "fonc", "FoJsonBaseType::define - done defining "
			  << _varname << endl ) ;
    }
}

/** @brief Returns the type of data of this variable
 *
 * This implementation of the method returns the default type of data.
 * Subclasses of FoJsonBaseType will return the specific type of data for
 * simple types
 */
nc_type
FoJsonBaseType::type()
{
    return NC_NAT ; // the constant ncdf uses to define simple type
}

/** @brief Clears the list of embedded variable names
 */
void
FoJsonBaseType::clear_embedded()
{
    _embed.clear() ;
}

/** @brief Identifies variable with use of NetCDF4 features
 */
void FoJsonBaseType::setVersion(string version)
{
	_ncVersion = version;

	BESDEBUG( "fonc", "FoJsonBaseType::setVersion: "
			<< _ncVersion << endl ) ;
}

#if 0
/** @brief Returns true if NetCDF4 features will be required
 */
bool FoJsonBaseType::isNetCDF4()
{

	int stax = NC_NOERR;

	if ( FoJsonBaseType::_ncVersion == RETURNAS_NETCDF4 ) {
		return true;
	}

	if ( FoJsonBaseType::_ncVersion == RETURNAS_NETCDF ) {
		return false;
	}
	else {
		string err = (string)"fileout.netcdf - "
					 + "Failed to define netcdf version: variable "
					 + _varname ;
		FoJsonUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;

		return false;
	}
}
#endif
