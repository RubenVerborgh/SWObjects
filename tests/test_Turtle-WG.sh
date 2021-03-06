#!/bin/sh

now=$(./bin/sparql -e 'SELECT NOW() {}' -l csv | tail -1)
cat <<EOF
# EARL report by test_Turtle-WG.sh
# $0 $*

@prefix doap: <http://usefulinc.com/ns/doap#>.
@prefix earl: <http://www.w3.org/ns/earl#>.
@prefix foaf: <http://xmlns.com/foaf/0.1/>.
@prefix swobj: <http://swobjects.org/>.
@prefix test: <http://swobjects.org/test_Turtle-WG.sh>.
@prefix dct: <http://purl.org/dc/terms/>.
@prefix xsd: <http://www.w3.org/2001/XMLSchema#>.
@prefix t: <http://www.w3.org/2013/TurtleTests/manifest.ttl#>.

<> foaf:primaryTopic swobj:sparql ;
	dct:issued "$now"^^xsd:dateTime ;
	foaf:maker <http://www.w3.org/People/Eric/ericP-foaf#ericP> .

swobj:sparql
	a doap:Project ;
	doap:name "SWObjects" ;
	doap:homepage <http://swobjects.org/> ;
	doap:developer <http://www.w3.org/People/Eric/ericP-foaf#ericP> .

<http://www.w3.org/People/Eric/ericP-foaf#ericP> a foaf:Person ;
	foaf:name "Eric Prud'hommeaux" ;
	foaf:mbox <mailto:eric@w3.org> ;
	foaf:mbox_sha1sum "e2d67791b2a0ce3441c0c770f94daa130b4e6d95" ;
	foaf:homepage <http://www.w3.org/People/Eric/> .

EOF

# Running locally
BASE="'-b ' REPLACE(STR(?ttl), '../../WWW/2013/TurtleTests/', 'http://www.w3.org/2013/TurtleTests/')"
FROM=../../WWW/2013/TurtleTests/manifest.ttl

#Running across the web.
#BASE=
#FROM=http://www.w3.org/2013/TurtleTests/manifest.ttl

WORKY="'echo \"[] a earl:Assertion ; earl:test t:' StrAfter(STR(?entry), 'manifest.ttl#') ' ; earl:subject swobj:sparql ; earl:assertedBy test: ; earl:result [ earl:outcome earl:passed; dct:date \'' NOW() '\'^^xsd:dateTime ] .\" '"
FAIL="'echo fail ' ?name"

./bin/sparql -d $FROM -e "
  PREFIX mf: <http://www.w3.org/2001/sw/DataAccess/tests/test-manifest#>
  PREFIX rdft: <http://www.w3.org/ns/rdftest#>
  SELECT './bin/sparql -q ' $BASE ' -d ' ?ttl ' --compare ' ?nt
         ' && ' $WORKY ' || ' $FAIL '\n'
  WHERE {
    ?l mf:entries MEMBERS(?entry) .
    ?entry a rdft:TestTurtleEval ;
           mf:action ?ttl ;
           mf:result ?nt ;
           mf:name ?name
  }" -L text/raw | bash

./bin/sparql -d $FROM -e "
  PREFIX mf: <http://www.w3.org/2001/sw/DataAccess/tests/test-manifest#>
  PREFIX rdft: <http://www.w3.org/ns/rdftest#>
  SELECT './bin/sparql -q ' $BASE ' -d ' ?ttl
         ' && ' $WORKY ' || ' $FAIL '\n'
  WHERE {
    ?l mf:entries MEMBERS(?entry) .
    ?entry a rdft:TestTurtlePositiveSyntax ;
           mf:action ?ttl ;
           mf:name ?name
  }" -L text/raw | bash

./bin/sparql -d $FROM -e "
  PREFIX mf: <http://www.w3.org/2001/sw/DataAccess/tests/test-manifest#>
  PREFIX rdft: <http://www.w3.org/ns/rdftest#>
  SELECT './bin/sparql -q -d ' ?ttl
         ' && ' $FAIL ' || ' $WORKY '\n'
  WHERE {
    ?l mf:entries MEMBERS(?entry) .
    ?entry a rdft:TestTurtleNegativeSyntax ;
           mf:action ?ttl ;
           mf:name ?name
  }" -L text/raw | bash

./bin/sparql -d $FROM -e "
  PREFIX mf: <http://www.w3.org/2001/sw/DataAccess/tests/test-manifest#>
  PREFIX rdft: <http://www.w3.org/ns/rdftest#>
  SELECT './bin/sparql --validation iricharacters -q -d ' ?ttl
         ' && ' $FAIL ' || ' $WORKY '\n'
  WHERE {
    ?l mf:entries MEMBERS(?entry) .
    ?entry a rdft:TestTurtleNegativeEval ;
           mf:action ?ttl ;
           mf:name ?name
  }" -L text/raw | bash

