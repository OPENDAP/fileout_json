/*
 * FoW10nJsonTransform.cc
 *
 *  Created on: Jul 15, 2014
 *      Author: ndp
 */

#include "FoW10nJsonTransform.h"
#include "config.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <stddef.h>
#include <string>

using std::ostringstream;
using std::istringstream;

#include "FoW10nJsonTransform.h"

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




#define FoW10nJsonTransform_debug_key "fojson"

/**
 * Replace every occurrence of 'char_to_escape' with the same preceded
 * by the backslash '\' character.
 */
string backslash_escape(string source, char char_to_escape){
	string escaped_result = source;
	if(source.find(char_to_escape) >= 0 ){
		size_t found = 0;
		for(size_t i=0; i< source.length() ; i++){
			if(source[i] == char_to_escape){
				escaped_result.insert( i + found++, "\\");
			}
		}
	}
	return escaped_result;
}




long computeConstrainedShape(libdap::Array *a, vector<unsigned int> *shape );
string getW10nTypeString(BaseType *bt){
	switch(bt->type()){
	// Handle the atomic types - that's easy!
	case dods_byte_c:
		return "b";
		break;

	case dods_int16_c:
		return "i";
		break;

	case dods_uint16_c:
		return "ui";
		break;

	case dods_int32_c:
		return "i";
		break;

	case dods_uint32_c:
		return "ui";
		break;

	case dods_float32_c:
		return "f";
		break;

	case dods_float64_c:
		return "f";
		break;

	case dods_str_c:
		return "s";
		break;

	case dods_url_c:
		return "s";
		break;

	case dods_structure_c:
	case dods_grid_c:
	case dods_sequence_c:
	case dods_array_c:
	{
		string s = (string) "File out JSON, W10N supports complex data types as nodes. "
				"The variable " + bt->type_name() +" " +bt->name() +" is a node type.";
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

/**
 *  @TODO Handle String and URL Arrays including backslash escaping double quotes in values.
 *
 */
template<typename T> unsigned  int FoW10nJsonTransform::json_simple_type_array_worker(ostream *strm, T *values, unsigned int indx, vector<unsigned int> *shape, unsigned int currentDim){

	*strm << "[";

	unsigned int currentDimSize = (*shape)[currentDim];

    for(int i=0; i<currentDimSize ;i++){
		if(currentDim < shape->size()-1){
			BESDEBUG(FoW10nJsonTransform_debug_key, "json_simple_type_array_worker() - Recursing! indx:  " << indx
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




template<typename T>void FoW10nJsonTransform::json_simple_type_array(ostream *strm, Array *a, string indent, bool sendData){


	*strm << indent << "{" << endl;\

	string childindent = indent + _indent_increment;

	writeLeafMetadata(strm,a,childindent);

	int numDim = a->dimensions(true);
	vector<unsigned int> shape(numDim);
	long length = computeConstrainedShape(a, &shape);

	*strm << childindent << "\"shape\": [";

	for(int i=0; i<shape.size() ;i++){
		if(i>0)
			*strm << ",";
		*strm << shape[i];
	}
	*strm << "]";

	if(sendData){
		*strm << ","<< endl;

		// Data
		*strm << childindent << "\"data\": ";

	    T *src = new T[length];
	    a->value(src);

	    unsigned int indx = json_simple_type_array_worker(strm, src, 0, &shape, 0);

	    delete src;

	    if(length != indx)
			BESDEBUG(FoW10nJsonTransform_debug_key, "json_simple_type_array() - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);


	}

	*strm << endl << indent << "}";

}




void FoW10nJsonTransform::writeDatasetMetadata(ostream *strm, DDS *dds, string indent){

	// Name
	*strm << indent << "\"name\": \""<< dds->get_dataset_name() << "\"," << endl;

	//Attributes
	transform(strm, dds->get_attr_table(), indent);
	*strm << "," << endl;

}


void FoW10nJsonTransform::writeNodeMetadata(ostream *strm, BaseType *bt, string indent){

	// Name
	*strm << indent << "\"name\": \""<< bt->name() << "\"," << endl;

	//Attributes
	transform(strm, bt->get_attr_table(), indent);
	*strm << "," << endl;



}


void FoW10nJsonTransform::writeLeafMetadata(ostream *strm, BaseType *bt, string indent){

	// Name
	*strm << indent << "\"name\": \""<< bt->name() << "\"," << endl;



	if(bt->type() == dods_array_c){
		Array *a = (Array *)bt;
		*strm << indent << "\"type\": \""<< getW10nTypeString(a->var()) << "\"," << endl;
	}
	else {
		*strm << indent << "\"type\": \""<< getW10nTypeString(bt) << "\"," << endl;
	}


	//Attributes
	transform(strm, bt->get_attr_table(), indent);
	*strm << "," << endl;



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
FoW10nJsonTransform::FoW10nJsonTransform(DDS *dds, BESDataHandlerInterface &dhi, const string &localfile) :
        _dds(0),
        _indent_increment("  ")
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

}

/** @brief Destructor
 *
 * Cleans up any temporary data created during the transformation
 */
FoW10nJsonTransform::~FoW10nJsonTransform()
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
void FoW10nJsonTransform::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FoW10nJsonTransform::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "temporary file = " << _localfile << endl;
    if(_dds != 0){
        _dds->print(strm);
    }
    BESIndent::UnIndent();
}




/** @brief Transforms each of the variables of the DataDDS to the NetCDF
 * file
 *
 * For each variable in the DataDDS write out that variable and its
 * attributes to the netcdf file. Each OPeNDAP data type translates into a
 * particular netcdf type. Also write out any global variables stored at the
 * top level of the DataDDS.
 */
void FoW10nJsonTransform::transform(bool sendData)
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


/**
 * DAP Constructor types are semantically equivalent to a w10n node type so they
 * must be represented as a collection of child nodes and leaves.
 */
void FoW10nJsonTransform::transform(ostream *strm, Constructor *cnstrctr, string indent, bool sendData){
	vector<BaseType *> leaves;
	vector<BaseType *> nodes;


	// Sort the variables into two sets/
	DDS::Vars_iter vi = cnstrctr->var_begin();
	DDS::Vars_iter ve = cnstrctr->var_end();
	for (; vi != ve; vi++) {
		if ((*vi)->send_p()) {
			BaseType *v = *vi;
			v->is_constructor_type();
			Type type = v->type();
			if(type == dods_array_c){
				type = v->var()->type();
			}
			if(v->is_constructor_type() ||
					(v->is_vector_type() && v->var()->is_constructor_type())){
				nodes.push_back(v);
			}
			else {
				leaves.push_back(v);
			}
		}
	}

	// Declare this node
	*strm << indent << "{" << endl ;
	string child_indent = indent + _indent_increment;

	// Write this node's metadata (name & attributes)
	writeNodeMetadata(strm, cnstrctr, child_indent);

	transform_node_worker(strm, leaves,  nodes, child_indent, sendData);

	*strm << indent << "}" << endl;

}

void FoW10nJsonTransform::transform_node_worker(ostream *strm, vector<BaseType *> leaves, vector<BaseType *> nodes, string indent, bool sendData){
	// Write down this nodes leaves
	*strm << indent << "\"leaves\": [";
	if(leaves.size() > 0)
		*strm << endl;
	for(int l=0; l< leaves.size(); l++){
		BaseType *v = leaves[l];
		BESDEBUG(FoW10nJsonTransform_debug_key, "Processing LEAF: " << v->name() << endl);
		if( l>0 ){
			*strm << "," ;
			*strm << endl ;
		}
		transform(strm, v, indent + _indent_increment, sendData);
	}
	if(leaves.size()>0)
		*strm << endl << indent;
	*strm << "]," << endl;


	// Write down this nodes child nodes
	*strm << indent << "\"nodes\": [";
	if(nodes.size() > 0)
		*strm << endl;
	for(int n=0; n< nodes.size(); n++){
		BaseType *v = nodes[n];
		transform(strm, v, indent + _indent_increment, sendData);
	}
	if(nodes.size()>0)
		*strm << endl << indent;

	*strm << "]" << endl;


}



void FoW10nJsonTransform::transform(ostream *strm, DDS *dds, string indent, bool sendData){



	vector<BaseType *> leaves;
	vector<BaseType *> nodes;

	DDS::Vars_iter vi = dds->var_begin();
	DDS::Vars_iter ve = dds->var_end();
	for (; vi != ve; vi++) {
		if ((*vi)->send_p()) {
			BaseType *v = *vi;
			Type type = v->type();
			if(type == dods_array_c){
				type = v->var()->type();
			}
			if(v->is_constructor_type() ||
					(v->is_vector_type() && v->var()->is_constructor_type())){
				nodes.push_back(v);
			}
			else {
				leaves.push_back(v);
			}
		}
	}

	// Declare this node
	*strm << indent << "{" << endl ;
	string child_indent = indent + _indent_increment;

	// Write this node's metadata (name & attributes)
	writeDatasetMetadata(strm, dds, child_indent);

	transform_node_worker(strm, leaves,  nodes, child_indent, sendData);

	*strm << indent << "}" << endl;

}



void FoW10nJsonTransform::transform(ostream *strm, BaseType *bt, string  indent, bool sendData)
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
	case dods_url_c:
		transformAtomic(strm, bt, indent, sendData);
		break;

	case dods_structure_c:
		transform(strm, (Structure *) bt, indent, sendData);
		break;

	case dods_grid_c:
		transform(strm, (Grid *) bt, indent, sendData);
		break;

	case dods_sequence_c:
		transform(strm, (Sequence *) bt, indent, sendData);
		break;

	case dods_array_c:
		transform(strm, (Array *) bt, indent, sendData);
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

void FoW10nJsonTransform::transformAtomic(ostream *strm, BaseType *b, string indent, bool sendData){

	*strm << indent << "{" << endl;

	string childindent = indent + _indent_increment;

	writeLeafMetadata(strm, b, childindent);

	*strm << childindent << "\"shape\": [1]," << endl;

	if(sendData){
		// Data
		*strm << childindent << "\"data\": [";

		if(b->type() == dods_str_c || b->type() == dods_url_c ){
			std::stringstream ss;
			b->print_val(ss,"",false);
			*strm << "\"" << backslash_escape(ss.str(), '"') << "\"";
		}
		else {
			b->print_val(*strm, "", false);
		}


		*strm << "]";
	}

}



void FoW10nJsonTransform::transform(ostream *strm, Array *a, string indent, bool sendData){

    BESDEBUG(FoW10nJsonTransform_debug_key, "FoJsonTransform::transform() - Processing Array. "
            << " a->type(): " << a->type()
			<< " a->var()->type(): " << a->var()->type()
			<< endl);

	switch(a->var()->type()){
	// Handle the atomic types - that's easy!
	case dods_byte_c:
		json_simple_type_array<dods_byte>(strm,a,indent,sendData);
		break;

	case dods_int16_c:
		json_simple_type_array<dods_int16>(strm,a,indent,sendData);
		break;

	case dods_uint16_c:
		json_simple_type_array<dods_uint16>(strm,a,indent,sendData);
		break;

	case dods_int32_c:
		json_simple_type_array<dods_int32>(strm,a,indent,sendData);
		break;

	case dods_uint32_c:
		json_simple_type_array<dods_uint32>(strm,a,indent,sendData);
		break;

	case dods_float32_c:
		json_simple_type_array<dods_float32>(strm,a,indent,sendData);
    	break;

	case dods_float64_c:
		json_simple_type_array<dods_float64>(strm,a,indent,sendData);
		break;

	case dods_str_c:
	{
		// @TODO Handle String and URL Arrays including backslash escaping double quotes in values.
		//json_simple_type_array<string>(strm,a,indent,sendData);
		//break;

		string s = (string) "File out JSON, " + "Arrays of String objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	case dods_url_c:
	{
		// @TODO Handle String and URL Arrays including backslash escaping double quotes in values.
		//json_simple_type_array<string>(strm,a,indent,sendData);
		//break;

		string s = (string) "File out JSON, " + "Arrays of URL objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

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


void FoW10nJsonTransform::transform(ostream *strm, AttrTable &attr_table, string  indent){

	string child_indent = indent + _indent_increment;

	*strm << indent << "\"attributes\": [";


//	if(attr_table.get_name().length()>0)
//		*strm  << endl << child_indent << "{\"name\": \"name\", \"value\": \"" << attr_table.get_name() << "\"},";


	if(attr_table.get_size() != 0) {
		*strm << endl;
		AttrTable::Attr_iter begin = attr_table.attr_begin();
		AttrTable::Attr_iter end = attr_table.attr_end();


		for(AttrTable::Attr_iter at_iter=begin; at_iter !=end; at_iter++){

			switch (attr_table.get_attr_type(at_iter)){
				case Attr_container:
				{
					AttrTable *atbl = attr_table.get_attr_table(at_iter);

					if(at_iter != begin )
						*strm << "," << endl;

					*strm << child_indent << "{" << endl;

					if(atbl->get_name().length()>0)
						*strm << child_indent + _indent_increment << "\"name\": \"" << atbl->get_name() << "\"," << endl;


					transform(strm, *atbl, child_indent + _indent_increment);
					*strm << endl << child_indent << "}";

					break;

				}
				default:
				{
					if(at_iter != begin)
						*strm << "," << endl;

					*strm << child_indent << "{\"name\": \""<< attr_table.get_name(at_iter) << "\", ";
					*strm  << "\"value\": [";
					vector<string> *values = attr_table.get_attr_vector(at_iter);
					for(int i=0; i<values->size() ;i++){
						if(i>0)
							*strm << ",";
						if(attr_table.get_attr_type(at_iter) == Attr_string || attr_table.get_attr_type(at_iter) == Attr_url){
							*strm << "\"";

							string value = (*values)[i] ;
							*strm << backslash_escape(value, '"') ;
							*strm << "\"";
						}
						else {
							*strm << (*values)[i] ;
						}

					}
					*strm << "]}";
					break;
				}

			}
		}
		*strm << endl << indent;

	}

	*strm << "]";



}








