// -*- mode: c++; c-basic-offset:4 -*-
//
// FoW10nJsonTransmitter.cc
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

#include "FoW10nJsonTransmitter.h"
#include "FoJsonTransform.h"

#include "FoW10nJsonTransform.h"

using namespace ::libdap;

#define FO_JSON_TEMP_DIR "/tmp"

string FoW10nJsonTransmitter::temp_dir;

/** @brief Construct the FoW10nJsonTransmitter
 *
 *
 * The transmitter is created to add the ability to return OPeNDAP data
 * objects (DataDDS) as abstract object representation JSON documents.
 *
 * The OPeNDAP data object is written to a JSON file locally in a
 * temporary directory specified by the BES configuration parameter
 * FoJson.Tempdir. If this variable is not found or is not set then it
 * defaults to the macro definition FO_JSON_TEMP_DIR.
 */
FoW10nJsonTransmitter::FoW10nJsonTransmitter() :
		BESBasicTransmitter()
{
    add_method(DATA_SERVICE, FoW10nJsonTransmitter::send_data);
    add_method(DDX_SERVICE,  FoW10nJsonTransmitter::send_metadata);

    if (FoW10nJsonTransmitter::temp_dir.empty()) {
        // Where is the temp directory for creating these files
        bool found = false;
        string key = "FoJson.Tempdir";
        TheBESKeys::TheKeys()->get_value(key, FoW10nJsonTransmitter::temp_dir, found);
        if (!found || FoW10nJsonTransmitter::temp_dir.empty()) {
            FoW10nJsonTransmitter::temp_dir = FO_JSON_TEMP_DIR;
        }
        string::size_type len = FoW10nJsonTransmitter::temp_dir.length();
        if (FoW10nJsonTransmitter::temp_dir[len - 1] == '/') {
            FoW10nJsonTransmitter::temp_dir = FoW10nJsonTransmitter::temp_dir.substr(0, len - 1);
        }
    }
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
void FoW10nJsonTransmitter::send_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(obj);
    if (!bdds) {
        throw BESInternalError("cast error", __FILE__, __LINE__);
    }

    DataDDS *dds = bdds->get_dds();
    if (!dds) {
        string err = (string) "No DataDDS has been created for transmit";
        BESInternalError pe(err, __FILE__, __LINE__);
        throw pe;
    }

    BESDEBUG("fojson", "FoW10nJsonTransmitter::send_data - parsing the constraint" << endl);



    ConstraintEvaluator &eval = bdds->get_ce();

    send_json(dds, eval, dhi, true);
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
void FoW10nJsonTransmitter::send_metadata(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *>(obj);
    if (!bdds) {
        throw BESInternalError("Cast to BESDDSResponse error.", __FILE__, __LINE__);
    }

    DDS *dds = bdds->get_dds();
    if (!dds) {
        string err = (string) "No DataDDS has been created for transmit";
        BESDEBUG("fojson", "send_metadata() - ERROR: "<< err << endl);
        BESInternalError pe(err, __FILE__, __LINE__);
        throw pe;
    }

    BESDEBUG("fojson", "FoW10nJsonTransmitter::send_metadata - parsing the constraint" << endl);

    ConstraintEvaluator &eval = bdds->get_ce();

    send_json(dds, eval, dhi, false);
}

/** @brief The send_json() method transmits both data and metadata responses.
 *
 * @note Used for unit tests.
 * @param dds The DDS to transform into JSON
 * @param ce the constraint evaluator to use to mark the variables.
 * @param dhi The data interface containing information about the current
 * request.
 * @param sendData If the sendData parameter is true data will be
 * sent. If sendData is false then the metadata will be sent.
 */
void FoW10nJsonTransmitter::send_json(DDS *dds, ConstraintEvaluator &eval, BESDataHandlerInterface &dhi, bool sendData)
{
    string ncVersion = dhi.data[RETURN_CMD] ;

    ostream &o_strm = dhi.get_output_stream();
    if (!o_strm) {
        string err = (string) "Output stream is not set, can not return as JSON";
        BESInternalError pe(err, __FILE__, __LINE__);
        throw pe;
    }

    // ticket 1248 jhrg 2/23/09
    string ce = www2id(dhi.data[POST_CONSTRAINT], "%", "%20%26");
    try {
        eval.parse_constraint(ce, *dds);
    }
    catch (Error &e) {
        string em = e.get_error_message();
        string err = "Failed to parse the constraint expression: " + em;
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    catch (...) {
        string err = (string) "Failed to parse the constraint expression: " + "Unknown exception caught";
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    // The dataset_name is no longer used in the constraint evaluator, so no
    // need to get here. Plus, just getting the first containers dataset
    // name would not have worked with multiple containers.
    // pwest Jan 4, 2009
    string dataset_name = "";

    // now we need to read the data
    BESDEBUG("fojson", "FoW10nJsonTransmitter::send_data - reading data into DataDDS" << endl);

    // I removed the functional_constraint bool and the (dead) code that used it.
    // This kind of temporary object should use auto_ptr<>, but in this case it
    // seems like it's not a supported feature of the handler. 12.27.2011 jhrg

#define FUNCTIONAL_CE_SUPPORTED 0
#if FUNCTIONAL_CE_SUPPORTED
    // This is used to record whether this is a functional CE or not. If so,
    // the code allocates a new DDS object to hold the BaseType returned by
    // the function and we need to delete that DDS before exiting this code.
    bool functional_constraint = false;
#endif
    try {
        // Handle *functional* constraint expressions specially
        if (eval.function_clauses()) {
            BESDEBUG("fojson", "processing a functional constraint clause(s)." << endl);
            DDS *tmp_dds = eval.eval_function_clauses(*dds);
            delete dds;
            dds = tmp_dds;
        }
#if FUNCTIONAL_CE_SUPPORTED
        if( eval.functional_expression() )
        {
            // This returns a new BaseType, not a pointer to one in the DataDDS
            // So once the data has been read using this var create a new
            // DataDDS and add this new var to the it.
            BaseType *var = eval).eval_function( *dds, dataset_name );
            if (!var)
            throw Error(unknown_error, "Error calling the CE function.");

            var->read( );

            dds = new DataDDS( NULL, "virtual" );
            // Set 'functional_constraint' here so that below we know that if
            // it's true we must delete 'dds'.
            functional_constraint = true;
            dds->add_var( var );
        }
#endif
        else
        {
            // Iterate through the variables in the DataDDS and read
            // in the data if the variable has the send flag set.

            // Note the special case for Sequence. The
            // transfer_data() method uses the same logic as
            // serialize() to read values but transfers them to the
            // d_values field instead of writing them to a XDR sink
            // pointer. jhrg 9/13/06
            for (DDS::Vars_iter i = dds->var_begin(); i != dds->var_end(); i++) {
                if ((*i)->send_p()) {
                    // FIXME: we don't have sequences in netcdf so let's not
                    // worry about that right now.
                    (*i)->intern_data(eval, *dds);
                }

            }
        }
    }
    catch (Error &e) {
#if FUNCTIONAL_CE_SUPPORTED
        if (functional_constraint)
        delete dds;
#endif
        string em = e.get_error_message();
        string err = "Failed to read data: " + em;
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    catch (...) {
#if FUNCTIONAL_CE_SUPPORTED
        if (functional_constraint)
        delete dds;
#endif
        string err = "Failed to read data: Unknown exception caught";
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    string temp_file_name = FoW10nJsonTransmitter::temp_dir + '/' + "jsonXXXXXX";
    char *temp_full = new char[temp_file_name.length() + 1];
    string::size_type len = temp_file_name.copy(temp_full, temp_file_name.length());
    *(temp_full + len) = '\0';
    // cover the case where older versions of mkstemp() create the file using
    // a mode of 666.
    mode_t original_mode = umask(077);
    int fd = mkstemp(temp_full);
    umask(original_mode);


    if (fd == -1) {
        delete[] temp_full;
#if FUNCTIONAL_CE_SUPPORTED
        if (functional_constraint)
        delete dds;
#endif
        string err = string("Failed to open the temporary file: ") + temp_file_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    // transform the OPeNDAP DataDDS to the netcdf file
    BESDEBUG("fojson", "FoW10nJsonTransmitter::send_data - transforming into temporary file " << temp_full << endl);

    try {



//        FoJsonTransform ft(dds, dhi, temp_full);
        BESDEBUG("fojson", "FoW10nJsonTransmitter::send_data - Building JSON Transformer. " << endl);
        FoW10nJsonTransform ft(dds, dhi, temp_full);


        BESDEBUG("fojson", "FoW10nJsonTransmitter::send_data - Transforming. sendData: "<< sendData << endl);

        ft.transform( sendData );

        BESDEBUG("fojson", "FoW10nJsonTransmitter::send_data - transmitting temp file " << temp_full << endl);
        FoW10nJsonTransmitter::return_temp_stream(temp_full, o_strm, ncVersion);

    }
    catch (BESError &e) {
        close(fd);
        (void) unlink(temp_full);
        delete[] temp_full;
#if FUNCTIONAL_CE_SUPPORTED
        if (functional_constraint)
        delete dds;
#endif
        throw;
    }
    catch (...) {
        close(fd);
        (void) unlink(temp_full);
        delete[] temp_full;
#if FUNCTIONAL_CE_SUPPORTED
        if (functional_constraint)
        delete dds;
#endif
        string err = (string) "fileout_json: Failed to transform to JSON, unknown error";
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    close(fd);
    (void) unlink(temp_full);
    delete[] temp_full;
#if FUNCTIONAL_CE_SUPPORTED
    if (functional_constraint)
    delete dds;
#endif
    BESDEBUG("fojson", "FoW10nJsonTransmitter::send_data - done transmitting JSON" << endl);
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
void FoW10nJsonTransmitter::return_temp_stream(const string &filename,
					 ostream &strm,
					 const string &ncVersion)
{
    //  int bytes = 0 ;    // Not used; jhrg 3/16/11
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
#if 0
        bool found = false;
        string context = "transmit_protocol";
        string protocol = BESContextManager::TheManager()->get_context(context, found);
        if (protocol == "HTTP") {
            strm << "HTTP/1.0 200 OK\n";
            strm << "Content-type: application/octet-stream\n";
            strm << "Content-Description: " << "BES dataset" << "\n";
            if ( ncVersion == RETURNAS_NETCDF4 ) {
            	strm << "Content-Disposition: filename=" << filename << ".nc4;\n\n";
            }
            else {
            	strm << "Content-Disposition: filename=" << filename << ".nc;\n\n";
            }
            strm << flush;
        }
#endif

        strm.write(block, nbytes);
        //bytes += nbytes ;
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
        //write( fileno( stdout ),(void*)block, nbytes ) ;
        //bytes += nbytes ;
    }
    os.close();
}

