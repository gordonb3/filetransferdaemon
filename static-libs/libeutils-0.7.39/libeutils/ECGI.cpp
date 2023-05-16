/*

    libeutils - http://www.excito.com/

    ECGI.cpp - this file is part of libeutils.

    Copyright (C) 2007 Tor Krill <tor@excito.com>

    libeutils is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    libeutils is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with libeutils; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/


#include "ECGI.h"
#include "Url.h"
#include "StringTools.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <ctype.h>
#include <algorithm>
#include <errno.h>

extern char **environ;

namespace EUtils {

const string multipart_str = "multipart/form-data;";
const string bound_str = "boundary=";
const string disposition = "Content-Disposition:";
const string formdata = "form-data;";
const string formname = "name=";
const string formfilename = "filename=";

ECGI::ECGI( int fd ) {
    this->part = 0;
    this->buf_fill = 0;
    this->bufsize = 0;
    this->readbuf = NULL;
    this->fd = fd;
    this->content_read = 0;
    this->content_length = 0;
    this->max_read = 0;
    this->upload_directory = "/tmp";
    this->ismulti = false;
    this->state = EMPTY;
    this->last_result = ERROR;
    this->cancel = false;

    this->parseEnv();

    if ( this->env.find( "CONTENT_LENGTH" ) != this->env.end() ) {
        this->content_length = atoll( this->env["CONTENT_LENGTH"].c_str() );
    }

    if ( this->env.find( "CONTENT_TYPE" ) != this->env.end() ) {
        string s = this->env["CONTENT_TYPE"];
        string::size_type pos = s.find( multipart_str );
        if ( pos != string::npos ) {
        	// multipart/form-data, find boundary.

        	this->ismulti = true;
            string::size_type bpos = s.find( bound_str, multipart_str.size() );
            if ( bpos != string::npos ) {
                // Found boundary
                // Prepend --
                this->boundary = string( "--" ) + StringTools::Trimmed( s.substr( bpos + bound_str.size() ), " " );
            } else {
                //Boundary not found
            }
        } else {
            // Asume application/x-www-form-urlencoded
            // Normal post
        }
    }

}

ECGI::~ECGI() {
}

void ECGI::parse( void ) {
    if ( this->content_length < 0 ) {
        cerr << "Content length: " << this->content_length << endl;
    }
    this->bufsize = std::min( this->content_length, static_cast<int64_t>(131072LL) );
    this->readbuf = new unsigned char[this->bufsize];
    this->cur_read = this->readbuf;

    if ( this->ismulti ) {
        this->processMultiPart();
    } else {
        this->processNonMultiPart();
    }

    delete[] this->readbuf;

}

void ECGI::parseCookies( void ) {
    if ( this->env.find( "HTTP_COOKIE" ) != this->env.end() ) {
        string cs = this->env["HTTP_COOKIE"];
        string::size_type pos = string::npos;

        while ( cs.size() > 0 ) {
            string cookie;

            pos = cs.find( ';' );
            if ( pos == string::npos ) {
                // Last value
                cookie = cs;
                cs = "";
            } else {
                cookie = cs.substr( 0, pos );
                cs.replace( 0, pos + 2, "" ); // Remove "read" cookie
            }

            if (( pos = cookie.find( '=' ) ) != string::npos ) {
                this->cookies[URL::UrlDecode( cookie.substr( 0,pos ) )] = URL::UrlDecode( cookie.substr( pos + 1, cookie.size() - pos - 1 ) );
            }
        }
    }
}

void ECGI::parseFields( string s ) {

    string::size_type pos = 0;

    while (( s.size() > 0 ) || ( pos = s.find( '&' ) ) > 0 ) {
        string field;

        if ( pos == string::npos ) {
            // Last value
            field = s;
            s = "";
        } else {
            field = s.substr( 0, pos );
            s.replace( 0, pos + 1, "" );
        }

        if (( pos = field.find( '=' ) ) != string::npos ) {
            this->var[field.substr( 0,pos )] = StringTools::UrlDecode( field.substr( pos + 1, field.size() - pos - 1 ) );
            this->FieldAdded.emit( field.substr( 0, pos ) );
        }
    }
}

void ECGI::advancePointer( ssize_t len ) {
    this->cur_read += len;
    this->buf_fill -= len;
}

unsigned char* ECGI::findBoundary( void ) {
    bool found = false;
    unsigned char* pos = this->cur_read;

    while ( !found && pos < ( this->cur_read + this->buf_fill ) ) {
        // Optimize search somewhat boundary should start with at least 2 "-"

    	if ( pos[0] == '-' && pos[1] == '-') {
            if ( strncmp(( char* )pos, this->boundary.c_str(), this->boundary.length() ) == 0 ) {
                found = true;
            } else {
                pos++;
            }
        } else {
            pos++;
        }

    }
    return found ? pos : NULL;
}

void ECGI::processNonMultiPart( void ) {

    if ( this->env.find( "REQUEST_METHOD" ) != this->env.end() ) {
        string requestmethod = this->env["REQUEST_METHOD"];

        if ( requestmethod == "GET" ) {
            this->parseFields( this->env["QUERY_STRING"] );
        } else if ( requestmethod == "POST" ) {
            if ( this->content_length > 0 ) {
                unsigned char* buffer = new unsigned char[this->content_length] + 1;

                if ( read( this->fd, buffer, this->content_length ) != this->content_length ) {
                    throw new runtime_error( "Failed to read post" );
                }

                buffer[this->content_length] = '\0';

                this->parseFields( string(( char* )buffer ) );

                delete[] buffer;
            }
        }

    }
}

void ECGI::processMultiPart( void ) {

    int stat;

    stat = this->processChunk();

    while ( stat > 0 && !this->cancel ) {
        switch ( stat ) {
        case FULLFIELD:
            this->var[StringTools::ToUpper( this->cur_fieldname )] = StringTools::UrlDecode( string(( const char* )this->cur_read, this->max_read ) );

            this->FieldAdded.emit( StringTools::ToUpper( this->cur_fieldname ) );

            this->advancePointer( this->max_read + 2 ); // +2 -> Skip trailing \r\n
            stat = this->processChunk();
            break;

        case PARTIALFIELD: {
            int tp = this->part;
            string value;

            do {
                value += string(( const char* )this->cur_read, this->max_read );
                this->advancePointer( this->max_read );
                stat = this->processChunk();
            } while ( tp == this->part && !this->cancel );

            this->advancePointer( 2 ); // Skip trailing \r\n
            this->var[StringTools::ToUpper( this->cur_fieldname )] = StringTools::UrlDecode( value );
            this->FieldAdded.emit( StringTools::ToUpper( this->cur_fieldname ) );
            break;
        }
        case FULLFILE: {
            char tmpfilename[100];
            sprintf( tmpfilename, "%s/filXXXXXX", this->upload_directory.c_str() );

            int fd = mkstemp( tmpfilename );

            if ( this->cur_filename != "" ) {
                this->filenames[tmpfilename] = this->cur_filename;
            }

            if ( write( fd, ( const char* )this->cur_read, this->max_read ) != this->max_read ) {
                throw new runtime_error( "No full write. Disk full?" );
            }

            close( fd );
            this->FileUploaded.emit( this->cur_filename );

            this->advancePointer( this->max_read + 2 ); // +2 -> Skip trailing \r\n
            stat = this->processChunk();
            break;
        }

        case PARTIALFILE: {

            int tp = this->part;

            char tmpfilename[100];
            sprintf( tmpfilename, "%s/filXXXXXX", this->upload_directory.c_str() );

            int fd = mkstemp( tmpfilename );

            if ( this->cur_filename != "" ) {
                this->filenames[tmpfilename] = this->cur_filename;
            }

            do {
                if ( write( fd, ( const char* )this->cur_read, this->max_read ) != this->max_read ) {
                    throw new runtime_error( "No full write. Disk full?" );
                }
                this->advancePointer( this->max_read );
                stat = this->processChunk();
            } while ( tp == this->part && !this->cancel );

            close( fd );

            if ( this->filenames[tmpfilename] != "" ) {
                this->FileUploaded.emit( this->filenames[tmpfilename] );
            }

            break;
        }
        default:
            throw new runtime_error( "Unknown state in processmulti" );
            break;
        }
    }

}




/* Process input and find parts
 *
 * Returns  -1 - Error, out of sync or bug
 * 			0 - we are done
 * 			1 - we retreived a full field
 * 			2 - we retreived a partial field
 * 			3 - we retreived a full file
 * 			4 - we retreived a partial file
*/
int ECGI::processChunk() {
    int result = ERROR;
    unsigned char* pos;

    if ( state == EMPTY ) {
        this->readPostPart();
        state = STARTOFFIELD;
    }

    switch ( state ) {
    case STARTOFFIELD:
        pos = this->findBoundary();
        if ( pos ) {
            // We should now have a post part looking something like:
            //
            // Content-Disposition: form-data; name="formentryname"; filename="localfilename"
            // Content-Type: application/octet-stream
            // [empty line]
            //
            // Or the trailing --\r\n from end boundary

            this->advancePointer( this->boundary.length() + ( pos - this->cur_read ) );

            // We found a new part
            this->part++;

            // Check if final boundary
            if ( this->buf_fill > 3 ){
                unsigned char *rp=this->cur_read;
                if( rp[0]=='-' && rp[1]=='-' && rp[2]=='\r' && rp[3]=='\n'){
                    result = DONE;
                    state = END;
                    break;
               	}
            }

            this->advancePointer( 2 ); // Skip \r\n after boundary

            pos = this->readString();
            if ( pos ) {
                if (( strncasecmp(( char* )pos, disposition.c_str(), disposition.length() ) == 0 ) &&
                        strlen(( char* )pos ) > disposition.length() ) {
                    // We found a "Content-Disposition:"
                    pos += disposition.length();
                    while ( *pos && !isalpha( *pos ) ) pos++;

                    if (( strncasecmp(( char* )pos, formdata.c_str(), formdata.length() ) == 0 ) &&
                            strlen(( char* )pos ) > formdata.length() ) {
                        // We found "form-data;" from post
                        pos += formdata.length();

                        while ( *pos && !isalpha( *pos ) ) pos++;

                        if ( strncasecmp(( char* )pos, formname.c_str(), formname.length() ) == 0 ) {
                            // "name=" part of post
                            pos += formname.length();

                            if ( *pos == '"' )pos++; // Eat initial "

                            unsigned char* namestart = pos;
                            while (( *pos ) && ( *pos != '"' ) )pos++; // Find trailing "
                            *pos = '\0';
                            pos++; // skip beyond added \0
                            this->cur_fieldname = ( char* )namestart;

                            while ( *pos && !isalpha( *pos ) ) pos++;

                            if (( strncasecmp(( char* )pos, formfilename.c_str(), formfilename.length() ) == 0 ) &&
                                    strlen(( char* )pos ) > formfilename.length() ) {
                                // We have found a filename
                                pos += formfilename.length();

                                if ( *pos == '"' )pos++; // Eat initial
                                unsigned char* namestart = pos;
                                while (( *pos ) && ( *pos != '"' ) )pos++; // Find trailing "
                                *pos = '\0';
                                pos++; // skip beyond added \0
                                this->cur_filename = this->getFileName(( char* )namestart );

                                unsigned char* tmp;
                                do {
                                    tmp = this->readString();
                                } while ( *tmp );

                                if (( tmp = this->findBoundary() ) ) {
                                    result = FULLFILE;
                                    this->max_read = ( tmp - this->cur_read ) - 2; // -2 to skip trailing \r\n
                                    *( this->cur_read + this->max_read ) = '\0';
                                } else {
                                    result = PARTIALFILE;
                                    this->max_read = this->buf_fill - BOUNDARY_MARGIN > 0 ? this->buf_fill - BOUNDARY_MARGIN : 0;
                                }

                            } else {
                                // No filename this is an ordinary field

                                // Read out any garbage stop after empty line
                                unsigned char* tmp;
                                do {
                                    tmp = this->readString();
                                } while ( *tmp );

                                if (( tmp = this->findBoundary() ) ) {
                                    result = FULLFIELD;
                                    this->max_read = ( tmp - this->cur_read ) - 2;
                                    *( this->cur_read + this->max_read ) = '\0';
                                } else {
                                    result = PARTIALFIELD;
                                    this->max_read = this->buf_fill - BOUNDARY_MARGIN > 0 ? this->buf_fill - BOUNDARY_MARGIN : 0;
                                }

                            }
                        } else {
                            throw new runtime_error( "No form name with disposition" );
                        }

                    } else {
                        throw new runtime_error( "No form data tag found" );
                    }

                } else {
                    throw new runtime_error( "Out of sync when parsing post" );
                }
            } else {
                throw new runtime_error( "No string found when expected" );
            }

        } else {
            throw new runtime_error( "Boundary not found." );
        }

        if (( result == PARTIALFIELD ) || ( result == PARTIALFILE ) ) {
            state = INFIELD;
        }

        if (( result == FULLFIELD ) || ( result == FULLFILE ) ) {
            state = STARTOFFIELD;
        }

        break;

    case INFIELD:
        // We are in a middle of a field fill up buffer and look for boundary
        this->readPostPart();

        unsigned char* tmp;
        if (( tmp = this->findBoundary() ) ) {
            if ( last_result == PARTIALFIELD ) {
                result = FULLFIELD;
            } else if ( last_result == PARTIALFILE ) {
                result = FULLFILE;
            } else {
                throw new runtime_error( "Unknown state in middle field read" );
            }
            state = STARTOFFIELD;
            this->max_read = ( tmp - this->cur_read ) - 2;

        } else {
            // Still no new boundary result should be unmodified.
            result = last_result;
            this->max_read = this->buf_fill - BOUNDARY_MARGIN;
        }

        break;

    case END:
        result = 0;
        break;

    default:
        // Should not get here
        throw new runtime_error( "Illegal state in processChunk" );
        break;
    }
    last_result = result;
    return result;
}

void ECGI::parseEnv() {
    char **iter = environ;
    int index;
    while ( *iter ) {
        string row( *iter );
        index = row.find( "=" );
        this->env[row.substr( 0,index )] = row.substr( index + 1, row.size() - index );
        iter++;
    }
}


// Fill up buffer from post file descriptor
ssize_t ECGI::readPostPart( void ) {

    // Sanity check
    if ( this->buf_fill < 0 ) {
        this->buf_fill = 0;
    }

    if ( this->buf_fill > this->bufsize ) {
        this->buf_fill = 0;
    }

    if ( this->cur_read < this->readbuf ) {
        this->buf_fill = 0;
    }

    if (( this->cur_read + this->buf_fill ) > ( this->readbuf + this->bufsize ) ) {
        this->buf_fill = 0;
    }

    // We have vaild content in buffer
    if ( this->buf_fill > 0 ) {
        // Move to front
        memmove( this->readbuf, this->cur_read, this->buf_fill );
    }

    // Reset read pointer
    this->cur_read = this->readbuf;

    int64_t num_to_read = this->bufsize - this->buf_fill;

    // Not enough content to read?
    if ( num_to_read > ( this->content_length - this->content_read ) ) {
        num_to_read = this->content_length - this->content_read;
    }

    int bytes_read = 0;

    if ( num_to_read > 0 ) {
        bytes_read = read( this->fd, this->cur_read + this->buf_fill, num_to_read );
        if ( bytes_read <= 0 ) {
            throw new runtime_error( string( "Read stream failed: " ) + strerror( errno ) );
        }
    } else {
        // Nothing to read
    }
    this->buf_fill += bytes_read;
    this->content_read += bytes_read;

    return bytes_read;
}

unsigned char* ECGI::readString() {

    unsigned char *rp, *lstart = NULL;

    rp = this->cur_read;

    if ( rp ) {

        // Test for empty line, special case
        if ( rp[0] == 0x0d && rp[1] == 0x0a ) {
            rp[0] = rp[1] = 0;
            this->advancePointer( 2 );
            return rp + 1;
        }

        // Eat initial junk
        while ( rp && ( *rp < 0x20 ) && ( rp < ( this->cur_read + this->buf_fill ) ) ) {
            rp++;
        }
        lstart = rp; //Found start of line
        // Find end of line
        while ( rp && ( *rp != 0x0d ) && ( rp < ( this->cur_read + this->buf_fill ) ) ) {
            rp++;
        }
        // Eat trailing NL and replace by zero
        *rp = '\0';
        rp++;
        *rp = '\0';
        rp++;

        this->advancePointer( rp - this->cur_read );
    }

    return lstart;
}

string ECGI::getFileName( string s ) {
    string::size_type pos;
    if (( pos = s.find_last_of( "\\" ) ) != string::npos ) {
        return s.substr( pos + 1 );
    } else {
        return s;
    }

}

}
