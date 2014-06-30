// FoJsonStructure.cc

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

#include <BESInternalError.h>
#include <BESDebug.h>

#include "FoJsonStructure.h"
#include "FoJsonUtils.h"
#include "FoJsonAttributes.h"

/** @brief Constructor for FoJsonStructure that takes a DAP Structure
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * Structure instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be a Structure
 * @throws BESInternalError if the BaseType is not a Structure
 */
FoJsonStructure::FoJsonStructure( BaseType *b )
    : FoJsonBaseType(), _s( 0 )
{
    _s = dynamic_cast<Structure *>(b) ;
    if( !_s )
    {
	string s = (string)"File out netcdf, write_structure was passed a "
		   + "variable that is not a structure" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

/** @brief Destructor that cleans up the structure
 *
 * Delete each of the FoJsonBaseType instances that is a part of this
 * structure.
 */
FoJsonStructure::~FoJsonStructure()
{
    bool done = false ;
    while( !done )
    {
	vector<FoJsonBaseType *>::iterator i = _vars.begin() ;
	vector<FoJsonBaseType *>::iterator e = _vars.end() ;
	if( i == e )
	{
	    done = true ;
	}
	else
	{
	    // These are the FoJson types, not the actual ones
	    FoJsonBaseType *b = (*i) ;
	    delete b ;
	    _vars.erase( i ) ;
	}
    }
}

/** @brief Creates the FoJson objects for each variable of the structure
 *
 * For each of the variables of the DAP Structure we convert to a
 * similar FoJson object. Because NetCDF does not support structures, we
 * must flatten out the structures. To do this, we embed the name of the
 * structure as part of the name of the children variables. For example,
 * if the structure, called s1, contains an array called a1 and an int
 * called i1, then two variables are created in the netcdf file called
 * s1.a1 and s1.i1.
 *
 * It only converts the variables that are to be sent
 *
 * @param embed The parent names of this structure.
 * @throws BESInternalError if there is a problem converting this
 * structure
 */
void
FoJsonStructure::convert( vector<string> embed )
{
    FoJsonBaseType::convert( embed ) ;
    embed.push_back( name() ) ;
    Constructor::Vars_iter vi = _s->var_begin() ;
    Constructor::Vars_iter ve = _s->var_end() ;
    for( ; vi != ve; vi++ )
    {
	BaseType *bt = *vi ;
	if( bt->send_p() )
	{
	    BESDEBUG( "fonc", "FoJsonStructure::convert - converting "
			      << bt->name() << endl ) ;
	    FoJsonBaseType *fbt = FoJsonUtils::convert( bt ) ;
	    _vars.push_back( fbt ) ;
	    fbt->convert( embed ) ;
	}
    }
}

/** @brief Define the members of the structure in the netcdf file
 *
 * Since netcdf does not support structures, we define the members of
 * the structure to include the name of the structure in their name.
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem defining the structure
 */
void
FoJsonStructure::define( int ncid )
{
    if( !_defined )
    {
	BESDEBUG( "fonc", "FoJsonStructure::define - defining "
			  << _varname << endl ) ;
	vector<FoJsonBaseType *>::const_iterator i = _vars.begin() ;
	vector<FoJsonBaseType *>::const_iterator e = _vars.end() ;
	for( ; i != e; i++ )
	{
	    FoJsonBaseType *fbt = (*i) ;
	    BESDEBUG( "fonc", "defining " << fbt->name() << endl ) ;
	    fbt->define( ncid ) ;
	}

	_defined = true ;

	BESDEBUG( "fonc", "FoJsonStructure::define - done defining "
			  << _varname << endl ) ;
    }
}

/** @brief write the member variables of the structure to the netcdf
 * file
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing out the
 * members of the structure.
 */
void
FoJsonStructure::write( int ncid )
{
    BESDEBUG( "fonc", "FoJsonStructure::write - writing "
		      << _varname << endl ) ;
    vector<FoJsonBaseType *>::const_iterator i = _vars.begin() ;
    vector<FoJsonBaseType *>::const_iterator e = _vars.end() ;
    for( ; i != e; i++ )
    {
	FoJsonBaseType *fbt = (*i) ;
	fbt->write( ncid ) ;
    }
    BESDEBUG( "fonc", "FoJsonStructure::define - done writing "
		      << _varname << endl ) ;
}

/** @brief Returns the name of the structure
 *
 * @returns The name of the structure
 */
string
FoJsonStructure::name()
{
    return _s->name() ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data,
 * including the members of the structure by calling dump on those FoJson
 * objects.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FoJsonStructure::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FoJsonStructure::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _s->name()  << " {" << endl ;
    BESIndent::Indent() ;
    vector<FoJsonBaseType *>::const_iterator i = _vars.begin() ;
    vector<FoJsonBaseType *>::const_iterator e = _vars.end() ;
    for( ; i != e; i++ )
    {
	FoJsonBaseType *fbt = *i ;
	fbt->dump( strm ) ;
    }
    BESIndent::UnIndent() ;
    strm << BESIndent::LMarg << "}" << endl ;
    BESIndent::UnIndent() ;
}

