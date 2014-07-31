// -*- mode: c++; c-basic-offset:4 -*-
//
// FoJsonTransform.cc
//
// This file is part of BES JSON File Out Module
//
// Copyright (c) 2014 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include "config.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <stddef.h>


#include "FoJsonTransform.h"

#include <DDS.h>
#include <Structure.h>
#include <Constructor.h>
#include <Array.h>
#include <Grid.h>
#include <Sequence.h>
#include <Str.h>
#include <Url.h>

#include <BESDebug.h>
#include <BESInternalError.h>

#include <utils.h>



#define ATTRIBUTE_SEPARATOR "."
#define JSON_ORIGINAL_NAME "json_original_name"



#define FoJsonTransform_debug_key "fojson"

/**
 * Writes out the values of an n-dimensional array. Uses recursion.
 */
template<typename T> unsigned  int FoJsonTransform::json_simple_type_array_worker(std::ostream *strm, T *values, unsigned int indx, std::vector<unsigned int> *shape, unsigned int currentDim){

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


/**
 * @brief Writes out (in a JSON instance object representation) the metadata and data values for the passed array of simple types.
 *
 * @param strm Where to write stuff
 * @param a The libdap::Array to write.
 * @param indent A string containing the indent level.
 * @param sendData A boolean value that when evaluated as true will cause the data values to be sent and not the metadata.
 */
template<typename T>void FoJsonTransform::json_simple_type_array(std::ostream *strm, libdap::Array *a, string indent, bool sendData){

	*strm << indent << "\"" << a->name() + "\":  ";



    if(sendData){ // send data
        int numDim = a->dimensions(true);
        std::vector<unsigned int> shape(numDim);
        long length = computeConstrainedShape(a, &shape);


    	//*strm << "," << endl;

		T *src = new T[length];
		a->value(src);
		unsigned int indx = json_simple_type_array_worker(strm, src, 0, &shape, 0);

		if(length != indx)
			BESDEBUG(FoJsonTransform_debug_key, "json_simple_type_array() - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);
		delete src;
    }
    else { // otherwise send metadata
    	*strm << "{" << endl;
    	//Attributes
    	transform(strm, a->get_attr_table(), indent+_indent_increment);
    	*strm << endl;
    	*strm << indent << "}";
    }

}



/**
 * @brief Constructor that creates transformation object from the specified
 * DataDDS object to the specified file
 *
 * Build a FoJsonTransform object.
 *
 * @param dds DataDDS object that contains the data structure, attributes
 * and data
 * @param dhi The data interface containing information about the current
 * request
 * @param localfile The file to create and write the JSON document into.
 * @throws BESInternalError if dds provided is empty or not read, if the
 * file is not specified or failed to create the netcdf file
 */
FoJsonTransform::FoJsonTransform(libdap::DDS *dds, BESDataHandlerInterface &dhi, const string &localfile) :
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
 * to be sent to the JSON file.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FoJsonTransform::dump(std::ostream &strm) const
{
    strm << BESIndent::LMarg << "FoJsonTransform::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "temporary file = " << _localfile << endl;
    if(_dds != 0){
        _dds->print(strm);
    }
    BESIndent::UnIndent();
}




/** @brief Transforms the DDS object into a JSON instance object representation.
 *
 * Transforms the DDS and all of it's "projected" variables into a JSON document using
 * an instance object representation.
 * @param sendData If the sendData parameter is true data will be
 * sent. If sendData is false then the metadata will be sent.
 */
void FoJsonTransform::transform(bool sendData)
{
    // FoJsonUtils::reset();

    ofstream fileStrm;
    fileStrm.open (_localfile.c_str());

    try {
        transform( &fileStrm, _dds,"", sendData);
    }
    catch (BESError &e) {
    	fileStrm.close();
        throw;
    }
    fileStrm.close();

}




/** @brief Transforms the DDS object into a JSON instance object representation.
 *
 * Transforms the DDS and all of it's "projected" variables into a JSON document using
 * an instance object representation.
 *
 * @param strm Stream to which to write JSON.
 * @param dds The DDS to produce JSON from.
 * @param indent White space indent.
 * @param sendData If the sendData parameter is true data will be sent. If sendData is false then the metadata will be sent.
 */
void FoJsonTransform::transform(std::ostream *strm, libdap::DDS *dds, string indent, bool sendData){


	bool sentSomething = false;


	// Open returned JSON object
	*strm << "{" << endl;



	// Name object
	*strm << indent + _indent_increment << "\"name\": \"" << dds->get_dataset_name() << "\"," << endl;

	if(!sendData){
		// Send metadata if we aren't sending data

		//Attributes
		transform(strm, dds->get_attr_table(), indent);
		if(dds->get_attr_table().get_size()>0)
			*strm << ",";
		*strm << endl;
	}

	// Process the variables in the DDS
    if(dds->num_var() > 0){

		libdap::DDS::Vars_iter vi = dds->var_begin();
		libdap::DDS::Vars_iter ve = dds->var_end();
		for (; vi != ve; vi++) {
			if ((*vi)->send_p()) {

				libdap::BaseType *v = *vi;
				BESDEBUG(FoJsonTransform_debug_key, "Processing top level variable: " << v->name() << endl);

				if( sentSomething ){
					*strm << "," ;
					*strm << endl ;
				}
				transform(strm, v, indent + _indent_increment, sendData);

				sentSomething = true;
			}

		}
    }

    // Close the JSON object
    *strm << endl << "}" << endl;

}

/** @brief Transforms the BaseType object into a JSON instance object representation.
 *
 * Transforms the BaseType into a JSON document using an instance object representation.
 *
 * @param strm Stream to which to write JSON.
 * @param bt The BaseType to produce JSON from.
 * @param indent White space indent.
 * @param sendData If the sendData parameter is true data will be sent. If sendData is false then the metadata will be sent.
 */
void FoJsonTransform::transform(std::ostream *strm, libdap::BaseType *bt, string  indent, bool sendData)
{
	switch(bt->type()){
	// Handle the atomic types - that's easy!
	case libdap::dods_byte_c:
	case libdap::dods_int16_c:
	case libdap::dods_uint16_c:
	case libdap::dods_int32_c:
	case libdap::dods_uint32_c:
	case libdap::dods_float32_c:
	case libdap::dods_float64_c:
	case libdap::dods_str_c:
	case libdap::dods_url_c:
		transformAtomic(strm, bt, indent, sendData);
		break;

	case libdap::dods_structure_c:
		transform(strm, (libdap::Structure *) bt, indent, sendData);
		break;

	case libdap::dods_grid_c:
		transform(strm, (libdap::Grid *) bt, indent, sendData);
		break;

	case libdap::dods_sequence_c:
		transform(strm, (libdap::Sequence *) bt, indent, sendData);
		break;

	case libdap::dods_array_c:
		transform(strm, (libdap::Array *) bt, indent, sendData);
		break;


	case libdap::dods_int8_c:
	case libdap::dods_uint8_c:
	case libdap::dods_int64_c:
	case libdap::dods_uint64_c:
	case libdap::dods_url4_c:
	case libdap::dods_enum_c:
	case libdap::dods_group_c:
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


/** @brief Transforms the BaseType object into a JSON instance object representation.
 *
 * Transforms the BaseType into a JSON document using an instance object representation. The assumption here is
 * that the passed BaseType is an "atomic" type.
 *
 * @param strm Stream to which to write JSON.
 * @param bt The BaseType to produce JSON from.
 * @param indent White space indent.
 * @param sendData If the sendData parameter is true data will be sent. If sendData is false then the metadata will be sent.
 */
void FoJsonTransform::transformAtomic(std::ostream *strm, libdap::BaseType *b, string indent, bool sendData){
	*strm << indent << "\"" << b->name() <<  "\": ";

	if(sendData){
		b->print_val(*strm, "", false);
	}
	else {
		transform(strm, b->get_attr_table(), indent);
	}
}


/** @brief Transforms the Structure object into a JSON instance object representation.
 *
 * Transforms the Structure into a JSON document using an instance object representation.
 *
 * @param strm Stream to which to write JSON.
 * @param bt The Structure to produce JSON from.
 * @param indent White space indent.
 * @param sendData If the sendData parameter is true data will be sent. If sendData is false then the metadata will be sent.
 */
void FoJsonTransform::transform(std::ostream *strm, libdap::Structure *b, string indent, bool sendData){

	// Open object with name of the structure
	*strm << indent << "\"" << b->name() << "\": {" << endl;

	// Process the variables.
    if(b->width(true) > 0){

    	libdap::Structure::Vars_iter vi = b->var_begin();
    	libdap::Structure::Vars_iter ve = b->var_end();
		for (; vi != ve; vi++) {

			// If the variable is projected, send (transform) it.
			if ((*vi)->send_p()) {
				libdap::BaseType *v = *vi;
				BESDEBUG(FoJsonTransform_debug_key, "FoJsonTransform::transform() - Processing structure variable: " << v->name() << endl);
				transform(strm, v, indent + _indent_increment, sendData );
				if( (vi+1) != ve ){
					*strm << "," ;
				}
				*strm << endl;
			}
		}
    }
    *strm << indent << "}";
}


/** @brief Transforms the Grid object into a JSON instance object representation.
 *
 * Transforms the Grid into a JSON document using an instance object representation.
 *
 * @param strm Stream to which to write JSON.
 * @param bt The Grid to produce JSON from.
 * @param indent White space indent.
 * @param sendData If the sendData parameter is true data will be sent. If sendData is false then the metadata will be sent.
 */
void FoJsonTransform::transform(std::ostream *strm, libdap::Grid *g, string indent, bool sendData){

	// Open JSON property object with name of the grid
    *strm << indent << "\"" << g->name() << "\": {" << endl;

	BESDEBUG(FoJsonTransform_debug_key, "FoJsonTransform::transform() - Processing Grid data Array: " << g->get_array()->name() << endl);

	// Process the data array
	transform(strm, g->get_array(), indent+_indent_increment, sendData);
    *strm << "," << endl;

    // Process the MAP arrays
	for(libdap::Grid::Map_iter mapi=g->map_begin(); mapi < g->map_end(); mapi++){
		BESDEBUG(FoJsonTransform_debug_key, "FoJsonTransform::transform() - Processing Grid Map Array: " << (*mapi)->name() << endl);
		if(mapi != g->map_begin()){
		    *strm << "," << endl;
		}
		transform(strm, *mapi, indent+_indent_increment, sendData);
	}
	// Close the JSON property object
    *strm << endl << indent << "}";

}

/** @brief Transforms the Sequence object into a JSON instance object representation.
 *
 * Transforms the Sequence into a JSON document using an instance object representation.
 *
 * @param strm Stream to which to write JSON.
 * @param s The Sequence to produce JSON from.
 * @param indent White space indent.
 * @param sendData If the sendData parameter is true data will be sent. If sendData is false then the metadata will be sent.
 */
void FoJsonTransform::transform(std::ostream *strm, libdap::Sequence *s, string indent, bool sendData){

	// Open JSON property object with name of the sequence
	*strm << indent << "\"" <<  s->name() << "\": {" << endl;

	string child_indent = indent + _indent_increment;

#if 0
	the erdap way
	*strm << indent << "\"table\": {" << endl;

	string child_indent = indent + _indent_increment;

	*strm << child_indent << "\"name\": \"" << s->name() << "\"," << endl;


#endif

	*strm << child_indent <<"\"columnNames\": [";
	for(libdap::Constructor::Vars_iter v=s->var_begin(); v<s->var_end() ; v++){
		if(v!=s->var_begin())
			*strm << ",";
		*strm << "\"" << (*v)->name() << "\"";
	}
	*strm <<"]," << endl;

	*strm << child_indent <<"\"columnTypes\": [";
	for(libdap::Constructor::Vars_iter v=s->var_begin(); v<s->var_end() ; v++){
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
		for(libdap::Constructor::Vars_iter v=s->var_begin(); v<s->var_end() ; v++){
			if(v!=s->var_begin())
				*strm << child_indent  <<",";
			transform(strm, (*v),child_indent+_indent_increment, sendData);
		}
		*strm << child_indent <<"]";
		first = false;
	}
	*strm << endl << child_indent << "]" << endl;

	// Close the JSON property object
    *strm << indent << "}" << endl;
}


/** @brief Transforms the Array object into a JSON instance object representation.
 *
 * Transforms the Array into a JSON document using an instance object representation.
 *
 * @param strm Stream to which to write JSON.
 * @param a The Array to produce JSON from.
 * @param indent White space indent.
 * @param sendData If the sendData parameter is true data will be sent. If sendData is false then the metadata will be sent.
 */
void FoJsonTransform::transform(std::ostream *strm, libdap::Array *a, string indent, bool sendData){

    BESDEBUG(FoJsonTransform_debug_key, "FoJsonTransform::transform() - Processing Array. "
            << " a->type(): " << a->type()
			<< " a->var()->type(): " << a->var()->type()
			<< endl);

	switch(a->var()->type()){
	// Handle the atomic types - that's easy!
	case libdap::dods_byte_c:
		json_simple_type_array<libdap::dods_byte>(strm,a,indent, sendData);
		break;

	case libdap::dods_int16_c:
		json_simple_type_array<libdap::dods_int16>(strm,a,indent, sendData);
		break;

	case libdap::dods_uint16_c:
		json_simple_type_array<libdap::dods_uint16>(strm,a,indent, sendData);
		break;

	case libdap::dods_int32_c:
		json_simple_type_array<libdap::dods_int32>(strm,a,indent, sendData);
		break;

	case libdap::dods_uint32_c:
		json_simple_type_array<libdap::dods_uint32>(strm,a,indent, sendData);
		break;

	case libdap::dods_float32_c:
		json_simple_type_array<libdap::dods_float32>(strm,a,indent, sendData);
    	break;

	case libdap::dods_float64_c:
		json_simple_type_array<libdap::dods_float64>(strm,a,indent, sendData);
		break;

	case libdap::dods_str_c:
	{
		//json_simple_type_array<Str>(strm,a,indent);
		string s = (string) "File out JSON, " + "Arrays of Strings are not yet a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	case libdap::dods_url_c:
	{
		//json_simple_type_array<Url>(strm,a,indent);
		string s = (string) "File out JSON, " + "Arrays of URLs are not yet a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	case libdap::dods_structure_c:
	{
		string s = (string) "File out JSON, " + "Arrays of Structure objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}
	case libdap::dods_grid_c:
	{
		string s = (string) "File out JSON, " + "Arrays of Grid objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	case libdap::dods_sequence_c:
	{
		string s = (string) "File out JSON, " + "Arrays of Sequence objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	case libdap::dods_array_c:
	{
		string s = (string) "File out JSON, " + "Arrays of Array objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}
	case libdap::dods_int8_c:
	case libdap::dods_uint8_c:
	case libdap::dods_int64_c:
	case libdap::dods_uint64_c:
	case libdap::dods_url4_c:
	case libdap::dods_enum_c:
	case libdap::dods_group_c:
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


/** @brief Transforms the AttrTable object into a JSON instance object representation.
 *
 * Transforms the AttrTable into a JSON document using an instance object representation.
 * Since Attributes get promoted to "properties" of their parent container (either a
 * BaseType variable or a DDS) the is method does not open a new JSON object, but rather
 * continues to add content to the currently open object.
 *
 * @param strm Stream to which to write JSON.
 * @param a The AttrTable to produce JSON from.
 * @param indent White space indent.
 * @param sendData If the sendData parameter is true data will be sent. If sendData is false then the metadata will be sent.
 */
void FoJsonTransform::transform(std::ostream *strm, libdap::AttrTable &attr_table, string  indent){

	/*
	 * Since Attributes get promoted to JSON "properties" of their parent object (either a
	 * BaseType variable or a DDS derived JSON object) this method does not open a new JSON
	 * object, but rather continues to add content to the currently open object.
	 */
	string child_indent = indent + _indent_increment;


	// Are there any attributes?
	if(attr_table.get_size() != 0) {
		//*strm << endl;
		libdap::AttrTable::Attr_iter begin = attr_table.attr_begin();
		libdap::AttrTable::Attr_iter end = attr_table.attr_end();


		// Process each Attribute
		for(libdap::AttrTable::Attr_iter at_iter=begin; at_iter !=end; at_iter++){

			switch (attr_table.get_attr_type(at_iter)){

				case libdap::Attr_container:    // If it's a container attribute
				{
					libdap::AttrTable *atbl = attr_table.get_attr_table(at_iter);

					if(at_iter != begin )
						*strm << "," << endl;

					// Open a JSON property with the name of the Attribute Table and
					*strm << child_indent  << "\"" << atbl->get_name() << "\": {" << endl;

					// Process the Attribute Table.
					transform(strm, *atbl, child_indent + _indent_increment);

					// Close JSON property object
					*strm << endl << child_indent  << "}";

					break;

				}
				default: // so it's not an Attribute Table. woot. time to print
				{
					// First?
					if(at_iter != begin)
						*strm << "," << endl;

					// Name of property
					*strm << child_indent << "\""<< attr_table.get_name(at_iter) << "\": ";

					// Open values array
					*strm  << "[";
					// Process value(s)
					vector<string> *values = attr_table.get_attr_vector(at_iter);
					for(int i=0; i<values->size() ;i++){
						if(i>0)
							*strm << ",";
						if(attr_table.get_attr_type(at_iter) == libdap::Attr_string || attr_table.get_attr_type(at_iter) == libdap::Attr_url){
							*strm << "\"";

							string value = (*values)[i] ;
							*strm << backslash_escape(value, '"') ;
							*strm << "\"";
						}
						else {
							*strm << (*values)[i] ;
						}

					}
					// Close values array
					*strm << "]";
					break;
				}

			}
		}

	}




}






