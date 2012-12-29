// $Id: TurtleSScanner.hpp,v 1.3 2008-10-03 07:06:03 eric Exp $

#ifndef TurtleSScanner_H
#define TurtleSScanner_H

// Flex expects the signature of yylex to be defined in the macro YY_DECL, and
// the C++ parser expects it to be declared. We can factor both as follows.

#ifndef YY_DECL

#define	YY_DECL						\
    w3c_sw::TurtleSParser::token_type				\
    w3c_sw::TurtleSScanner::lex(				\
	w3c_sw::TurtleSParser::semantic_type* yylval,		\
	w3c_sw::TurtleSParser::location_type* yylloc		\
    )
#endif

#ifndef __FLEX_LEXER_H
#define yyFlexLexer TurtleSFlexLexer
#include "FlexLexer.h"
#undef yyFlexLexer
#endif

#include "TurtleSParser/TurtleSParser.hpp"

namespace w3c_sw {

/** TurtleSScanner is a derived class to add some extra function to the scanner
 * class. Flex itself creates a class named yyFlexLexer, which is renamed using
 * macros to TurtleSFlexLexer. However we change the context of the generated
 * yylex() function to be contained within the TurtleSScanner class. This is required
 * because the yylex() defined in TurtleSFlexLexer has no parameters. */
class TurtleSScanner : public TurtleSFlexLexer
{
private:
    TurtleSDriver* driver;
public:
    /** Create a new scanner object. The streams arg_yyin and arg_yyout default
     * to cin and cout, but that assignment is only made when initializing in
     * yylex(). */
    TurtleSScanner (TurtleSDriver* driver, std::istream* in = NULL, std::ostream* out = NULL)
	: TurtleSFlexLexer(in, out), driver(driver)
    {  }

    /** Required for virtual functions */
    virtual ~TurtleSScanner () {  }

    /** This is the main lexing function. It is generated by flex according to
     * the macro declaration YY_DECL above. The generated bison parser then
     * calls this virtual function to fetch new tokens. */
    virtual TurtleSParser::token_type lex(
	TurtleSParser::semantic_type* yylval,
	TurtleSParser::location_type* yylloc
	);
    TurtleSParser::token_type lexWrapper(
	   TurtleSParser::semantic_type* yylval,
	   TurtleSParser::location_type* yylloc
					   ) {
	try {
	    return lex(yylval, yylloc);
	} catch (const char* e) {
	    std::stringstream s;
	    s << *yylloc << ": " << e;
	    throw s.str();
	}
    }

    /** Enable debug output (via arg_yyout) if compiled into the scanner. */
    void set_debug (bool b) {
	yy_flex_debug = b;
    }

    TurtleSParser::token_type typedLiteral (TurtleSParser::semantic_type*& yylval, TurtleSParser::token_type tok) {
	std::istringstream is(yytext);
	std::ostringstream normalized;

	switch (tok) {
	case w3c_sw::TurtleSParser::token::INTEGER:
	    int i;
	    is >> i;
	    normalized << i;
	    yylval->p_NumericRDFLiteral = driver->getNumericRDFLiteral(yytext, i);
	    return tok;
	case w3c_sw::TurtleSParser::token::DECIMAL:
	    float f;
	    is >> f;
	    normalized << f;
	    yylval->p_NumericRDFLiteral = driver->getNumericRDFLiteral(yytext, f);
	    return tok;
	case w3c_sw::TurtleSParser::token::DOUBLE:
	    double d;
	    is >> d;
	    normalized << d;
	    yylval->p_NumericRDFLiteral = driver->getNumericRDFLiteral(yytext, d);
	    return tok;
	default: throw(new std::exception());
	}
    }

    TurtleSParser::token_type unescapeString (TurtleSParser::semantic_type*& yylval, TurtleSParser::location_type* yylloc, size_t skip, TurtleSParser::token_type tok) {
	std::string* space = new std::string;
	YaccDriver::unescapeString(yytext+skip, yyleng-skip-skip, space, yylloc);
	yylval->p_string = space;
	return tok;
    }

    const URI* resolvePrefix (const char* yytext, TurtleSParser::location_type* yylloc) {
	std::string stripped;
	YaccDriver::unescapeReserved(yytext, ::strlen(yytext), &stripped, yylloc);

	size_t index = stripped.find(':');
	if (index == std::string::npos)
	    throw(std::runtime_error("Inexplicable lack of ':' in prefix"));
	const URI* nspace = driver->getNamespace(stripped.substr(0, index));
	if (nspace == NULL) {
	    std::stringstream err;
	    err << "Unknown prefix: \"" << stripped.substr(0, index) << "\"";
	    throw(std::runtime_error(err.str()));
	}
	stripped.replace(0, index+1, nspace->getLexicalValue());

	return driver->getAbsoluteURI(stripped.c_str());
    }

    const URI* unescapeAndResolveBase (const char* p_rel, size_t len, TurtleSParser::location_type* yylloc) {
	std::string stripped;
	YaccDriver::unescapeNumeric(p_rel, len, &stripped, yylloc);
	return driver->getAbsoluteURI(stripped.c_str());
    }
};

} // namespace w3c_sw

#endif // TurtleSScanner_H
