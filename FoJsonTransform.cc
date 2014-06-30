// FoJsonTransform.cc

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

#include <sstream>
#include <iostream>
#include <fstream>
#include <stddef.h>

using std::ostringstream;
using std::istringstream;

#include "FoJsonTransform.h"

#include <DDS.h>
#include <Structure.h>
#include <Constructor.h>
#include <Array.h>
#include <Grid.h>
#include <Sequence.h>
#include <BESDebug.h>
#include <BESInternalError.h>

#define ATTRIBUTE_SEPARATOR "."
#define JSON_ORIGINAL_NAME "json_original_name"



#define FoJsonTransform_debug_key "fojson"


long computeConstrainedShape(libdap::Array *a, vector<unsigned int> *shape ){
    BESDEBUG(FoJsonTransform_debug_key, "FoJsonTransform::computeConstrainedShape() - BEGIN. Array name: "<< a->name() << endl);

    libdap::Array::Dim_iter dIt;
    unsigned int start;
    unsigned int stride;
    unsigned int stop;

    unsigned int dimSize = 1;
    int dimNum = 0;
    long totalSize = 1;

    BESDEBUG(FoJsonTransform_debug_key, "FoJsonTransform::computeConstrainedShape() - Array has " << a->dimensions(true) << " dimensions."<< endl);

    stringstream msg;

    for(dIt = a->dim_begin() ; dIt!=a->dim_end() ;dIt++){
        BESDEBUG(FoJsonTransform_debug_key, "FoJsonTransform::computeConstrainedShape() - Processing dimension '" << a->dimension_name(dIt)<< "'. (dim# "<< dimNum << ")"<< endl);
        start  = a->dimension_start(dIt, true);
        stride = a->dimension_stride(dIt, true);
        stop   = a->dimension_stop(dIt, true);
        BESDEBUG(FoJsonTransform_debug_key, "FoJsonTransform::computeConstrainedShape() - start: " << start << "  stride: " << stride << "  stop: "<<stop<< endl);

        dimSize = 1 + ( (stop - start) / stride);
        BESDEBUG(FoJsonTransform_debug_key, "FoJsonTransform::computeConstrainedShape() - dimSize: " << dimSize << endl);

        (*shape)[dimNum++] = dimSize;
        totalSize *= dimSize;
    }
    BESDEBUG(FoJsonTransform_debug_key, "FoJsonTransform::computeConstrainedShape() - totalSize: " << totalSize << endl);
    BESDEBUG(FoJsonTransform_debug_key, "FoJsonTransform::computeConstrainedShape() - END." << endl);

    return totalSize;
}

template<typename T> unsigned  int json_simple_type_array_worker(ostream *strm, T *values, unsigned int indx, vector<unsigned int> *shape, unsigned int currentDim){

	*strm << "[";

	unsigned int currentDimSize = (*shape)[currentDim];

    for(int i=0; i<currentDimSize ;i++){
		if(currentDim < shape->size()-1){
			BESDEBUG(FoJsonTransform_debug_key, "json_simple_type_array_worker() - Recursing! indx:  " << indx
					<< " currentDim: " << currentDim
					<< " currentDimSize: " << currentDimSize
					<< endl);
			indx = json_simple_type_array_worker<T>(strm,values,indx,shape,currentDim+1);
			if(i+1 != currentDimSize)
		    	*strm << ", ";
		}
		else {
	    	if(i)
		    	*strm << ", ";
	    	*strm << values[indx++];
		}

    }
	*strm << "]";

	return indx;
}


template<typename T>void json_simple_type_array(ostream *strm, Array *a, string indent){

	*strm << indent << "\"" << a->name() + "\": ";
    int numDim = a->dimensions(true);
    vector<unsigned int> shape(numDim);
    long length = computeConstrainedShape(a, &shape);

    T *src = new T[length];
    a->value(src);
    unsigned int indx = json_simple_type_array_worker(strm, src, 0, &shape, 0);

    if(length != indx)
		BESDEBUG(FoJsonTransform_debug_key, "json_simple_type_array() - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);

    delete src;
}


/** @brief Constructor that creates transformation object from the specified
 * DataDDS object to the specified file
 *
 * @param dds DataDDS object that contains the data structure, attributes
 * and data
 * @param dhi The data interface containing information about the current
 * request
 * @param localfile netcdf to create and write the information to
 * @throws BESInternalError if dds provided is empty or not read, if the
 * file is not specified or failed to create the netcdf file
 */
FoJsonTransform::FoJsonTransform(DDS *dds, BESDataHandlerInterface &dhi, const string &localfile) :
        _dds(0),
        _indent_increment(" ")
{
    if (!dds) {
        string s = (string) "File out JSON, " + "null DDS passed to constructor";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    if (localfile.empty()) {
        string s = (string) "File out JSON, " + "empty local file name passed to constructor";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    _localfile = localfile;
    _dds = dds;

    // FoJsonUtils::name_prefix = "json_";
}

/** @brief Destructor
 *
 * Cleans up any temporary data created during the transformation
 */
FoJsonTransform::~FoJsonTransform()
{
}

/** @brief dumps information about this transformation object for debugging
 * purposes
 *
 * Displays the pointer value of this instance plus instance data,
 * including all of the FoJson objects converted from DAP objects that are
 * to be sent to the netcdf file.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FoJsonTransform::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FoJsonTransform::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "temporary file = " << _localfile << endl;
    if(_dds != 0){
        _dds->print(strm);
    }
    BESIndent::UnIndent();
}




#if 0
/** @brief Transforms each of the variables of the DataDDS to the NetCDF
 * file
 *
 * For each variable in the DataDDS write out that variable and its
 * attributes to the netcdf file. Each OPeNDAP data type translates into a
 * particular netcdf type. Also write out any global variables stored at the
 * top level of the DataDDS.
 */
void FoJsonTransform::transform_OLD()
{
    FoJsonUtils::reset();

    // Convert the DDS into an internal format to keep track of
    // variables, arrays, shared dimensions, grids, common maps,
    // embedded structures. It only grabs the variables that are to be
    // sent.
    DDS::Vars_iter vi = _dds->var_begin();
    DDS::Vars_iter ve = _dds->var_end();
    for (; vi != ve; vi++) {
        if ((*vi)->send_p()) {
            BaseType *v = *vi;
            BESDEBUG("fojson", "converting " << v->name() << endl);
            FoJsonBaseType *fb = FoJsonUtils::convert(v);
            fb->setVersion( FoJsonTransform::_returnAs );
            _fonc_vars.push_back(fb);
            vector<string> embed;
            fb->convert(embed);
        }
    }
    BESDEBUG("fojson", *this << endl);

    // Open the file for writing
    int stax;

#if 0
    if ( FoJsonTransform::_returnAs == RETURNAS_NETCDF4 ) {
    	stax = nc_create(_localfile.c_str(), NC_CLOBBER|NC_NETCDF4|NC_CLASSIC_MODEL, &_ncid);
    }
    else {
    	stax = nc_create(_localfile.c_str(), NC_CLOBBER, &_ncid);
    }
#endif

	stax = nc_create(_localfile.c_str(), NC_CLOBBER, &_ncid);

    if (stax != NC_NOERR) {
        string err = (string) "File out netcdf, " + "unable to open file " + _localfile;
        FoJsonUtils::handle_error(stax, err, __FILE__, __LINE__);
    }

    try {
        // Here we will be defining the variables of the netcdf and
        // adding attributes. To do this we must be in define mode.
        nc_redef(_ncid);

        // For each conerted FoJson object, call define on it to define
        // that object to the netcdf file. This also adds the attributes
        // for the variables to the netcdf file
        vector<FoJsonBaseType *>::iterator i = _fonc_vars.begin();
        vector<FoJsonBaseType *>::iterator e = _fonc_vars.end();
        for (; i != e; i++) {
            FoJsonBaseType *fbt = *i;
            fbt->define(_ncid);
        }

        // Add any global attributes to the netcdf file
        AttrTable &globals = _dds->get_attr_table();
        BESDEBUG("fojson", "Adding Global Attributes" << endl << globals << endl);
        FoJsonAttributes::addattrs(_ncid, NC_GLOBAL, globals, "", "");

        // We are done defining the variables, dimensions, and
        // attributes of the netcdf file. End the define mode.
        int stax = nc_enddef(_ncid);

        // Check error for nc_enddef. Handling of HDF failures
        // can be detected here rather than later.  KY 2012-10-25
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, " + "unable to end the define mode " + _localfile;
            FoJsonUtils::handle_error(stax, err, __FILE__, __LINE__);
        }

        // Write everything out
        i = _fonc_vars.begin();
        e = _fonc_vars.end();
        for (; i != e; i++) {
            FoJsonBaseType *fbt = *i;
            fbt->write(_ncid);
        }
    }
    catch (BESError &e) {
        nc_close(_ncid);
        throw;
    }

    nc_close(_ncid);
}
#endif


/** @brief Transforms each of the variables of the DataDDS to the NetCDF
 * file
 *
 * For each variable in the DataDDS write out that variable and its
 * attributes to the netcdf file. Each OPeNDAP data type translates into a
 * particular netcdf type. Also write out any global variables stored at the
 * top level of the DataDDS.
 */
void FoJsonTransform::transform()
{
    // FoJsonUtils::reset();

    ofstream fileStrm;
    fileStrm.open (_localfile.c_str());

    try {
        transform( &fileStrm, _dds,"");
    }
    catch (BESError &e) {
    	fileStrm.close();
        throw;
    }
    fileStrm.close();

}

void FoJsonTransform::transform(ostream *strm, DDS *dds, string indent){

	string s = "";

	*strm << "{" << endl << "\"" << dds->get_dataset_name() << "\": {" << endl;
    if(dds->num_var() > 0){

		DDS::Vars_iter vi = dds->var_begin();
		DDS::Vars_iter ve = dds->var_end();
		for (; vi != ve; vi++) {
			if ((*vi)->send_p()) {
				BaseType *v = *vi;
				BESDEBUG(FoJsonTransform_debug_key, "Processing top level variable: " << v->name() << endl);
				transform(strm, v, indent + _indent_increment);
				if( (vi+1) != ve ){
					*strm << "," ;
				}
				*strm << endl ;

			}

		}
    }
    *strm <<  s <<  "}" << endl << "}" << endl;

}

void FoJsonTransform::transform(ostream *strm, BaseType *bt, string  indent)
{
	switch(bt->type()){
	// Handle the atomic types - that's easy!
	case dods_byte_c:
	case dods_int16_c:
	case dods_uint16_c:
	case dods_int32_c:
	case dods_uint32_c:
	case dods_float32_c:
	case dods_float64_c:
	case dods_str_c:
		transformAtomic(strm, bt, indent);
		break;

	case dods_structure_c:
		transform(strm, (Structure *) bt, indent);
		break;

	case dods_grid_c:
		transform(strm, (Grid *) bt, indent);
		break;

	case dods_sequence_c:
		transform(strm, (Sequence *) bt, indent);
		break;

	case dods_array_c:
		transform(strm, (Array *) bt, indent);
		break;


	case dods_int8_c:
	case dods_uint8_c:
	case dods_int64_c:
	case dods_uint64_c:
	case dods_url4_c:
	case dods_enum_c:
	case dods_group_c:
	{
		string s = (string) "File out JSON, " + "DAP4 types not yet supported.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	default:
	{
		string s = (string) "File out JSON, " + "Unrecognized type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	}

}


void FoJsonTransform::transformAtomic(ostream *strm, BaseType *b, string indent){
	*strm << indent << "\"" << b->name() <<  "\": ";
	b->print_val(*strm, "", false);
}


void FoJsonTransform::transform(ostream *strm, Structure *b, string indent){

	*strm << indent << "\"" << b->name() << "\": {" << endl;
    if(b->width(true) > 0){

		DDS::Vars_iter vi = b->var_begin();
		DDS::Vars_iter ve = b->var_end();
		for (; vi != ve; vi++) {
			if ((*vi)->send_p()) {
				BaseType *v = *vi;
				BESDEBUG(FoJsonTransform_debug_key, "FoJsonTransform::transform() - Processing structure variable: " << v->name() << endl);
				transform(strm, v, indent + _indent_increment );
				if( (vi+1) != ve ){
					*strm << "," ;
				}
				*strm << endl;
			}
		}
    }
    *strm << indent << "}";
}


void FoJsonTransform::transform(ostream *strm, Grid *g, string indent){

    *strm << indent << "\"" << g->name() << "\": {" << endl;

	BESDEBUG(FoJsonTransform_debug_key, "FoJsonTransform::transform() - Processing Grid data Array: " << g->get_array()->name() << endl);

	transform(strm, g->get_array(), indent+_indent_increment);
    *strm << "," << endl;

	for(Grid::Map_iter mapi=g->map_begin(); mapi < g->map_end(); mapi++){
		BESDEBUG(FoJsonTransform_debug_key, "FoJsonTransform::transform() - Processing Grid Map Array: " << (*mapi)->name() << endl);
		if(mapi != g->map_begin()){
		    *strm << "," << endl;
		}
		transform(strm, *mapi, indent+_indent_increment);
	}
    *strm << endl << indent << "}";

}

void FoJsonTransform::transform(ostream *strm, Sequence *s, string indent){

	*strm << indent << "\"" <<  s->name() << "\": {" << endl;

	string child_indent = indent + _indent_increment;


#if 0
	the erdap way
	*strm << indent << "\"table\": {" << endl;

	string child_indent = indent + _indent_increment;

	*strm << child_indent << "\"name\": \"" << s->name() << "\"," << endl;


#endif



	*strm << child_indent <<"\"columnNames\": [";
	for(Constructor::Vars_iter v=s->var_begin(); v<s->var_end() ; v++){
		if(v!=s->var_begin())
			*strm << ",";
		*strm << "\"" << (*v)->name() << "\"";
	}
	*strm <<"]," << endl;

	*strm << child_indent <<"\"columnTypes\": [";
	for(Constructor::Vars_iter v=s->var_begin(); v<s->var_end() ; v++){
		if(v!=s->var_begin())
			*strm << ",";
		*strm << "\"" << (*v)->type_name() << "\"";
	}
	*strm <<"]," << endl;

	bool first = true;
	*strm << child_indent <<"\"rows\": [";
	while(s->read()){
		if(!first)
			*strm << ", ";
		*strm << endl << child_indent << "[";
		for(Constructor::Vars_iter v=s->var_begin(); v<s->var_end() ; v++){
			if(v!=s->var_begin())
				*strm << child_indent  <<",";
			transform(strm, (*v),child_indent+_indent_increment);
		}
		*strm << child_indent <<"]";
		first = false;
	}
	*strm << endl << child_indent << "]" << endl;

    *strm << indent << "}" << endl;

}


void FoJsonTransform::transform(ostream *strm, Array *a, string indent){


	switch(a->var()->type()){
	// Handle the atomic types - that's easy!
	case dods_byte_c:
		json_simple_type_array<dods_byte>(strm,a,indent);
		break;

	case dods_int16_c:
		json_simple_type_array<dods_int16>(strm,a,indent);
		break;

	case dods_uint16_c:
		json_simple_type_array<dods_uint16>(strm,a,indent);
		break;

	case dods_int32_c:
		json_simple_type_array<dods_int32>(strm,a,indent);
		break;

	case dods_uint32_c:
		json_simple_type_array<dods_uint32>(strm,a,indent);
		break;

	case dods_float32_c:
		json_simple_type_array<dods_float32>(strm,a,indent);
    	break;

	case dods_float64_c:
		json_simple_type_array<dods_float64>(strm,a,indent);
		break;

	case dods_structure_c:
	{
		string s = (string) "File out JSON, " + "Arrays of Structure objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}
	case dods_grid_c:
	{
		string s = (string) "File out JSON, " + "Arrays of Grid objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	case dods_sequence_c:
	{
		string s = (string) "File out JSON, " + "Arrays of Sequence objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	case dods_array_c:
	{
		string s = (string) "File out JSON, " + "Arrays of Array objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}
	case dods_int8_c:
	case dods_uint8_c:
	case dods_int64_c:
	case dods_uint64_c:
	case dods_url4_c:
	case dods_enum_c:
	case dods_group_c:
	{
		string s = (string) "File out JSON, " + "DAP4 types not yet supported.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	default:
	{
		string s = (string) "File out JSON, " + "Unrecognized type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	}

}








