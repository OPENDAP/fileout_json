// FoJsonUtils.cc

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

#include "config.h"
#include "FoJsonUtils.h"
#include "FoJsonByte.h"
#include "FoJsonStr.h"
#include "FoJsonShort.h"
#include "FoJsonInt.h"
#include "FoJsonFloat.h"
#include "FoJsonDouble.h"
#include "FoJsonStructure.h"
#include "FoJsonGrid.h"
#include "FoJsonArray.h"
#include "FoJsonSequence.h"

#include <BESInternalError.h>

/** @brief If a variable name, dimension name, or attribute name begins
 * with a character that is not supported by netcdf, then use this
 * prefix to prepend to the name.
 */
string FoJsonUtils::name_prefix = "" ;

/** @brief Resets the FoJson transformation for a new input and out file
 */
void
FoJsonUtils::reset()
{
    FoJsonArray::Dimensions.clear() ;
    FoJsonGrid::Maps.clear() ;
    FoJsonDim::DimNameNum = 0 ;
}

/** @brief convert the provided string to a netcdf allowed
 * identifier.
 *
 * The function makes a copy of the incoming parameter to use and
 * returns the new string.
 *
 * @param in identifier to convert
 * @returns new netcdf compliant identifier
 */
string
FoJsonUtils::id2netcdf( string in )
{
    // string of allowed characters in netcdf naming convention
    string allowed = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+_.@" ;
    // string of allowed first characters in netcdf naming
    // convention
    string first = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_" ;

    string::size_type i = 0;

    while( (i = in.find_first_not_of( allowed, i ) ) != string::npos)
    {
	in.replace( i, 1, "_" ) ;
	i++ ;
    }

    if( first.find( in[0] ) == string::npos )
    {
	in = FoJsonUtils::name_prefix + in ;
    }

    return in;
}

/** @brief translate the OPeNDAP data type to a netcdf data type
 *
 * @param element The OPeNDAP element to translate
 * @return the netcdf data type
 */
nc_type
FoJsonUtils::get_nc_type( BaseType *element )
{
    nc_type x_type = NC_NAT ; // the constant ncdf uses to define simple type

    string var_type = element->type_name() ;
    if( var_type == "Byte" )        	// check this for dods type
	x_type = NC_BYTE ;
    else if( var_type == "String" )
	x_type = NC_CHAR ;
    else if( var_type == "Int16" )
	x_type = NC_SHORT ;
    // The attribute of UInt16 maps to NC_INT, so we need to map UInt16
    // to NC_INT for the variable so that end_def won't complain about
    // the inconsistent datatype between fillvalue and the variable. KY 2012-10-25
    //else if( var_type == "UInt16" )
    //  x_type = NC_SHORT ;
    else if( var_type == "UInt16" )
	x_type = NC_INT ;
    else if( var_type == "Int32" )
	x_type = NC_INT ;
    else if( var_type == "UInt32" )
	x_type = NC_INT ;
    else if( var_type == "Float32" )
	x_type = NC_FLOAT ;
    else if( var_type == "Float64" )
	x_type = NC_DOUBLE ;

    return x_type ;
}

/** @brief generate a new name for the embedded variable
 *
 * This function takes the name of a variable as it exists in a data
 * file, and generates a new name given that netcdf does not have
 * structures or grids. Variables within structures and grids are
 * considered embedded variables, so a new name needs to be generated.
 *
 * The new name is then passed top id2netcdf to remove any characters
 * that are not allowed by netcdf.
 *
 * @param embed A list of names for parent structures
 * @param name The name of the variable to use for the new name
 * @param original The variable name before calling id2netcdf
 * @returns the newly generated name with embedded names preceeding it,
 * and converted using id2netcdf
 */
string
FoJsonUtils::gen_name( const vector<string> &embed, const string &name,
		     string &original )
{
    string new_name ;
    vector<string>::const_iterator i = embed.begin() ;
    vector<string>::const_iterator e = embed.end() ;
    bool first = true ;
    for( ; i != e; i++ )
    {
	if( first ) new_name = (*i) ;
	else new_name += FONC_EMBEDDED_SEPARATOR + (*i) ;
	first = false ;
    }
    if( first ) new_name = name ;
    else new_name += FONC_EMBEDDED_SEPARATOR + name ;

    original = new_name ;

    return FoJsonUtils::id2netcdf( new_name ) ;
}

/** @brief creates a FoJson object for the given DAP object
 *
 * @param v The DAP object to convert
 * @returns The FoJson object created via the DAP object
 * @throws BESInternalError if the DAP object is not an expected type
 */
FoJsonBaseType *
FoJsonUtils::convert( BaseType *v )
{
    FoJsonBaseType *b = 0 ;
    switch( v->type() )
    {
	case dods_str_c:
	case dods_url_c:
	    b = new FoJsonStr( v ) ;
	    break ;
	case dods_byte_c:
	    b = new FoJsonByte( v ) ;
	    break ;
	case dods_int16_c:
	case dods_uint16_c:
	    b = new FoJsonShort( v ) ;
	    break ;
	case dods_int32_c:
	case dods_uint32_c:
	    b = new FoJsonInt( v ) ;
	    break ;
	case dods_float32_c:
	    b = new FoJsonFloat( v ) ;
	    break ;
	case dods_float64_c:
	    b = new FoJsonDouble( v ) ;
	    break ;
	case dods_grid_c:
	    b = new FoJsonGrid( v ) ;
	    break ;
	case dods_array_c:
	    b = new FoJsonArray( v ) ;
	    break ;
	case dods_structure_c:
	    b = new FoJsonStructure( v ) ;
	    break ;
	case dods_sequence_c:
	    b = new FoJsonSequence( v ) ;
	    break ;
	default:
	    string err = (string)"file out netcdf, unable to "
			 + "write unknown variable type" ;
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;

    }
    return b ;
}

/** @brief handle any netcdf errors
 *
 * Looks up the netcdf error message associated with the provided netcdf
 * return value and throws an exception with that information appended to
 * the provided error message
 *
 * @param stax A netcdf return value, NC_NOERR if no error occurred
 * @param err A provided error message to begin the error message with
 * @param file The source code file name where the error was generated
 * @param line The source code line number where the error was generated
 * @throws BESInternalError if the return value represents a netcdf error
 */
void
FoJsonUtils::handle_error( int stax, string &err, const string &file, int line )
{
    if( stax != NC_NOERR )
    {
	const char *nerr = nc_strerror( stax ) ;
	if( nerr )
	{
	    err += (string)": " + nerr ;
	}
	else
	{
	    err += (string)": unknown error" ;
	}
	throw BESInternalError( err, file, line ) ;
    }
}

