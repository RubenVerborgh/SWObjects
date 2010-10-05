/* RdfDB - sets of variable bindings and their proofs.
 * $Id: RdfDB.cpp,v 1.2 2008-08-27 02:21:41 eric Exp $
 */

#include "RdfDB.hpp"
#include "ResultSet.hpp"
#include "TurtleSParser/TurtleSParser.hpp"
#include "TrigSParser/TrigSParser.hpp"
#include <boost/iostreams/stream.hpp>

namespace w3c_sw {

    size_t RdfDB::DebugEnumerateLimit = 50;

    RdfDB::HandlerSet RdfDB::defaultHandler;

    RdfDB::~RdfDB () {
	for (graphmap_type::const_iterator it = graphs.begin();
	     it != graphs.end(); it++)
	    delete it->second;
    }

    void RdfDB::clearTriples () {
	for (graphmap_type::const_iterator it = graphs.begin();
	     it != graphs.end(); it++)
	    it->second->clearTriples();
    }

    BasicGraphPattern* RdfDB::ensureGraph (const TTerm* name) {
	if (name == NULL)
	    name = DefaultGraph;
	graphmap_type::const_iterator vi = graphs.find(name);
	if (vi == graphs.end()) {
	    BasicGraphPattern* ret;
	    if (name == DefaultGraph)
		ret = new DefaultGraphPattern();
	    else
		ret = new NamedGraphPattern(name);
	    graphs[name] = ret;
	    return ret;
	} else {
	    return vi->second;
	}
    }

    bool RdfDB::loadData (BasicGraphPattern* target, IStreamContext& istrP, std::string nameStr, std::string baseURI, AtomFactory* atomFactory, NamespaceMap* nsMap) {
	w3c_sw::StreamRewinder rb(*istrP);
	boost::iostreams::stream_buffer<w3c_sw::StreamRewinder::Device> srsb(rb.device); // ## debug with small buffer size, e.g. 4
	std::istream is(&srsb);
	IStreamContext istr(istrP.nameStr, is, 
			    istrP.mediaType.is_initialized() ? istrP.mediaType.get().c_str() : NULL);
	try {
	    if (istr.mediaType.match("text/html") || 
		istr.mediaType.match("application/xhtml")) {
		if (xmlParser == NULL)
		    throw std::string("no XML parser to parse ")
			+ istr.mediaType.toString()
			+ " document " + nameStr;
		RDFaParser parser(nameStr, atomFactory, xmlParser);
		if (baseURI != "")
		    parser.setBase(baseURI);
		if (nsMap != NULL)
		    parser.setNamespaceMap(nsMap);
		return parser.parse(target, istr);
	    } else if (istr.mediaType.match("text/rdf") || 
		       istr.mediaType.match("application/rdf+xml")) {
		if (xmlParser == NULL)
		    throw std::string("no XML parser to parse ")
			+ istr.mediaType.toString()
			+ " document " + nameStr;
		RdfXmlParser parser(nameStr, atomFactory, xmlParser);
		if (baseURI != "")
		    parser.setBase(baseURI);
		if (nsMap != NULL)
		    parser.setNamespaceMap(nsMap);
		return parser.parse(target, istr);
	    } else if (istr.mediaType.match("text/turtle") || 
		       istr.mediaType.match("text/ntriples")) {
		TurtleSDriver parser(nameStr, atomFactory);
		if (baseURI != "")
		    parser.setBase(baseURI);
		if (nsMap != NULL)
		    parser.setNamespaceMap(nsMap);
		parser.parse(istr, target);
		return false;
	    } else {
		TrigSDriver parser(nameStr, atomFactory);
		if (baseURI != "")
		    parser.setBase(baseURI);
		if (nsMap != NULL)
		    parser.setNamespaceMap(nsMap);
		parser.parse(istr, this, target);
		return false;
	    }
	} catch (ChangeMediaTypeException& e) {
	    rb.replay();
	    boost::iostreams::stream_buffer<w3c_sw::StreamRewinder::Device> sb2(rb.device);
	    std::istream is2(&sb2);
	    IStreamContext again(istrP.nameStr, is, e.mediaType.c_str());
	    return handler->parse(e.mediaType, e.args, target, again, nameStr, baseURI, atomFactory, nsMap);
	}
    }

    DefaultGraphClass defaultGraphInst;
    TTerm* DefaultGraph = &defaultGraphInst;

    void RdfDB::bindVariables (ResultSet* rs, const TTerm* graph, const BasicGraphPattern* toMatch) {
	if (graph == NULL) graph = DefaultGraph;
	graphmap_type::const_iterator vi;
	size_t matched = 0;

	/* Look in each candidate graph. */
	if (graph->isConstant()) {
	    if ((vi = graphs.find(graph)) != graphs.end()) {
		vi->second->bindVariables(rs, toMatch, graph, vi->first);
		++matched;
	    }
	} else {
	    ResultSet island(rs->getAtomFactory());
	    delete *(island.begin());
	    island.erase(island.begin());
	    /* Multi-graph match algorithm:
	     * for each graph
	     *     BasicGraphPattern::bindVariables(... ?graph, foundGraph)
	     *         for each rs row...
	     * 
	     * The lack of SPOG index means we always iterate across all graphs.
	     * It could be cheaper to iterate across rows in the result set and
	     * only iterate across graphs if ?graph is unbound. Could decide by:
	     *   rs->first()->get(?graph) == TTerm::Unbound
	     */
	    for (vi = graphs.begin(); vi != graphs.end(); vi++)
		if (vi->first != DefaultGraph) {
		    ResultSet disjoint(rs->getAtomFactory());
		    vi->second->bindVariables(&disjoint, toMatch, graph, vi->first);
		    for (ResultSetIterator row = disjoint.begin() ; row != disjoint.end(); ) {
			island.insert(island.end(), (*row)->duplicate(&island, island.end()));
			delete *row;
			row = disjoint.erase(row);
		    }
		    ++matched;
		}
	    rs->joinIn(&island, false);
	}
	if (matched == 0)
	    for (ResultSetIterator it = rs->begin(); it != rs->end(); ) {
		delete *it;
		it = rs->erase(it);
	    }
    }

    void RdfDB::express (Expressor* expressor) const {
	for (graphmap_type::const_iterator it = graphs.begin();
	     it != graphs.end(); it++)
	    it->second->express(expressor);
    }

} // namespace w3c_sw

