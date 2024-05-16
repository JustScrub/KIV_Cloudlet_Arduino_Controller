/*
# $BEGIN_KEYHOLE_LICENSE$
# 
# This file is part of the Keyhole project, a lightweight library for
# the Arduino IDE, for interpreting commands and communicating variable
# values via a JSON- and Python-compatible text interface. 
# 
# Copyright (c) 2023- Jeremy Hill
# 
# Behold the 3-Clause BSD License under which it is hereby released:
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# 
# $END_KEYHOLE_LICENSE$

*/
#ifndef   __Keyhole_CPP__
#define   __Keyhole_CPP__

#include "Keyhole.h"
Keyhole::Keyhole( Stream & _stream, float _autoSeconds, bool _plotterMode ) :
	stream( _stream ),
	autoSeconds( _autoSeconds ),
	plotterMode( _plotterMode ),
	mBeginMicros( 0 ),
	//mFullCommand( "" ),
	mListAllVariables( 0 ),
	//mPartialCommand( "" ),
	mBackslash( false ),
	mHexEscape( 0 ),
	mHexValue( '\0' ),
	mQuote( '\0' ),
	mTimestampOfLastAutoReport( 0 ),
	mBad( '\xFF' )
{
	/*     Members are set up.
	   What more do you want to see?
	      No lines of code here.    */
}

Keyhole::~Keyhole()
{
	this->end();
}
	
bool Keyhole::begin( void )
{
	return this->begin( this->autoSeconds ? micros() : 0 );
}

bool Keyhole::begin( unsigned long microsecondTimestamp )
{
	mBeginMicros = microsecondTimestamp;
	while( this->stream.available() )
	{
		char c = this->stream.read();	
		if( !mQuote && ( c == ';' || c == '\n' ) )
		{
			// Solution 1
			//mFullCommand = mPartialCommand; // TODO: on some boards this messes up when there's a null byte in there
			//mFullCommand.trim();
			
			// Solution 2
			assignString( mFullCommand, mPartialCommand.c_str(), mPartialCommand.length(), true );
						
			if( mFullCommand == "?" ) { mListAllVariables = 1; mFullCommand = ""; }
			mPartialCommand = "";
			mBackslash = false;
			mHexEscape = 0;
			mHexValue = '\0';
			mQuote = '\0';
			return true;
		}
		bool escape = ( c == '\\' && !mBackslash && mQuote );
		if( mHexEscape == 2 )
		{
			if(      c >= 'A' && c <= 'F' ) { mHexValue = c - 'A' + 10; mHexEscape = 1; continue; }
			else if( c >= 'a' && c <= 'f' ) { mHexValue = c - 'a' + 10; mHexEscape = 1; continue; }
			else if( c >= '0' && c <= '9' ) { mHexValue = c - '0'     ; mHexEscape = 1; continue; }
			else mHexEscape = 0;
		}
		if( mHexEscape == 1 )
		{
			if(      c >= 'A' && c <= 'F' ) { c = c - 'A' + 10 + mHexValue * 16; }
			else if( c >= 'a' && c <= 'f' ) { c = c - 'a' + 10 + mHexValue * 16; }
			else if( c >= '0' && c <= '9' ) { c = c - '0'      + mHexValue * 16; }
			else mPartialCommand += mHexValue;
			mHexEscape = 0;
			mHexValue  = 0;
		}
		if( mBackslash )
		{
			if(      c == 'n' ) c = '\n';
			else if( c == 'r' ) c = '\r';
			else if( c == 't' ) c = '\t';
			else if( c == '0' ) c = '\0';
			else if( c == 'x' ) { mBackslash = false; mHexEscape = 2; continue; }
		}
		if( !escape && ( mPartialCommand.length() || !isspace( c ) ) ) mPartialCommand += c;
		if( !mQuote && ( c == '\'' || c == '"' ) ) mQuote = c;
		else if( mQuote && c == mQuote && !mBackslash ) mQuote = '\0';
		mBackslash = escape;
	}
	if( autoSeconds > 0.0 && microsecondTimestamp - mTimestampOfLastAutoReport >= ( unsigned long )( autoSeconds * 1e6 ) )
	{
		mTimestampOfLastAutoReport = microsecondTimestamp;
		mListAllVariables = 1;
		return true;
	}
	return false;
}

bool Keyhole::command( const char * cmd )
{
	if( mFullCommand != cmd ) return false;
	mFullCommand = "";
	return true;
}

// Let's define some hefty macros for internal use:

#define _DEFINE_LSHIFT( TYPE )   Kout Keyhole::operator<<( TYPE x ) { Kout s( this->stream ); s << x; return s; }
_DEFINE_LSHIFT( const char * ) // edge-case that will not be covered by the various _START_VARIABLE_PROCESSOR() macro calls
_DEFINE_LSHIFT( const Kfmt & )

#define _START_VARIABLE_PROCESSOR( TYPE, PRINT_STATEMENT, PLOTTABLE ) \
	_DEFINE_LSHIFT( const TYPE & ) \
	bool Keyhole::variable( const char * key, TYPE & var, KeyholeWriteMode writeMode ) \
	{ \
		bool allowOutput = PLOTTABLE || !this->plotterMode; \
		if( mListAllVariables && allowOutput ) \
		{ \
			if( this->plotterMode ) this->stream.print( ( mListAllVariables++ == 1 ) ?    "" : ","    ); \
			else                    this->stream.print( ( mListAllVariables++ == 1 ) ? "{\"" : ", \"" ); \
			this->stream.print( key ); \
			if( this->plotterMode ) this->stream.print(   ":"  ); \
			else                    this->stream.print( "\": " ); \
			PRINT_STATEMENT; \
		} \
		unsigned int commandLength; \
		const char * commandPtr = _parseVariableCommand( key, commandLength ); /* this quickly returns NULL if mFullCommand has been used and emptied already */ \
		if( !commandPtr ) return false; /* ...so this effectively shortcuts the whole thing if a command has already been matched since the call to begin() */ \
		if( !*commandPtr && !this->plotterMode ) { this->stream.print( "{\"" ); this->stream.print( key ); this->stream.print( "\": " ); PRINT_STATEMENT; this->stream.println( "}" ); this->stream.flush();   mFullCommand = ""; return false; } \
		if( !*commandPtr &&  this->plotterMode ) { if( allowOutput ) {          this->stream.print( key ); this->stream.print(   ":"  ); PRINT_STATEMENT; this->stream.println(     ); this->stream.flush(); } mFullCommand = ""; return false; } \
		if( writeMode == VARIABLE_READ_ONLY ) { this->_startError( "ReadOnly" ); this->stream.print( "\"cannot change the '" ); this->stream.print( key ); this->stream.println( "' variable because it is read-only\"}" ); this->stream.flush(); mFullCommand = ""; return false; } \
		char *remainder = NULL; \
		TYPE value;
		//
		// conversion of const char * commandPtr to TYPE value happens here between _START_VARIABLE_PROCESSOR and _END_VARIABLE_PROCESSOR macros
		//
#define _END_VARIABLE_PROCESSOR( TYPE, PRINT_STATEMENT, ASSIGN ) \
		while( remainder && isspace( *remainder ) ) remainder++; \
		if( remainder && *remainder ) \
		{ \
			this->_startError( "BadValue" ); \
			this->stream.print( "\"failed to interpret argument as type '" ); \
			this->stream.print( #TYPE ); \
			this->stream.print( "' when setting the '" ); \
			this->stream.print( key ); \
			this->stream.println( "' variable\"}" ); \
			this->stream.flush(); \
			mFullCommand = ""; \
			return false; \
		} \
		if( ASSIGN ) var = value; \
		if( writeMode == VARIABLE_VERBOSE && allowOutput ) \
		{ \
			if( !this->plotterMode ) this->stream.print( "{\"" ); \
			this->stream.print( key ); \
			this->stream.print( this->plotterMode ? ":" : "\": " ); \
			PRINT_STATEMENT; \
			this->stream.println( this->plotterMode ? "" : "}" ); \
			this->stream.flush(); \
		} \
		mFullCommand = ""; \
		return true; \
	}

// Now let us use the above macros to define multiple polymorphisms, supporting different variable TYPEs, of:
// bool variable( const char * key, TYPE & referenceToVariable,  KeyholeWriteMode mode=VARIABLE_SILENT );
// (mode can be VARIABLE_READ_ONLY, VARIABLE_SILENT or VARIABLE_VERBOSE)

///////////////////////////////////////////////////////////////////////////////
_START_VARIABLE_PROCESSOR( String,         this->printLiteral( var ),  false )
	char quote = *commandPtr++; commandLength--;
	if( quote != '"' && quote != '\'' ) remainder = &mBad;
	else
	{
		while( commandLength && isspace( commandPtr[ commandLength - 1 ] ) ) commandLength--; // trim trailing space
		if( !commandLength || commandPtr[ commandLength - 1 ] != quote ) remainder = &mBad;
		else
		{
			commandLength--; // remove closing quote
			// At this point we know parsing has succeeded, so now let's assign directly to `var`, skipping the
			// intermediate step from `value` that the other variable() polymorphs do. Anyway note that some
			// boards mess up copying `string2 = string1;` if `string1` contains null characters, and that some
			// boards do not have a `String(ptr,length)` constructor.
			assignString( var, commandPtr, commandLength, false );
		}
	}
_END_VARIABLE_PROCESSOR(   String,         this->printLiteral( var ), false )
///////////////////////////////////////////////////////////////////////////////
_START_VARIABLE_PROCESSOR( char,           this->printLiteral( var ),  true )
	value = '\0';
	char quote = '\'';
	if( *commandPtr == quote )
	{
		while( commandLength && isspace( commandPtr[ commandLength - 1 ] ) ) commandLength--; // trim trailing space
		commandPtr++; commandLength--; // skip opening quote
		if( commandLength == 2 && commandPtr[ 1 ] == quote ) value = commandPtr[ 0 ]; // check for closing quote and take the payload (it will already have been unescaped, in begin())
		else remainder = &mBad;
	}
	else value = strToSignedInteger( commandPtr, &remainder );
_END_VARIABLE_PROCESSOR(   char,           this->printLiteral( var ),  true )
///////////////////////////////////////////////////////////////////////////////
_START_VARIABLE_PROCESSOR( bool,           this->stream.print( var ),  true )
	value = strToUnsignedInteger( commandPtr, &remainder );
	if( remainder )
	{
		String s; assignString( s, commandPtr, commandLength, false, true );
		if(      s == "true"  ) { value = true;  remainder = NULL; }
		else if( s == "false" ) { value = false; remainder = NULL; }
	}
_END_VARIABLE_PROCESSOR(   bool,           this->stream.print( var ),  true )
///////////////////////////////////////////////////////////////////////////////
#define PRINT_FLOAT( X )   this->printLiteral( X, this->plotterMode ? '\0' : '"' )
_START_VARIABLE_PROCESSOR( int8_t,         this->stream.print( var ),  true )   value = strToSignedInteger(   commandPtr, &remainder );   _END_VARIABLE_PROCESSOR(   int8_t,         this->stream.print( var ),       true )
_START_VARIABLE_PROCESSOR( unsigned char,  this->stream.print( var ),  true )   value = strToUnsignedInteger( commandPtr, &remainder );   _END_VARIABLE_PROCESSOR(   unsigned char,  this->stream.print( var ),       true )
_START_VARIABLE_PROCESSOR( int,            this->stream.print( var ),  true )   value = strToSignedInteger(   commandPtr, &remainder );   _END_VARIABLE_PROCESSOR(   int,            this->stream.print( var ),       true )
_START_VARIABLE_PROCESSOR( unsigned int,   this->stream.print( var ),  true )   value = strToUnsignedInteger( commandPtr, &remainder );   _END_VARIABLE_PROCESSOR(   unsigned int,   this->stream.print( var ),       true )
_START_VARIABLE_PROCESSOR( short,          this->stream.print( var ),  true )   value = strToSignedInteger(   commandPtr, &remainder );   _END_VARIABLE_PROCESSOR(   short,          this->stream.print( var ),       true )
_START_VARIABLE_PROCESSOR( unsigned short, this->stream.print( var ),  true )   value = strToUnsignedInteger( commandPtr, &remainder );   _END_VARIABLE_PROCESSOR(   unsigned short, this->stream.print( var ),       true )
_START_VARIABLE_PROCESSOR( long,           this->stream.print( var ),  true )   value = strToSignedInteger(   commandPtr, &remainder );   _END_VARIABLE_PROCESSOR(   long,           this->stream.print( var ),       true )
_START_VARIABLE_PROCESSOR( unsigned long,  this->stream.print( var ),  true )   value = strToUnsignedInteger( commandPtr, &remainder );   _END_VARIABLE_PROCESSOR(   unsigned long,  this->stream.print( var ),       true )
_START_VARIABLE_PROCESSOR( float,          PRINT_FLOAT( var ),         true )   value = ( float )strToDouble( commandPtr, &remainder );   _END_VARIABLE_PROCESSOR(   float,          PRINT_FLOAT( var ),              true )
_START_VARIABLE_PROCESSOR( double,         PRINT_FLOAT( var ),         true )   value = strToDouble(          commandPtr, &remainder );   _END_VARIABLE_PROCESSOR(   double,         PRINT_FLOAT( var ),              true )
///////////////////////////////////////////////////////////////////////////////


bool Keyhole::end( void )
{
	if( mListAllVariables ) { this->stream.println( this->plotterMode ? "" : "}" ); this->stream.flush(); mListAllVariables = 0; }
	if( mFullCommand.length() ) { this->error( "failed to recognize command", "BadKey" ); mFullCommand = ""; return true; }
	return false;
}

void Keyhole::error( const String & msg, const String & type )
{
	_startError( type );
	this->printLiteral( msg, '"' );
	this->stream.println( "}" );
	this->stream.flush();
}

void Keyhole::_startError( const String & type )
{
	this->stream.print( "{\"_KEYHOLE_ERROR_TYPE\": ");
	this->printLiteral( type, '"' );
	this->stream.print( ", \"_KEYHOLE_ERROR_MSG\": " );
}


unsigned long Keyhole::elapsedMicros( void )
{
	// NB: result will not be valid unless you passed a valid micros() reading to .begin()
	// With 6 integer variables and 2 commands, on a 2021 Raspberry Pi Pico:
	// * around 600 microseconds to process and respond to the "?" command and print all variable values
	// * 230-260 microseconds overhead on a loop in which an integer variable() value was assigned (without verbose response)
	// * 150-190 microseconds overhead on a loop in which a "dummy" command() was received and recognized (no actions or outputs in response)
	// * 4-11 microseconds overhead on a more-typical loop in which no command was received
	return micros() - mBeginMicros;
}
	
void Keyhole::printLiteral( double f, char withQuotes )
{
	double z = f;
	z *= 0.0;
	const int precision = 4;
	if( z == 0.0 )
	{
		this->stream.print( f, precision ); // still no quotes, that's intentional - we only need them for non-numeric-looking renderings
		// TODO: a couple of things are missing:
		//       - support for scientific notation (you get "ovr" if the number gets too large)
		//       - intelligent variation in precision (like printf %g)
	}
	else
	{
		// we're in inf and nan territory now - that's where we need quotes, to keep JSON/Python happy
		if( ( unsigned char )withQuotes > 127 ) withQuotes = '\0';
		this->stream.print( withQuotes );
		if(      f < 0 ) this->stream.print( "-inf" ); // this works around a bug whereby (some architectures?) 
		else if( f > 0 ) this->stream.print(  "inf" ); // render -inf as just "inf" (even though you can show
		else this->stream.print( f, precision );       // that they know it is < 0)
		this->stream.print( withQuotes );
	}
}

void Keyhole::printLiteral( char c, char withQuotes )
{	
	if( withQuotes == ( char )-1 )
	{
		// This option prints 97 rather than  a  or  'a'  or  "a"   and 9 rather than  \t  or  '\t'  or  "\t" 
		// which is legal everywhere and reflects the fact that char is the same thing as int8_t or uint8_t
		this->stream.print( ( int )c );
	}
	else
	{
		// This option prints  a  or  'a'  or  "a"   and   \t  or  '\t'  or  "\t"  depending on whether you
		// set withQuotes to '\0' or '\'' or '"'
		// Note that only the double-quote option is legal in JSON.

		//this->printLiteral( String( &c, 1 ), withQuotes ); return;
		// but unfortunately the String(ptr,n) constructor is not supported on all architectures, so instead:
		String s; s += c; this->printLiteral( s, withQuotes );
	}
	// The default is withQuotes=-1 to avoid problems on the other side: even with the double-quote option
	// you would be creating a Python or Javascript object that behaves fundamentally differently from the
	// way a char behaves in a sketch (i.e. as an int8_t or uint8_t, depending on processor architecture).
}
void Keyhole::printLiteral( const String & s, char withQuotes )
{
	if( ( unsigned char )withQuotes > 127 ) withQuotes = '\0';
	if( withQuotes ) this->stream.print( withQuotes );
	for( unsigned int i = 0; i < s.length(); i++ )
	{
		char c = s[ i ];
		if(      c == '\t' ) this->stream.print( "\\t" );
		else if( c == '\r' ) this->stream.print( "\\r" );
		else if( c == '\n' ) this->stream.print( "\\n" );
		else if( c == '\0' ) this->stream.print( "\\0" );
		else if( c == '\\' ) this->stream.print( "\\\\" );
		else if( c == withQuotes ) { this->stream.print( "\\" ); this->stream.print( withQuotes ); }
		else if( isprint( c ) ) this->stream.print( c );
		else
		{
			this->stream.print( "\\x" );
			if( ( unsigned char )c < 16 ) this->stream.print( "0" );
			this->stream.print( ( unsigned char )c, HEX );
		}
	}
	if( withQuotes ) this->stream.print( withQuotes );
}

const char * Keyhole::_parseVariableCommand( const char * key, unsigned int & commandLength )
{
	commandLength = mFullCommand.length();
	if( !commandLength ) return NULL;
	const char * commandPtr = mFullCommand.c_str();
	int keyLength = strlen( key );
	if( strncmp( commandPtr, key, keyLength ) != 0 ) return NULL;
	commandPtr += keyLength;
	commandLength -= keyLength;
	while( isspace( *commandPtr ) ) { commandPtr++; commandLength--; }
	if( *commandPtr && *commandPtr++ != '=' ) return NULL;
	if( commandLength ) commandLength--;
	while( isspace( *commandPtr ) ) { commandPtr++; commandLength--; }
	return commandPtr;
}

void Keyhole::assignString( String & dst, const String & src, bool trim, bool lowercase )
{
	Keyhole::assignString( dst, src.c_str(), src.length(), trim, lowercase );
}

void Keyhole::assignString( String & dst, const char *ptr, unsigned int length, bool trim, bool lowercase )
{ // this exists as a workaround because some board libraries mess up when copying `string2 = string1;` if string1 contains null characters
	if( trim )
	{
		while( length && isspace( *ptr ) ) { ptr++; length--; }
		while( length && isspace( ptr[ length - 1 ] ) ) length--;
	}
	dst = "";
	dst.reserve( length );
	while( length-- )
	{
		char c = *ptr++;
		if( lowercase ) c = tolower( c );
		dst += c;
	}
}

unsigned long Keyhole::strToUnsignedInteger( const char * start, char ** endptr )
{
	if( start && start[ 0 ] == '0' && ( start[ 1 ] == 'x' || start[ 1 ] == 'X' ) && start[ 2 ] ) return strtoul( start + 2, endptr, 16 );
	if( start && start[ 0 ] == '0' && ( start[ 1 ] == 'b' || start[ 1 ] == 'B' ) && start[ 2 ] ) return strtoul( start + 2, endptr,  2 );
	/******************************************************************************************/ return strtoul( start,     endptr, 10 );
}

long Keyhole::strToSignedInteger( const char * start, char ** endptr )
{
	if( start && start[ 0 ] == '0' && ( start[ 1 ] == 'x' || start[ 1 ] == 'X' ) && start[ 2 ] ) return strtol( start + 2, endptr, 16 );
	if( start && start[ 0 ] == '0' && ( start[ 1 ] == 'b' || start[ 1 ] == 'B' ) && start[ 2 ] ) return strtol( start + 2, endptr,  2 );
	/******************************************************************************************/ return strtol( start,     endptr, 10 );
}

double Keyhole::strToDouble( const char * start, char ** endptr )
{
	if( start && start[ 0 ] == '0' && ( start[ 1 ] == 'x' || start[ 1 ] == 'X' ) && start[ 2 ] ) return ( double )strtol( start + 2, endptr, 16 );
	if( start && start[ 0 ] == '0' && ( start[ 1 ] == 'b' || start[ 1 ] == 'B' ) && start[ 2 ] ) return ( double )strtol( start + 2, endptr,  2 );
	return ( double )strtod( start, endptr );
}

void Keyhole::flicker( int ledPin, unsigned long millisOn, unsigned long millisOff, unsigned long millisTotal )
{
	if( ledPin < 0 ) return;
	unsigned long startTime = millis();
	pinMode( ledPin, OUTPUT );
	for( unsigned long i = 0; millis() - startTime < millisTotal; i++ )
	{
		digitalWrite( ledPin, i % 2 ? LOW : HIGH );
		delay( i % 2 ? millisOff : millisOn );
	}
	digitalWrite( ledPin, LOW );
}

Kout::Kout( Stream  & s ):        stream( s ),            mArmed( true ), mFloatPrecision( 2 ),                     mQuoteChar( ( char )Kfmt::DO_NOT_ESCAPE ), mClosingString( NULL ) {}
Kout::Kout( Keyhole & k ):        stream( k.stream ),     mArmed( true ), mFloatPrecision( 2 ),                     mQuoteChar( ( char )Kfmt::DO_NOT_ESCAPE ), mClosingString( NULL ) {}
Kout::Kout( const Kout & other ): stream( other.stream ), mArmed( true ), mFloatPrecision( other.mFloatPrecision ), mQuoteChar( other.mQuoteChar ),              mClosingString( other.mClosingString ) {}
Kout::Kout( Kout && other ):      stream( other.stream ), mArmed( true ), mFloatPrecision( other.mFloatPrecision ), mQuoteChar( other.mQuoteChar ),              mClosingString( other.mClosingString ) { other.mArmed = false; }
Kout::~Kout()
{
	if( !mArmed ) return;
	if( mClosingString && *mClosingString ) this->stream.print( mClosingString );
	this->stream.println();
	this->stream.flush();
}
Kout & Kout::operator<<( const char *           x ) { return x ? ( *this << String( x ) ) : *this; }
Kout & Kout::operator<<( const String         & x ) { if( mQuoteChar == ( char )Kfmt::DO_NOT_ESCAPE ) this->stream.print( x ); else Keyhole( this->stream ).printLiteral( x, mQuoteChar ); return *this; }
Kout & Kout::operator<<( const char           & x ) { if( mQuoteChar == ( char )Kfmt::DO_NOT_ESCAPE ) this->stream.print( x ); else Keyhole( this->stream ).printLiteral( x, mQuoteChar ); return *this; }
Kout & Kout::operator<<( const unsigned char  & x ) { this->stream.print( x ); return *this; }
Kout & Kout::operator<<( const bool           & x ) { this->stream.print( x ); return *this; }
Kout & Kout::operator<<( const int            & x ) { this->stream.print( x ); return *this; }
Kout & Kout::operator<<( const unsigned int   & x ) { this->stream.print( x ); return *this; }
Kout & Kout::operator<<( const short          & x ) { this->stream.print( x ); return *this; }
Kout & Kout::operator<<( const unsigned short & x ) { this->stream.print( x ); return *this; }
Kout & Kout::operator<<( const long           & x ) { this->stream.print( x ); return *this; }
Kout & Kout::operator<<( const unsigned long  & x ) { this->stream.print( x ); return *this; }
Kout & Kout::operator<<( const float          & x ) { this->stream.print( x, mFloatPrecision ); return *this; }
Kout & Kout::operator<<( const double         & x ) { this->stream.print( x, mFloatPrecision ); return *this; }
Kout & Kout::operator<<( const int8_t         & x ) { this->stream.print( x ); return *this; }
Kout & Kout::operator<<( const Kfmt & x )
{
	if( x.mQuoteChar != ( char )Kfmt::NO_CHANGE ) mQuoteChar = x.mQuoteChar;
	if( x.mFloatPrecision != Kfmt::NO_CHANGE ) mFloatPrecision = x.mFloatPrecision;
	if( x.mClosingString != NULL ) mClosingString = ( *x.mClosingString ) ? x.mClosingString : NULL;
	return *this;
}
Kfmt::Kfmt() : mFloatPrecision( Kfmt::NO_CHANGE ), mQuoteChar( ( char )Kfmt::NO_CHANGE ), mClosingString( NULL ) {}
Kfmt & Kfmt::floatPrecision( int precision ) { mFloatPrecision = precision; return *this; }
Kfmt & Kfmt::quote( char quoteChar ) { mQuoteChar = quoteChar; return *this; }
Kfmt & Kfmt::closingString( const char * s ) { mClosingString = s; return *this; } 
Kfmt::~Kfmt() {}

Kout Keyhole::errorStream( const String & errorType )
{
	_startError( errorType ); // start the JSON dictionary using standardized error-related keys
	this->stream.print( '"' ); // manually open the quotes for the error message
	Kout s( this->stream ); // open a Kout instance into which the caller can then feed pieces of the error message using a chain of << operators
	s << KFMT.quote( '\0' ); // set it to escape any non-printables, but not to put actual quotes around every string the caller feeds in 
	s << KFMT.closingString( "\"}" ); // set a flag to manually close the quotes as well as the JSON dictionary, before the automatic line-ending
	return s;
}

#endif // __Keyhole_CPP__
