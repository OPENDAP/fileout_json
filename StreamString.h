/*
 * StreamString.h
 *
 *  Created on: Jul 3, 2014
 *      Author: ndp
 */

#ifndef STREAMSTRING_H_
#define STREAMSTRING_H_

#include <Str.h>

namespace libdap {

class StreamString: public Str{

public:

	StreamString(const string &n): Str(n) {};
	StreamString(const string &n, const string &d): Str(n,d) {}

    virtual ~StreamString()
    {}

    StreamString(const Str &copy_from): Str(copy_from){}


	friend ostream& operator<<(ostream& out, const Str& s) // output
	{
		out <<  s.value();

		return out;
	}

	friend istream& operator>>(istream& in, Str& s) // input
	{
		string tmp;
		in >> tmp;
		s.set_value(tmp);

		return in;
	}

};

} /* namespace libdap */

#endif /* STREAMSTRING_H_ */
