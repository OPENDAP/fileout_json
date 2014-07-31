// -*- mode: c++; c-basic-offset:4 -*-
//
// FoJsonTransform.h
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


#ifndef FoJsonTransfrom_h_
#define FoJsonTransfrom_h_ 1

#include <string>
#include <vector>
#include <map>

//using std::string ;
//using std::vector ;
//using std::map ;

#include <DDS.h>
#include <Array.h>

//using namespace::libdap ;

#include <BESObj.h>
#include <BESDataHandlerInterface.h>


/**
 * @brief Transforms a DDS into JSON document on disk.
 *
 * Used to transform a DDS into an instance object representation (meta)data JSON document.
 * The output is written to a local file whose name is passed as a parameter
 * to the constructor.
 */
class FoJsonTransform: public BESObj {
private:
	libdap::DDS *_dds;
	string _localfile;
	string _returnAs;
	string _indent_increment;

	template<typename T> unsigned  int json_simple_type_array_worker(std::ostream *strm, T *values, unsigned int indx, std::vector<unsigned int> *shape, unsigned int currentDim);
	template<typename T>void json_simple_type_array(ostream *strm, libdap::Array *a, string indent, bool sendData);

	void transformAtomic(std::ostream *strm, libdap::BaseType *bt, string indent, bool sendData);

	void transform(std::ostream *strm, libdap::DDS *dds, string indent, bool sendData);
	void transform(std::ostream *strm, libdap::BaseType *bt, string indent, bool sendData);
    void transform(std::ostream *strm, libdap::Structure *s,string indent, bool sendData );
    void transform(std::ostream *strm, libdap::Grid *g, string indent, bool sendData);
    void transform(std::ostream *strm, libdap::Sequence *s, string indent, bool sendData);
    void transform(std::ostream *strm, libdap::Array *a, string indent, bool sendData);
    void transform(std::ostream *strm, libdap::AttrTable &attr_table, string  indent);

public:
	FoJsonTransform(libdap::DDS *dds, BESDataHandlerInterface &dhi, const string &localfile);
	virtual ~FoJsonTransform();

	virtual void transform(bool sendData);

	virtual void dump(std::ostream &strm) const;
};

#endif // FoJsonTransfrom_h_

