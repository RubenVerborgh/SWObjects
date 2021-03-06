/* test_SADI.hpp -- 
 *
 * Test the SADI server built in to ../bin/sparql and SADI interactions built
 * into the SPARQL language.
 *
 * $Id: test_Sadi.cpp,v 1.5 2008-12-04 22:37:09 eric Exp $
 */

#define BOOST_TEST_MODULE SADI

#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>
#include <boost/lexical_cast.hpp>
#define NEEDDEF_W3C_SW_SAXPARSER
#define NEEDDEF_W3C_SW_WEBAGENT
#include "SWObjects.hpp"
#include "ServerInteraction.hpp"

#if HTTP_CLIENT != SWOb_DISABLED
  W3C_SW_WEBAGENT<> WebClient;
#else /* HTTP_CLIENT == SWOb_DISABLED */
  #warning unable to test RDFa over HTTP
#endif /* HTTP_CLIENT == SWOb_DISABLED */

#if XML_PARSER != SWOb_DISABLED
  W3C_SW_SAXPARSER P;
#else
  #error RDFa tests require an XML parser
#endif

/* Keep all inclusions of boost *after* the inclusion of SWObjects.hpp
 * (or define BOOST_*_DYN_LINK manually).
 */
#include <boost/test/unit_test.hpp>
w3c_sw_PREPARE_TEST_LOGGER("--log"); // invoke with e.g. "--log *:-1,IO,GraphMatch:3"
w3c_sw_DEBUGGING_FUNCTIONS();

w3c_sw::AtomFactory F;

// Allocate distinct server port ranges to prevent conflicts in simultaneous tests.
// test_SPARQL: 9000-90ff, test_SADI: 9100-91ff, test_LWP: 9200-92ff, test_SPARQL11: 0x9300-0x93ff
#define LOWPORT 0x9100
#define HIPORT  0x91ff

/** CurlPOSTtoSADIservice - invoke curl with parameters used in the server
 *  invocation.
 */
struct CurlPOSTtoSADIservice : w3c_sw::ClientServerInteraction {
    CurlPOSTtoSADIservice (std::string serverParams, const char* data, const char* media)
	: w3c_sw::ClientServerInteraction(serverParams, "/SADI", LOWPORT, HIPORT)
    {
	invoke(std::string()
	       + "curl -s -X POST -H 'Content-Type: " + media + "' " + serverURL + "  -d '" + data + "'");
    }
};

struct OperationOnSADIServer : w3c_sw::OperationOnInvokedServer {
    OperationOnSADIServer (std::string serverParams, std::string query, std::string expect)
	: w3c_sw::OperationOnInvokedServer(&F, &WebClient, &P, serverParams, "/SADI",
					   LOWPORT, HIPORT, query, expect)
    {  }
};

/** OperationOnRemoteServer - client interactions with the server built into
 *  the bin/sparql binary.
 */
struct OperationOnRemoteServer {
    w3c_sw::EvaluatedResultSet got;
    w3c_sw::ParsedResultSet expected;

    OperationOnRemoteServer (std::string query, std::string expect)
	: got(&F, &WebClient, &P, query),
	  expected(&F, expect)
    {  }
};


BOOST_AUTO_TEST_SUITE( local )
BOOST_AUTO_TEST_CASE( curl1to1 ) {
    CurlPOSTtoSADIservice i
	(// Server invocation -- construct a pattern from supplied graph.
	 // (A real SADI rule should include body data in the head.)
	 "--SADI 'CONSTRUCT { ?s <tag:eric@w3.org/2012/p2> \"X\" }\n"
	 "            WHERE { ?s <tag:eric@w3.org/2012/p1> ?o }' --stop-after 1",

	 // Curl this data and media type to verify the server response.
	 "<s1> <tag:eric@w3.org/2012/p1> <ooo> .", "text/turtle");
    BOOST_CHECK_EQUAL("<s1> <tag:eric@w3.org/2012/p2> \"X\" .\n", i.clientS);
}

#ifdef INVOKED_SADI
BOOST_AUTO_TEST_CASE( invoked1 ) {
    OperationOnSADIServer i
	(// Server invocation -- construct a pattern from supplied graph.
	 // (A real SADI rule should include body data in the head.)
	 "--SADI 'CONSTRUCT { ?s <tag:eric@w3.org/2012/p2> \"X\" }\n"
	 "            WHERE { ?s <tag:eric@w3.org/2012/p1> ?o }' --stop-after 1",

	 // Client SPARQL operation to invoke the SADI service.
	 //   bind ?s
	 "+--------------------------+\n"
	 "| ?s                       |\n"
	 "| <tag:eric@w3.org/2012/S> |\n"
	 "+--------------------------+\n"
	 //   ask for the s2 return from the SADI service (should be <S>)
	 "SELECT ?s2 ?x\n"
	 " WHERE {\n"
	 //     from the server invoked above.
	 "    SADI <http://localhost:%p/SADI>\n"
	 //       sending { <S> <tag:eric@w3.org/2012/p1> <ooo> }
	 "        FROM { ?s <tag:eric@w3.org/2012/p1> <ooo> }\n"
	 //       and getting back { <S> <tag:eric@w3.org/2012/p2> \"X\" }
	 "        WHERE { ?s2 <tag:eric@w3.org/2012/p2> ?x }\n"
	 "}",
	 "+--------------------------+-----+\n"
	 "| ?s2                      | ?x  |\n"
	 "| <tag:eric@w3.org/2012/S> | 'X' |\n"
	 "+--------------------------+-----+");
    BOOST_CHECK_EQUAL(i.got, i.expected);
}
#endif /* INVOKED_SADI */
BOOST_AUTO_TEST_SUITE_END(/* local */)


BOOST_AUTO_TEST_SUITE( remote )
#ifdef REMOTE_SADI
BOOST_AUTO_TEST_CASE( hello ) {
    OperationOnRemoteServer i
	("SELECT ?greeting\n"
	 "WHERE {\n"
	 "  SADI <http://sadiframework.org/examples/hello>\n"
	 "  FROM {\n"
	 "    <http://example.com/1> a <http://sadiframework.org/examples/hello.owl#NamedIndividual> ;\n"
	 "        <http://xmlns.com/foaf/0.1/name> 'Guy Incognito' .\n"
	 "  } WHERE {\n"
	 "    <http://example.com/1> <http://sadiframework.org/examples/hello.owl#greeting> ?greeting .\n"
	 "  }\n"
	 "}",
	 " +-------------------------+\n"
	 " | ?greeting               |\n"
	 " | 'Hello, Guy Incognito!'^^<http://www.w3.org/2001/XMLSchema#string> |\n"
	 " +-------------------------+");
    BOOST_CHECK_EQUAL(i.got, i.expected);
}

/* Can't precisely compare floats or doubles so do to tests, once with integer
 * values and once with an approximate test of the bmi of the 0th row:
 */
BOOST_AUTO_TEST_CASE( bmi_int ) {
    OperationOnRemoteServer i
	("PREFIX bmi: <http://sadiframework.org/examples/bmi.owl#>\n"
	 "PREFIX xsd: <http://www.w3.org/2001/XMLSchema#>\n"
	 "SELECT (xsd:integer(?bmi) AS ?i)\n"
	 "WHERE {\n"
	 "  SADI <http://sadiframework.org/examples/simpleBMI>\n"
	 "  FROM {\n"
	 "    <http://example.com/1> a bmi:SimpleInputClass ;\n"
	 "      bmi:height_m 1.8796 ;\n"
	 "      bmi:weight_kg 92.9864359 .\n"
	 "  } WHERE {\n"
	 "    <http://example.com/1> bmi:BMI ?bmi .\n"
	 "  }\n"
	 "}\n",
	 " +----+\n"
	 " | ?i |\n"
	 " | 26 |\n"
	 " +----+");
    BOOST_CHECK_EQUAL(i.got, i.expected);
}
BOOST_AUTO_TEST_CASE( bmi ) {
    EvaluatedResultSet got
	("PREFIX bmi: <http://sadiframework.org/examples/bmi.owl#>\n"
	 "SELECT ?bmi\n"
	 "WHERE {\n"
	 "  SADI <http://sadiframework.org/examples/simpleBMI>\n"
	 "  FROM {\n"
	 "    <http://example.com/1> a bmi:SimpleInputClass ;\n"
	 "      bmi:height_m 1.8796 ;\n"
	 "      bmi:weight_kg 92.9864359 .\n"
	 "  } WHERE {\n"
	 "    <http://example.com/1> bmi:BMI ?bmi .\n"
	 "  }\n"
	 "}\n");
    BOOST_CHECK_CLOSE(got[0]["bmi"].getDouble(), 26.32, 0.01);
}

BOOST_AUTO_TEST_CASE( drugs ) {
    OperationOnRemoteServer i
    ("PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>\n"
     "PREFIX str: <http://nlp2rdf.lod2.eu/schema/string/>\n"
        "PREFIX scms: <http://ns.aksw.org/scms/>\n"
        "PREFIX nlp: <http://sadiframework.org/services/nlp/nlp.owl#>\n"
     "SELECT ?matchingText ?drugName ?drug\n"
     "WHERE {\n"
     "  SADI <http://sadiframework.org/services/nlp/identifyDrugs>\n"
     "  FROM {\n"
     "    <http://example.com/1> a nlp:Document ;\n"
     "      str:sourceString \"Theo-Dur is a brand name of Theophylline\" .\n"
     "  } WHERE {\n"
     "    <http://example.com/1> str:subString ?match .\n"
     "    ?match str:anchorOf ?matchingText .\n"
     "    ?match scms:means ?drug .\n"
     "    ?drug rdfs:label ?drugName\n"
     "  }\n"
     "}",
        " +----------------+----------------+----------------------------------------+\n"
        " | ?matchingText  | ?drugName      | ?drug                                  |\n"
        " | 'Theo-Dur'     | 'Theophylline' | <http://www.drugbank.ca/drugs/DB00277> |\n"
        " | 'Theophylline' | 'Theophylline' | <http://www.drugbank.ca/drugs/DB00277> |\n"
        " +----------------+----------------+----------------------------------------+");
    BOOST_CHECK_EQUAL(i.got, i.expected);
}

#endif /* REMOTE_SADI */
BOOST_AUTO_TEST_SUITE_END(/* remote */)

