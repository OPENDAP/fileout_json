/*
 * FoW10nJsonTransform.h
 *
 *  Created on: Jul 15, 2014
 *      Author: ndp
 */

#ifndef FOW10NJSONTRANSFORM_H_
#define FOW10NJSONTRANSFORM_H_

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


class FoW10nJsonTransform: public BESObj {
private:
	DDS *_dds;
	string _localfile;
	string _returnAs;
	string _indent_increment;


	void writeNodeMetadata(ostream *strm, BaseType *bt, string indent);
	void writeLeafMetadata(ostream *strm, BaseType *bt, string indent);
	void writeDatasetMetadata(ostream *strm, DDS *dds, string indent);

	void transformAtomic(ostream *strm, BaseType *bt, string indent, bool sendData);


	void transform(ostream *strm, DDS *dds, string indent, bool sendData);
	void transform(ostream *strm, BaseType *bt, string indent, bool sendData);

    //void transform(ostream *strm, Structure *s,string indent );
    //void transform(ostream *strm, Grid *g, string indent);
    //void transform(ostream *strm, Sequence *s, string indent);
	void transform(ostream *strm, Constructor *cnstrctr, string indent, bool sendData);
	void transform_node_worker(ostream *strm, vector<BaseType *> leaves, vector<BaseType *> nodes, string indent, bool sendData);


    void transform(ostream *strm, Array *a, string indent, bool sendData);
    void transform(ostream *strm, AttrTable &attr_table, string  indent);

    template<typename T>
    void json_simple_type_array(ostream *strm, Array *a, string indent, bool sendData);

    template<typename T>
    unsigned  int json_simple_type_array_worker(
    		ostream *strm,
    		T *values,
    		unsigned int indx,
    		vector<unsigned int> *shape,
    		unsigned int currentDim
    		);



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
    FoW10nJsonTransform(DDS *dds, BESDataHandlerInterface &dhi, const string &localfile);
	virtual ~FoW10nJsonTransform();
	virtual void transform(bool sendData);

	virtual void dump(ostream &strm) const;

};

#endif /* FOW10NJSONTRANSFORM_H_ */
