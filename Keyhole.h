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

The following example allows two global variables to be queried and/or
set over a serial port or other Stream::

    #include "Keyhole.h"
  
    double foo = 0.0;
    String bar = "look at these escaped characters: \" \x08 \\ \t\r\n";
  
    void setup()
    {
      Serial.begin(9600);
    }
  
    void loop()
    {
      // main business of your sketch goes here
      // ...
      
      KEYHOLE keyhole(Serial);
      if( keyhole.begin() )
      {
        if(keyhole.variable("foo", foo)) Serial.println("assigned to foo");
        if(keyhole.variable("bar", bar)) Serial.println("assigned to bar");
        if(keyhole.command("Marco!")) Serial.println("Polo!");
        keyhole.end();
      }
    }
    
An optional third argument to `variable()` can be `VARIABLE_READ_ONLY`,
`VARIABLE_VERBOSE`, or the default value `VARIABLE_SILENT`.

In this example, if you send the command `foo` over the serial port,
terminated either by a semicolon or a newline (the latter of which
gets added automatically by the Arduino IDE's Serial Monitor) the
reply will be a JSON string associating the key `foo` with the
current value of the corresponding global variable `foo`. 

Provided the variable is not designated as `VARIABLE_READ_ONLY`, you can
also set its value, e.g. by sending the command `foo = 2`.  (If you have
designated the variable as `VARIABLE_VERBOSE` then this will also trigger
a JSON output, as if you had queried `foo` immediately after setting it.)

You can send the simple command `?` to receive a JSON output containing all
the variables that are accessible in this manner.

You can also have the full report delivered automatically on a repeating
schedule by setting the `.autoSeconds` member greater than zero; you can
even allow this parameter itself to be controlled via the keyhole, by
exposing the `autoSeconds` member itself as a variable::

      KEYHOLE keyhole(Serial);
      if(keyhole.begin())
      {
        if(keyhole.variable("foo", foo)) Serial.println("assigned to foo");
        if(keyhole.variable("bar", bar)) Serial.println("assigned to bar");
        keyhole.variable("report_period_sec", keyhole.autoSeconds);
        keyhole.end();
      }

Under the hood: `keyhole` is an instance of the class `Keyhole`. Such
instances must either be global variables, or declared `static`. The
`KEYHOLE` macro can be used inside or outside of `loop()` - it simply
declares a `static Keyhole` instance (while hiding the possibly-not-so-
user-friendly keyword `static` from users who are not familiar with it).
  
*/

#ifndef   __Keyhole_H__
#define   __Keyhole_H__

#include "Arduino.h" // for Stream and String
#include <stdint.h>  // for int8_t

// Debugging macros:
#ifdef DBSTREAM
#	define REPORT( X )  { DBSTREAM.print( "{\"" #X "\" : " ); DBSTREAM.print( X );                         DBSTREAM.println( "}" ); DBSTREAM.flush(); }
#	define REPORTS( X ) { DBSTREAM.print( "{\"" #X "\" : " ); Keyhole( DBSTREAM ).printStringLiteral( X ); DBSTREAM.println( "}" ); DBSTREAM.flush(); }
#else
#	define REPORT( X )
#	define REPORTS( X )
#endif

class Keyhole;
class Kout;
class Kfmt;

typedef enum
{
	VARIABLE_READ_ONLY = 0,
	VARIABLE_SILENT    = 1,
	VARIABLE_VERBOSE   = 2
} KeyholeWriteMode;

#define KEYHOLE       static Keyhole
class Keyhole
{
	public:
		// Typically, you should declare instances of the Keyhole class as static (the KEYHOLE macro does this for you).
		Keyhole( Stream & stream=Serial, float autoSeconds=0.0, bool plotterMode=false );
		~Keyhole();
	
		// begin() returns true if a command (terminated by an unquoted semicolon or newline) is ready for processing.
		bool begin( void );
		// begin() returns true if a command (terminated by an unquoted semicolon or newline) is ready for processing.
		bool begin( unsigned long microsecondTimestamp );
	
		// command() returns true if the specified command has been received (do not include the semicolon or newline terminator).
		bool command( const char * cmd );
	
		// variable() exposes a sketch variable under the specified key (default mode is VARIABLE_SILENT, other possibilities are VARIABLE_VERBOSE and VARIABLE_READ_ONLY) and returns true if an incoming command has assigned a value to the variable.
		bool variable( const char * key, bool &           referenceToVariable,  KeyholeWriteMode mode=VARIABLE_SILENT );
		// variable() exposes a sketch variable under the specified key (default mode is VARIABLE_SILENT, other possibilities are VARIABLE_VERBOSE and VARIABLE_READ_ONLY) and returns true if an incoming command has assigned a value to the variable.
		bool variable( const char * key, char &           referenceToVariable,  KeyholeWriteMode mode=VARIABLE_SILENT );
		// variable() exposes a sketch variable under the specified key (default mode is VARIABLE_SILENT, other possibilities are VARIABLE_VERBOSE and VARIABLE_READ_ONLY) and returns true if an incoming command has assigned a value to the variable.
		bool variable( const char * key, unsigned char &  referenceToVariable,  KeyholeWriteMode mode=VARIABLE_SILENT );
		// variable() exposes a sketch variable under the specified key (default mode is VARIABLE_SILENT, other possibilities are VARIABLE_VERBOSE and VARIABLE_READ_ONLY) and returns true if an incoming command has assigned a value to the variable.
		bool variable( const char * key, int &            referenceToVariable,  KeyholeWriteMode mode=VARIABLE_SILENT );
		// variable() exposes a sketch variable under the specified key (default mode is VARIABLE_SILENT, other possibilities are VARIABLE_VERBOSE and VARIABLE_READ_ONLY) and returns true if an incoming command has assigned a value to the variable.
		bool variable( const char * key, unsigned int &   referenceToVariable,  KeyholeWriteMode mode=VARIABLE_SILENT );
		// variable() exposes a sketch variable under the specified key (default mode is VARIABLE_SILENT, other possibilities are VARIABLE_VERBOSE and VARIABLE_READ_ONLY) and returns true if an incoming command has assigned a value to the variable.
		bool variable( const char * key, short &          referenceToVariable,  KeyholeWriteMode mode=VARIABLE_SILENT );
		// variable() exposes a sketch variable under the specified key (default mode is VARIABLE_SILENT, other possibilities are VARIABLE_VERBOSE and VARIABLE_READ_ONLY) and returns true if an incoming command has assigned a value to the variable.
		bool variable( const char * key, unsigned short & referenceToVariable,  KeyholeWriteMode mode=VARIABLE_SILENT );
		// variable() exposes a sketch variable under the specified key (default mode is VARIABLE_SILENT, other possibilities are VARIABLE_VERBOSE and VARIABLE_READ_ONLY) and returns true if an incoming command has assigned a value to the variable.
		bool variable( const char * key, long &           referenceToVariable,  KeyholeWriteMode mode=VARIABLE_SILENT );
		// variable() exposes a sketch variable under the specified key (default mode is VARIABLE_SILENT, other possibilities are VARIABLE_VERBOSE and VARIABLE_READ_ONLY) and returns true if an incoming command has assigned a value to the variable.
		bool variable( const char * key, unsigned long &  referenceToVariable,  KeyholeWriteMode mode=VARIABLE_SILENT );
		// variable() exposes a sketch variable under the specified key (default mode is VARIABLE_SILENT, other possibilities are VARIABLE_VERBOSE and VARIABLE_READ_ONLY) and returns true if an incoming command has assigned a value to the variable.
		bool variable( const char * key, float &          referenceToVariable,  KeyholeWriteMode mode=VARIABLE_SILENT );
		// variable() exposes a sketch variable under the specified key (default mode is VARIABLE_SILENT, other possibilities are VARIABLE_VERBOSE and VARIABLE_READ_ONLY) and returns true if an incoming command has assigned a value to the variable.
		bool variable( const char * key, double &         referenceToVariable,  KeyholeWriteMode mode=VARIABLE_SILENT );
		// variable() exposes a sketch variable under the specified key (default mode is VARIABLE_SILENT, other possibilities are VARIABLE_VERBOSE and VARIABLE_READ_ONLY) and returns true if an incoming command has assigned a value to the variable.
		bool variable( const char * key, String &         referenceToVariable,  KeyholeWriteMode mode=VARIABLE_SILENT );
		// variable() exposes a sketch variable under the specified key (default mode is VARIABLE_SILENT, other possibilities are VARIABLE_VERBOSE and VARIABLE_READ_ONLY) and returns true if an incoming command has assigned a value to the variable.
		bool variable( const char * key, int8_t &         referenceToVariable,  KeyholeWriteMode mode=VARIABLE_SILENT ); 
		// Note: a char may be considered signed or unsigned, depending on architecture.
		// So, in case it is unsigned, we needed an explicit signed-8-bit polymorph.
		// The rest of the stdint types should take care of themselves via the usual builtin types.
		
#		define variableAssigned variable // so you can express it like this if the semantics appeal to you more:
		                                 //     if( keyhole.variableAssigned("foo", foo) ) doWhatever(foo);
	
		// If begin() returned true, then you must call end() after processing all variables and commands.
		bool end( void );
	
		// error() lets you print your own error message in JSON format using the customary Keyhole keys.
		void error( const String & msg, const String & type="BadValue" );
	
		// elapsedMicros() returns the microseconds elapsed since begin() (but only if you passed micros() as an argument to .begin(), or if .autoSeconds is set > 0.0).
		unsigned long elapsedMicros( void );

		Stream &      stream;      // a reference to the Stream (e.g. Serial) used for text input and output
		float         autoSeconds; // set this >0.0 to receive periodic automatic output
		int           plotterMode; // set this to true to make the output format compatible with the Serial Plotter in the Arduino IDE
		
		// Use  k << x << "y" << z;  to print a sequence of things followed by an automatic line-ending and flush.
		Kout operator<<( const char *           x );
		Kout operator<<( const String         & x );
		Kout operator<<( const char           & x );
		Kout operator<<( const int8_t         & x );
		Kout operator<<( const bool           & x );
		Kout operator<<( const unsigned char  & x );
		Kout operator<<( const int            & x );
		Kout operator<<( const unsigned int   & x );
		Kout operator<<( const short          & x );
		Kout operator<<( const unsigned short & x );
		Kout operator<<( const long           & x );
		Kout operator<<( const unsigned long  & x );
		Kout operator<<( const float          & x );
		Kout operator<<( const double         & x );
		Kout operator<<( const Kfmt           & x );
		
		// Example:  k.errorStream("BadValue") << "I do not like the value x=" << KFMT.floatPrecision(4) << x;
		Kout errorStream( const String & errorType );
		
	private: // nothing to see here
		unsigned long mBeginMicros;
		String        mFullCommand;
		int           mListAllVariables;
		String        mPartialCommand;
		bool          mBackslash;
		int           mHexEscape;
		char          mHexValue;
		char          mQuote;
		unsigned long mTimestampOfLastAutoReport;
		char          mBad;

		const char *  _parseVariableCommand( const char * key, unsigned int & commandLength );
		void          _startError( const String & type );	
	
	public:
		// General-purpose helper method (an instance method only because it accesses `this->stream`)
		void          printLiteral( const String & s, char withQuotes='"' );
		// General-purpose helper method (an instance method only because it accesses `this->stream`)
		void          printLiteral( double f,         char withQuotes='"' );
		// General-purpose helper method (an instance method only because it accesses `this->stream`)
		void          printLiteral( char c,           char withQuotes=(char)-1 );
		// * withQuotes=0 means print characters (or escape codes) unquoted, and don't escape any quotes.
		// * withQuotes='\'' or withQuotes='"' means print the content wrapped in the specified type of quote, and
		//   escape that quote character if it occurs in the content. (For floating-point, quotes are only used if the
		//   value is an inf or a nan, for reasons of JSON legality and ast.literal_eval() compatibility.)
		// * withQuotes=-1 has a special meaning for chars which causes the char to be printed as a numeric int8_t or
		//   uint8_t value (depending on architecture). This is the default for reasons discussed in the comments of the
		//   method.
	
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// And finally some general-purpose static helper functions (but let's keep them inside the Keyhole:: namespace):
	
		// This is needed because some boards' standard libraries don't have a String(ptr,length) constructor 
		static void          assignString( String & dst, const char *ptr, unsigned int length, bool trim=false, bool lowercase=false );
		// This is needed because some boards' standard libraries may mess up copying `string2=string1;` if `string1` contains '\0'
		static void          assignString( String & dst, const String & src,                   bool trim=false, bool lowercase=false );
		
		// Works the same as the standard library function `strtoul` except optional prefixes 0x and 0b are recognized and the base (16, 2 or 10) is inferred.
		static unsigned long strToUnsignedInteger( const char * start, char ** endptr );
		// Works the same as the standard library function `strtol` except optional prefixes 0x and 0b are recognized and the base (16, 2 or 10) is inferred.
		static long          strToSignedInteger(   const char * start, char ** endptr );
		// Works the same as the standard library function `strtod` except that whole numbers may be expressed in hex or binary using optional prefixes 0x or 0b.
		static double        strToDouble(          const char * start, char ** endptr );
		
#ifdef LED_BUILTIN
		// This one is good at the start or end of `setup()`, to say hello and reassure the user that things are working.
		static void          flicker( int ledPin=LED_BUILTIN, unsigned long millisOn=50, unsigned long millisOff=50, unsigned long millisTotal=1000 );
#else
		// This one is good at the start or end of `setup()`, to say hello and reassure the user that things are working.
		static void          flicker( int ledPin,             unsigned long millisOn=50, unsigned long millisOff=50, unsigned long millisTotal=1000 );
#endif
};

// Kfmt sets certain options when feeding items to a Keyhole instance with <<
// The KFMT macro is the simplest way to use it.
// examples::
// 
//     KEYHOLE k(Serial);
//     k << "The magic number is " << KFMT.floatPrecision(6) << "1.234567";
//     k << "s = " << KFMT.quote('"') << "a string containing \n\t\x08 special characters";
class Kfmt
{
	public:
		static const int CHAR_AS_NUMERIC = -1; // understood by Keyhole::printLiteral()
		static const int DO_NOT_ESCAPE   = -2;
		static const int NO_CHANGE       = -3;
	
		 Kfmt();
		~Kfmt();
		 Kfmt & floatPrecision( int decimalPlaces );
		 Kfmt & quote( char quoteChar );
		 Kfmt & closingString( const char * s );
	
	private:
		friend class Kout;
		int          mFloatPrecision;
		char         mQuoteChar;
		const char * mClosingString;
};
// examples::
// 
//     KEYHOLE k(Serial);
//     k << "The magic number is " << KFMT.floatPrecision(6) << "1.234567";
//     k << "s = " << KFMT.quote('"') << "a string containing \n\t\x08 special characters";
#define KFMT  Kfmt()


// Kout is a helper class. You don't need to use it directly - you can actually << items into your Keyhole instance 
// directly. The helper class destructor will ensure that the line ends with a line-ending and a flush.
class Kout
{
	public:
		 Kout( Stream  & s=Serial );
		 Kout( Keyhole & k        );
		 Kout( const Kout &  other );
		 Kout( Kout && other );
		~Kout();
		 Kout & operator<<( const int8_t         & x );
		 Kout & operator<<( const bool           & x );
		 Kout & operator<<( const char           & x );
		 Kout & operator<<( const unsigned char  & x );
		 Kout & operator<<( const int            & x );
		 Kout & operator<<( const unsigned int   & x );
		 Kout & operator<<( const short          & x );
		 Kout & operator<<( const unsigned short & x );
		 Kout & operator<<( const long           & x );
		 Kout & operator<<( const unsigned long  & x );
		 Kout & operator<<( const float          & x );
		 Kout & operator<<( const double         & x );
		 Kout & operator<<( const String         & x );
		 Kout & operator<<( const char *           x );
		 Kout & operator<<( const Kfmt           & x );
	
		Stream & stream;
		
	private:
		bool         mArmed;
		int          mFloatPrecision;
		char         mQuoteChar;
		const char * mClosingString;
};
#define KOUT  Kout( Serial )

#define KTIME( X ) { unsigned long _t0 = micros(); X; KOUT << micros() - _t0 << "us elapsed for  " << #X; }

#endif // __Keyhole_H__
