/* perform DAWG tests.
 * call from: ..
 * files: <tests>/data-r2  -- from http://www.w3.org/2001/sw/DataAccess/tests/data-r2/
 *
 * $Id: test_GraphMatch.cpp,v 1.5 2008-12-04 22:37:09 eric Exp $
 */

#define BOOST_TEST_DYN_LINK 1
#define BOOST_TEST_MODULE DAWG_tests
#include <boost/test/unit_test.hpp>
//#pragma warning( disable : 4273 )
//#include <boost/test/included/unit_test.hpp>

#include <fstream>
#include <iterator>
#include "SWObjects.hpp"
#include "SPARQLfedParser.hpp"
#include "TurtleSParser.hpp"
#include "RdfDB.hpp"
#include "ResultSet.hpp"
#include "SPARQLSerializer.hpp"

#if XML_PARSER == SWOb_LIBXML2
  #include "../interface/SAXparser_libxml.hpp"
#elif XML_PARSER == SWOb_EXPAT1
  #include "../interface/SAXparser_expat.hpp"
#else
  #warning DAWG tests require an XML parser
#endif

using namespace w3c_sw;

POSFactory f;
SPARQLfedDriver sparqlParser("", &f);
#ifndef _MSC_VER
    /* @@@ Temporary work-around for a build bug in MSVC++ where TurltSDriver
     *     isn't defined by including TurtleSParser/TurtleSParser.hpp .
     */
TurtleSDriver turtleParser("", &f);
#endif /* !_MSC_VER */

/* Sentinal to mark end of arrays of files: */
const char* Sentinel = "sentinel";

std::string readFile (const char* filename, const char* type) {
    std::ifstream dataStream(filename);
    if (!dataStream.is_open()) {
	std::string msg = std::string("failed to ") + std::string(type) + 
	    std::string(" data \"") + filename + std::string("\".");
	throw msg;
    }
    std::istreambuf_iterator<char> i(dataStream), e;
    std::string ret(i, e);
    dataStream.close();
    return ret;
}

void queryTest (const char* defGraphs[], const char* namGraphs[], 
		const char* queryFile, const char* resultsFile) {
 
    /* Parse query. */
    if (sparqlParser.parse_file(queryFile)) {
	std::string msg = std::string("failed to parse query \"") + 
	    queryFile + std::string("\".");
	throw msg;
    }

    /* Parse data. */
    RdfDB d;
    for (size_t i = 0; defGraphs[i] != Sentinel; ++i) {
#ifdef _MSC_VER
    /* @@@ Temporary work-around for a build bug in MSVC++ where TurltSDriver
     *     isn't defined by including TurtleSParser/TurtleSParser.hpp .
     */
	loadGraph(d.assureGraph(NULL), &f, "text/turtle", "", defGraphs[i]);
#else /* !_MSC_VER */
 	turtleParser.setGraph(d.assureGraph(NULL));
 	turtleParser.parse_file(defGraphs[i]);
#endif /* !_MSC_VER */
    }
    for (size_t i = 0; namGraphs[i] != Sentinel; ++i) {
#ifdef _MSC_VER
    /* @@@ Temporary work-around for a build bug in MSVC++ where TurltSDriver
     *     isn't defined by including TurtleSParser/TurtleSParser.hpp .
     */
	loadGraph(d.assureGraph(f.getURI(namGraphs[i])), &f, "text/turtle", "", defGraphs[i]);
#else /* !_MSC_VER */
 	turtleParser.setGraph(d.assureGraph(f.getURI(namGraphs[i])));
 	turtleParser.parse_file(defGraphs[i]);
#endif /* !_MSC_VER */
    }
    std::cout << "Database: " << d;

    /* Exectute query. */
    ResultSet got(&f);
    sparqlParser.root->execute(&d, &got);
    std::cout << "query: " << *sparqlParser.root;
    std::cout << "got: " << got;

    /* Compare to expected results. */
    if (!::strncmp(resultsFile + ::strlen(resultsFile) - 4, ".srx", 4)) {
#if XML_PARSER == SWOb_DISABLED
	throw "XML parser needed for parsing srx files";
#else /* !XML_PARSER == SWOb_DISABLED */
	SWSAXparser* p = SWSAXparser::makeSAXparser();
	ResultSet expected(&f, p, resultsFile);
	delete p;
	std::cout << "expected: " << expected;
	BOOST_CHECK_EQUAL(got, expected);
#endif /* !XML_PARSER == SWOb_DISABLED */
    } else {
	ResultSet expected(&f, readFile(resultsFile, "results"), false);
	std::cout << "expected: " << expected;
	BOOST_CHECK_EQUAL(got, expected);
    }
}

/* Macros for terse test syntax: */
#define QTEST(Q, R) \
    queryTest(defaultGraphs, namedGraphs, Q, R);
#define S Sentinel

BOOST_AUTO_TEST_CASE( basic_Basic_Var_1 ) {
    const char* defaultGraphs[] = { "tests/data-r2/basic/data-5.ttl", S };
    const char* namedGraphs[] = { S };
    QTEST("tests/data-r2/basic/var-1.rq", "tests/data-r2/basic/var-1.srx");
}

// EOF
