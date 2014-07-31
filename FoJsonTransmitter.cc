// -*- mode: c++; c-basic-offset:4 -*-
//
// FoJsonTransmitter.cc
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

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>                  // For umask
#include <sys/stat.h>

#include <iostream>
#include <fstream>

#include <DataDDS.h>
#include <BaseType.h>
#include <escaping.h>
#include <ConstraintEvaluator.h>

#include <BESInternalError.h>
#include <TheBESKeys.h>
#include <BESContextManager.h>
#include <BESDataDDSResponse.h>
#include <BESDDSResponse.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDebug.h>

#include "FoJsonTransmitter.h"
#include "FoJsonTransform.h"

#include "FoW10nJsonTransform.h"

using namespace ::libdap;

#define FO_JSON_TEMP_DIR "/tmp"

#define RETURNAS_JSON "json"

string FoJsonTransmitter::temp_dir;

/** @brief Construct the FoJsonTransmitter.
 *
 * The transmitter is created to add the ability to return OPeNDAP data
 * objects (DataDDS) as instance object representation JSON documents.
 *
 * The OPeNDAP data object is written to a JSON file locally in a
 * temporary directory specified by the BES configuration parameter
 * FoJson.Tempdir. If this variable is not found or is not set then it
 * defaults to the macro definition FO_JSON_TEMP_DIR.
 */
FoJsonTransmitter::FoJsonTransmitter() :
		BESBasicTransmitter()
{
    add_method(DATA_SERVICE, FoJsonTransmitter::send_data);
    add_method(DDX_SERVICE,  FoJsonTransmitter::send_metadata);

    if (FoJsonTransmitter::temp_dir.empty()) {
        // Where is the temp directory for creating these files
        bool found = false;
        string key = "FoJson.Tempdir";
        TheBESKeys::TheKeys()->get_value(key, FoJsonTransmitter::temp_dir, found);
        if (!found || FoJsonTransmitter::temp_dir.empty()) {
            FoJsonTransmitter::temp_dir = FO_JSON_TEMP_DIR;
        }
        string::size_type len = FoJsonTransmitter::temp_dir.length();
        if (FoJsonTransmitter::temp_dir[len - 1] == '/') {
            FoJsonTransmitter::temp_dir = FoJsonTransmitter::temp_dir.substr(0, len - 1);
        }
    }
}

#if 0
/** @brief The static method registered to transmit OPeNDAP data objects as
 * a JSON file.
 *
 * This function takes the OPeNDAP DataDDS object, reads in the data (can be
 * used with any data handler), transforms the data into a JSON file, and
 * streams back that JSON file back to the requester using the stream
 * specified in the BESDataHandlerInterface.
 *
 * @param obj The BESResponseObject containing the OPeNDAP DataDDS object
 * @param dhi BESDataHandlerInterface containing information about the
 * request and response
 * @throws BESInternalError if the response is not an OPeNDAP DataDDS or if
 * there are any problems reading the data, writing to a JSON file, or
 * streaming the JSON file
 */
void FoJsonTransmitter::send_metadata(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *>(obj);
    if (!bdds) {
        throw BESInternalError("Cast to BESDDSResponse error.", __FILE__, __LINE__);
    }

    DDS *dds = bdds->get_dds();
    if (!dds)
        throw BESInternalError("No DataDDS has been created for transmit", __FILE__, __LINE__);

    BESDEBUG("fojson", "FoJsonTransmitter::send_metadata - parsing the constraint" << endl);

    ConstraintEvaluator &eval = bdds->get_ce();

    send_json(dds, eval, dhi, false);
}
#endif

/** @brief The static method registered to transmit OPeNDAP data objects as
 * a JSON file.
 *
 * This function takes the OPeNDAP DataDDS object, reads in the data (can be
 * used with any data handler), transforms the data into a JSON file, and
 * streams back that JSON file back to the requester using the stream
 * specified in the BESDataHandlerInterface.
 *
 * @param obj The BESResponseObject containing the OPeNDAP DataDDS object
 * @param dhi BESDataHandlerInterface containing information about the
 * request and response
 * @throws BESInternalError if the response is not an OPeNDAP DataDDS or if
 * there are any problems reading the data, writing to a JSON file, or
 * streaming the JSON file
 */
void FoJsonTransmitter::send_metadata(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *>(obj);
    if (!bdds) {
        throw BESInternalError("Cast to BESDDSResponse error.", __FILE__, __LINE__);
    }

    DDS *dds = bdds->get_dds();
    if (!dds)
        throw BESInternalError("No DataDDS has been created for transmit", __FILE__, __LINE__);

    BESDEBUG("fojson", "FoJsonTransmitter::send_metadata - parsing the constraint" << endl);

    ConstraintEvaluator &eval = bdds->get_ce();
    ostream &o_strm = dhi.get_output_stream();
    if (!o_strm)
        throw BESInternalError("Output stream is not set, can not return as JSON", __FILE__, __LINE__);

    // ticket 1248 jhrg 2/23/09
    string ce = www2id(dhi.data[POST_CONSTRAINT], "%", "%20%26");
    try {
        eval.parse_constraint(ce, *dds);
    }
    catch (Error &e) {
        throw BESInternalError("Failed to parse the constraint expression: " + e.get_error_message(), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalError("Failed to parse the constraint expression: Unknown exception caught", __FILE__, __LINE__);
    }

    // now we need to read the data
    BESDEBUG("fojson", "FoJsonTransmitter::send_data - reading data into DataDDS" << endl);

    try {
        // Handle *functional* constraint expressions specially
        if (eval.function_clauses()) {
            BESDEBUG("fojson", "processing a functional constraint clause(s)." << endl);
            DDS *tmp_dds = eval.eval_function_clauses(*dds);
            bdds->set_dds(tmp_dds);
            delete dds;
            dds = tmp_dds;
        }
        else {
            // Iterate through the variables in the DataDDS and read
            // in the data if the variable has the send flag set.

            for (DDS::Vars_iter i = dds->var_begin(); i != dds->var_end(); i++) {
                if ((*i)->send_p()) {
                    (*i)->intern_data(eval, *dds);
                }

            }
        }
    }
    catch (Error &e) {
        throw BESInternalError("Failed to read data: " + e.get_error_message(), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalError("Failed to read data: Unknown exception caught", __FILE__, __LINE__);
    }

    string temp_file_name = FoJsonTransmitter::temp_dir + '/' + "jsonXXXXXX";
    vector<char> temp_full(temp_file_name.length() + 1);
    string::size_type len = temp_file_name.copy(&temp_full[0], temp_file_name.length());
    temp_full[len] = '\0';

    // cover the case where older versions of mkstemp() create the file using
    // a mode of 666.
    mode_t original_mode = umask(077);
    int fd = mkstemp(&temp_full[0]);
    umask(original_mode);
    if (fd == -1)
        throw BESInternalError("Failed to open the temporary file: " + temp_file_name, __FILE__, __LINE__);

    // transform the OPeNDAP DataDDS to the netcdf file
    BESDEBUG("fojson", "FoJsonTransmitter::send_data - transforming into temporary file " << &temp_full[0] << endl);

    try {
        FoJsonTransform ft(dds, dhi, &temp_full[0]);

        ft.transform( false /* do not send data */ );

        FoJsonTransmitter::return_temp_stream(&temp_full[0], o_strm);
    }
    catch (BESError &e) {
        close(fd);
        (void) unlink(&temp_full[0]);
        throw;
    }
    catch (...) {
        close(fd);
        (void) unlink(&temp_full[0]);
        throw BESInternalError("fileout_json: Failed to transform to JSON, unknown error", __FILE__, __LINE__);
    }

    close(fd);
    (void) unlink(&temp_full[0]);

    BESDEBUG("fojson", "FoJsonTransmitter::send_data - done transmitting JSON" << endl);
}


    /** @brief The static method registered to transmit OPeNDAP data objects as
 * a JSON file.
 *
 * This function takes the OPeNDAP DataDDS object, reads in the data (can be
 * used with any data handler), transforms the data into a JSON file, and
 * streams back that JSON file back to the requester using the stream
 * specified in the BESDataHandlerInterface.
 *
 * @param obj The BESResponseObject containing the OPeNDAP DataDDS object
 * @param dhi BESDataHandlerInterface containing information about the
 * request and response
 * @throws BESInternalError if the response is not an OPeNDAP DataDDS or if
 * there are any problems reading the data, writing to a JSON file, or
 * streaming the JSON file
 */
void FoJsonTransmitter::send_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(obj);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    DataDDS *dds = bdds->get_dds();
    if (!dds)
        throw BESInternalError("No DataDDS has been created for transmit", __FILE__, __LINE__);

    BESDEBUG("fojson", "FoJsonTransmitter::send_data - parsing the constraint" << endl);

    ConstraintEvaluator &eval = bdds->get_ce();

    ostream &o_strm = dhi.get_output_stream();
    if (!o_strm)
        throw BESInternalError("Output stream is not set, can not return as JSON", __FILE__, __LINE__);

    // ticket 1248 jhrg 2/23/09
    string ce = www2id(dhi.data[POST_CONSTRAINT], "%", "%20%26");
    try {
        eval.parse_constraint(ce, *dds);
    }
    catch (Error &e) {
        throw BESInternalError("Failed to parse the constraint expression: " + e.get_error_message(), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalError("Failed to parse the constraint expression: Unknown exception caught", __FILE__, __LINE__);
    }

    // now we need to read the data
    BESDEBUG("fojson", "FoJsonTransmitter::send_data - reading data into DataDDS" << endl);

    try {
        // Handle *functional* constraint expressions specially
        if (eval.function_clauses()) {
            BESDEBUG("fojson", "processing a functional constraint clause(s)." << endl);
            DataDDS *tmp_dds = eval.eval_function_clauses(*dds);
            bdds->set_dds(tmp_dds);
            delete dds;
            dds = tmp_dds;
        }
        else {
            // Iterate through the variables in the DataDDS and read
            // in the data if the variable has the send flag set.

            for (DDS::Vars_iter i = dds->var_begin(); i != dds->var_end(); i++) {
                if ((*i)->send_p()) {
                    (*i)->intern_data(eval, *dds);
                }

            }
        }
    }
    catch (Error &e) {
        throw BESInternalError("Failed to read data: " + e.get_error_message(), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalError("Failed to read data: Unknown exception caught", __FILE__, __LINE__);
    }

    string temp_file_name = FoJsonTransmitter::temp_dir + '/' + "jsonXXXXXX";
    vector<char> temp_full(temp_file_name.length() + 1);
    string::size_type len = temp_file_name.copy(&temp_full[0], temp_file_name.length());
    temp_full[len] = '\0';

    // cover the case where older versions of mkstemp() create the file using
    // a mode of 666.
    mode_t original_mode = umask(077);
    int fd = mkstemp(&temp_full[0]);
    umask(original_mode);
    if (fd == -1)
        throw BESInternalError("Failed to open the temporary file: " + temp_file_name, __FILE__, __LINE__);

    // transform the OPeNDAP DataDDS to the netcdf file
    BESDEBUG("fojson", "FoJsonTransmitter::send_data - transforming into temporary file " << &temp_full[0] << endl);

    try {
        FoJsonTransform ft(dds, dhi, &temp_full[0]);

        ft.transform( true /* send data */ );

        FoJsonTransmitter::return_temp_stream(&temp_full[0], o_strm);
    }
    catch (BESError &e) {
        close(fd);
        (void) unlink(&temp_full[0]);
        throw;
    }
    catch (...) {
        close(fd);
        (void) unlink(&temp_full[0]);
        throw BESInternalError("fileout_json: Failed to transform to JSON, unknown error", __FILE__, __LINE__);
    }

    close(fd);
    (void) unlink(&temp_full[0]);

    BESDEBUG("fojson", "FoJsonTransmitter::send_data - done transmitting JSON" << endl);
}

/** @brief stream the temporary netcdf file back to the requester
 *
 * Streams the temporary netcdf file specified by filename to the specified
 * C++ ostream
 *
 * @param filename The name of the file to stream back to the requester
 * @param strm C++ ostream to write the contents of the file to
 * @throws BESInternalError if problem opening the file
 */
void FoJsonTransmitter::return_temp_stream(const string &filename, ostream &strm)
{
    ifstream os;
    os.open(filename.c_str(), ios::binary | ios::in);
    if (!os) {
        string err = "Can not connect to file " + filename;
        BESInternalError pe(err, __FILE__, __LINE__);
        throw pe;
    }
    int nbytes;
    char block[4096];

    os.read(block, sizeof block);
    nbytes = os.gcount();
    if (nbytes > 0) {
        strm.write(block, nbytes);
    }
    else {
        // close the stream before we leave.
        os.close();

        string err = (string) "0XAAE234F: failed to stream. Internal server "
                + "error, got zero count on stream buffer." + filename;
        BESInternalError pe(err, __FILE__, __LINE__);
        throw pe;
    }
    while (os) {
        os.read(block, sizeof block);
        nbytes = os.gcount();
        strm.write(block, nbytes);
    }
    os.close();
}

