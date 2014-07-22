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

using std::string ;
using std::vector ;
using std::map ;

#include <DDS.h>
#include <Array.h>

using namespace::libdap ;

#include <BESObj.h>
#include <BESDataHandlerInterface.h>


/** @brief Transformation object that converts an OPeNDAP DataDDS to a
 * netcdf file
 *
 * This class transforms each variable of the DataDDS to a netcdf file. For
 * more information on the transformation please refer to the OpeNDAP
 * documents wiki.
 */
class FoJsonTransform: public BESObj {
private:
	DDS *_dds;
	string _localfile;
	string _returnAs;
	string _indent_increment;

	void transformAtomic(ostream *strm, BaseType *bt, string indent, bool sendData);

	void transform(ostream *strm, DDS *dds, string indent, bool sendData);
	void transform(ostream *strm, BaseType *bt, string indent, bool sendData);
    void transform(ostream *strm, Structure *s,string indent, bool sendData );
    void transform(ostream *strm, Grid *g, string indent, bool sendData);
    void transform(ostream *strm, Sequence *s, string indent, bool sendData);

    void transform(ostream *strm, Array *a, string indent, bool sendData);

    void transform(ostream *strm, AttrTable &attr_table, string  indent);

public:
	/**
	 * Build a FoJsonTransform object. By default it builds a netcdf 3 file; pass "netcdf-4"
	 * to get a netcdf 4 file.
	 *
	 * @note added default value to fourth param to preserve the older API. 5/6/13 jhrg
	 * @param dds
	 * @param dhi
	 * @param localfile
	 * @param netcdfVersion
	 */
	FoJsonTransform(DDS *dds, BESDataHandlerInterface &dhi, const string &localfile);
	virtual ~FoJsonTransform();
	virtual void transform(bool sendData);

	virtual void dump(ostream &strm) const;

};

#endif // FoJsonTransfrom_h_

