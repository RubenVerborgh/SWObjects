// $Id: TrigScanner.hpp,v 1.3 2008-10-03 07:06:03 eric Exp $

#ifndef TrigScanner_H
#define TrigScanner_H

// Flex expects the signature of yylex to be defined in the macro YY_DECL, and
// the C++ parser expects it to be declared. We can factor both as follows.

#ifndef YY_DECL

#define	YY_DECL						\
    w3c_sw::TrigParser::token_type				\
    w3c_sw::TrigScanner::lex(				\
	w3c_sw::TrigParser::semantic_type* yylval,		\
	w3c_sw::TrigParser::location_type* yylloc		\
    )
#endif

#ifndef __FLEX_LEXER_H
#define yyFlexLexer TrigFlexLexer
#include "FlexLexer.h"
#undef yyFlexLexer
#endif

#include "TrigParser.hpp"

namespace w3c_sw {

/** TrigScanner is a derived class to add some extra function to the scanner
 * class. Flex itself creates a class named yyFlexLexer, which is renamed using
 * macros to TrigFlexLexer. However we change the context of the generated
 * yylex() function to be contained within the TrigScanner class. This is required
 * because the yylex() defined in TrigFlexLexer has no parameters. */
class TrigScanner : public TrigFlexLexer
{
private:
    TrigDriver* driver;
public:
    /** Create a new scanner object. The streams arg_yyin and arg_yyout default
     * to cin and cout, but that assignment is only made when initializing in
     * yylex(). */
    TrigScanner(TrigDriver* driver, std::istream* arg_yyin = 0,
	    std::ostream* arg_yyout = 0);

    /** Required for virtual functions */
    virtual ~TrigScanner();

    /** This is the main lexing function. It is generated by flex according to
     * the macro declaration YY_DECL above. The generated bison parser then
     * calls this virtual function to fetch new tokens. */
    virtual TrigParser::token_type lex(
	TrigParser::semantic_type* yylval,
	TrigParser::location_type* yylloc
	);
    TrigParser::token_type lexWrapper(
	   TrigParser::semantic_type* yylval,
	   TrigParser::location_type* yylloc
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

protected:
    TrigParser::token_type typedLiteral (TrigParser::semantic_type*& yylval, TrigParser::token_type tok) {
	std::istringstream is(yytext);
	std::ostringstream normalized;

	switch (tok) {
	case w3c_sw::TrigParser::token::INTEGER:
	    int i;
	    is >> i;
	    normalized << i;
	    yylval->p_NumericRDFLiteral = driver->getNumericRDFLiteral(yytext, i);
	    return tok;
	case w3c_sw::TrigParser::token::DECIMAL:
	    float f;
	    is >> f;
	    normalized << f;
	    yylval->p_NumericRDFLiteral = driver->getNumericRDFLiteral(yytext, f);
	    return tok;
	case w3c_sw::TrigParser::token::DOUBLE:
	    double d;
	    is >> d;
	    normalized << d;
	    yylval->p_NumericRDFLiteral = driver->getNumericRDFLiteral(yytext, d);
	    return tok;
	default: throw(new std::exception());
	}
    }

    TrigParser::token_type unescapeString (TrigParser::semantic_type*& yylval, TrigParser::location_type* yylloc, const char* text, size_t len, TrigParser::token_type tok) {
	std::string* space = new std::string;
	YaccDriver::unescapeString(text, len, space, yylloc);
	yylval->p_string = space;
	return tok;
    }

    /** resolvePrefix
     * Assumes barewords are localnames in the default namespace.
     * This leaves it to the lexer to warn about missing prefixes.
     */
    template <typename T> // e.g. TrigParser::location_type
    const URI* resolvePrefix (const char* yytext, T* yylloc) {
	std::string stripped;
	YaccDriver::unescapeReserved(yytext, ::strlen(yytext), &stripped, yylloc);

	size_t index = stripped.find(':');
	if (index == std::string::npos)
	    // throw(std::runtime_error("Inexplicable lack of ':' in prefix"));
	    index = 0;
	std::string prefix = stripped.substr(0, index);
	const URI* nspace = driver->getNamespace(prefix, true);
	if (nspace == NULL)
	    nspace = driver->prefixError(prefix, *yylloc);
	stripped.replace(0, index+1, nspace->getLexicalValue());

	return driver->getAbsoluteURI(stripped.c_str());
    }

    template <typename T> // e.g. TrigParser::location_type
    const URI* unescapeAndResolveBase (const char* p_rel, size_t len, T* yylloc) {
	std::string stripped;
	try {
	    YaccDriver::unescapeNumeric(p_rel, len, &stripped, yylloc,
					AtomFactory::validate & AtomFactory::VALIDATE_IRIcharacters
					? &CharacterRange::NonIRI
					: NULL);
#if 0
	    /*
	      In principle, something like this could be done to pass this back
	      to the parser to make sure that the unescaped value complies with
	      IRIREF, but the overhead of creating another driver and scanner
	      and adding some way to limit recursion aren't worth the mild
	      cleverness.
	     */
	    std::istringstream is("<" + stripped + ">");
	    TrigDriver parser2(stripped, driver->atomFactory);
	    TrigScanner s(&parser2, &is);
	    TrigParser::semantic_type yylval2;
	    TrigParser::location_type yylloc2;
	    TrigParser::token_type yychar = s.lexWrapper(&yylval2, &yylloc2);
	    if (yychar != TrigParser::token::IRIREF)
		throw;
#endif
	} catch (std::runtime_error& e) {
	    driver->error(*yylloc, e.what());
	}
	try {
	    return driver->getAbsoluteURI(stripped.c_str());
	} catch (std::runtime_error& e) {
	    driver->error(*yylloc, e.what());

	    // We're here because we've noted this error and are
	    // continuing. We turn off validation so we can get this
	    // IRI from the AtomFactory and then restore the original
	    // state.
	    disableValidation();
	    const URI* ret = driver->getAbsoluteURI(stripped.c_str());
	    restoreValidation();
	    return ret;
	}
    }

    template <typename T> // e.g. TrigParser::location_type
    void scanError (const char* msg, char quote, T* yylloc) {
	std::stringstream ss;
	ss << "Malformed " << msg << " " << quote << yytext << quote;
	driver->error(*yylloc, ss.str());
    }

    AtomFactory::e_Validation validation;
    void disableValidation () {
	validation = AtomFactory::validate;
	AtomFactory::validate = static_cast<AtomFactory::e_Validation>
	    // enums become ints when you & them so cast back to enum.
	    (AtomFactory::validate&~AtomFactory::VALIDATE_IRIcharacters);
    }
    void restoreValidation () {
	    AtomFactory::validate = validation;	
    }

};

} // namespace w3c_sw

#endif // TrigScanner_H
