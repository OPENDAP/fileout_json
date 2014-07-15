// FoJsonTransform.h

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

class FoJsonBaseType ;

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

	void transformAtomic(ostream *strm, BaseType *bt, string indent);

	void transform(ostream *strm, DDS *dds, string indent);
	void transform(ostream *strm, BaseType *bt, string indent);
    void transform(ostream *strm, Structure *s,string indent );
    void transform(ostream *strm, Grid *g, string indent);
    void transform(ostream *strm, Sequence *s, string indent);

    void transform(ostream *strm, Array *a, string indent);
	void transform(ostream *strm, AttrTable *at, string indent);

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
	virtual void transform();

	virtual void dump(ostream &strm) const;

};

#endif // FoJsonTransfrom_h_

