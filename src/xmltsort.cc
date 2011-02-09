/*
 * © Copyright 2011 Carl N. Baldwin
 *
 * Confidential computer software. Valid license from Carl Baldwin required for
 * possession, use or copying.
 */
#include <unistd.h>
#include <map>
#include <cstring>

#include "crawl-with-fork.h"
#include "xml-graph.h"
#include "stream-dfs.h"

// Assumptions:
//
//  * The input XML file only has one element with any given name matching the
//    vertex expression.

template<class Ch, class Tr = std::char_traits<Ch> >
class basic_tsorter : public basic_marcher<Ch, Tr>, public strmQueueInterface<XmlGraph> {
  public:
    typedef basic_marcher<Ch,Tr>           parent;
    typedef boost::StreamDfs< XmlGraph >   graph_t;
    typedef boost::graph_traits< graph_t > g_traits;

    basic_tsorter( std::istream &instream,
                   const char *expression,
                   const char *srcexpr,
                   const char *targetexpr,
                   const char **argv,
                   bool allatonce,
                   std::ostream *errorstream = NULL )
      : parent( instream, expression, argv, allatonce ),
        _srcexpr( srcexpr ),
        _targetexpr( targetexpr ),
        _errorstream( errorstream )
      {
        _graph.strm_q = this;
      }

    void run() {
      for( auto iters = boost::vertices( _graph ); iters.first != iters.second; ++iters.first ) {
        xmlDocPtr doc = boost::get( boost::get( docptr_t(), _graph ), *iters.first );
        dumpNode( xmlDocGetRootElement( doc ), std::cout );
      }
    }

    bool empty() {
      replenish_q();
      return q.empty();
    }

    graph_t::vertex_descriptor &front() {
      replenish_q();
      return q.front();
    }

    void pop() { q.pop(); }

    void replenish_q() {
      while( not basic_marcher<Ch,Tr>::finished() and q.empty() )
        basic_marcher<Ch,Tr>::read_chunk();
    }

  private:
    void post_reap_child( std::pair<pid_t,int> child ) {
    //   pid_t cpid = child.first;
    //   int status = child.second;

    //   if( pid_to_node.count( cpid ) ) {
    //     Agnode_t *node = pid_to_node.find( cpid )->second;
    //     pid_to_node.erase( cpid );

    //     bool failed = stream_tsort::finished_node( node, status );

    //     if( not failed and node_to_xml.count( node ) ) {
    //       xmlFreeDoc( node_to_xml[ node ] );
    //       node_to_xml.erase( node );
    //     }
    //   }
    }

    // void handle_failed_node( Agnode_t *gnode, bool failed ) {
    //   if( node_to_xml.count( gnode ) ) {
    //     xmlDocPtr xdoc = node_to_xml[ gnode ];
    //     if( _errorstream )
    //       dumpNode( xmlDocGetRootElement( xdoc ), *_errorstream );

    //     xmlFreeDoc( xdoc );
    //     node_to_xml.erase( gnode );
    //   }
    // }

    // void handle_node( Agnode_t *node ) {
    //   pid_t child = parent::handle_node_fork( xmlDocGetRootElement( node_to_xml[ node ] ) );
    //   pid_to_node.insert( std::make_pair( child, node ) );
    // }

    void finish() {
    //   // stream_tsort::find_missing();
    //   while( process_handler::children_active() ) {
    //     process_handler::reap_all_active();
    //     stream_tsort::flush_ready();
    //   }
    //   if( _errorstream )
    //     *_errorstream << "<" << parent::rootname << ">" << std::flush;
    //   stream_tsort::finished_graph();
    //   if( _errorstream )
    //     *_errorstream << "</" << parent::rootname << ">" << std::flush;
    }

    g_traits::vertex_descriptor getVertex( const char *name ) {
      auto i = _name_to_vertex.find( name );

      if( i == _name_to_vertex.end() ) {
        g_traits::vertex_descriptor vertex;
        vertex = boost::add_vertex( _graph );
        _name_to_vertex.insert( std::make_pair( std::string( name ), vertex ) );
        return vertex;
      } else {
        return _name_to_vertex[ name ];
      }
    }

    void handle_node( xmlNodePtr node ) {
      // Make this node into a new document.
      xmlDocPtr doc = xmlNewDoc( toXmlChar( "1.0" ) );
      xmlNodePtr copy = xmlDocCopyNode( node, doc, 1 );
      assert( copy );
      xmlDocSetRootElement( doc, copy );

      // Evaluate the XPath expressions
      xmlXPathContextPtr xpathCtx = xmlXPathNewContext( doc );
      assert( xpathCtx );

      // Find the name of the current node
      xmlXPathObjectPtr srcXPathObj = xmlXPathEvalExpression( toXmlChar( _srcexpr ), xpathCtx );
      assert( srcXPathObj );
      xmlNodeSetPtr srcNodeSet = srcXPathObj->nodesetval;
      if( not srcNodeSet or 1 != srcNodeSet->nodeNr ) {
        parent::handle_node_fork( xmlDocGetRootElement( doc ) );
        xmlFreeDoc( doc );
      } else {
        xmlNodePtr *srcNode = srcNodeSet->nodeTab;
        const char *srcname = toChar( xmlNodeGetContent( *srcNode ) );
        // Agnode_t *src = stream_tsort::found_node( srcname );
        // Find or create the vertex representing this node
        g_traits::vertex_descriptor source = getVertex( srcname );
        // Attach the xml document to it.
        boost::put( boost::get( docptr_t(), _graph ), source, doc );

        // Find the target or child nodes
        xmlXPathObjectPtr targetXPathObj = xmlXPathEvalExpression( toXmlChar( _targetexpr ), xpathCtx );
        assert( targetXPathObj );
        xmlNodeSetPtr targetNodes = targetXPathObj->nodesetval;

        if( targetNodes )
          for( xmlNodePtr *i = targetNodes->nodeTab; i != targetNodes->nodeTab + targetNodes->nodeNr; ++i ) {
            const char *targetName = toChar( xmlNodeGetContent( *i ) );
            g_traits::vertex_descriptor target = getVertex( targetName );
            if( source != target )
              boost::add_edge( source, target, _graph );
          }

        q.push( source );
        xmlXPathFreeObject( targetXPathObj );
      }

      xmlXPathFreeObject(  srcXPathObj );
      xmlXPathFreeContext( xpathCtx );
    }

    const char   *_srcexpr, *_targetexpr;
    std::ostream *_errorstream;
    graph_t       _graph;
    std::map<
        std::string,
        g_traits::vertex_descriptor
      > _name_to_vertex;
    Ch           *mybuf;
    int           mybufsize;
    std::queue<g_traits::vertex_descriptor> q;

    basic_tsorter();
    basic_tsorter( const basic_tsorter& );
};

typedef basic_tsorter<char> tsorter;

using namespace std;

void usage( const char *name ) {
  cerr << endl;
  cerr << "Usage:" << endl;
  cerr << "  " << name << " [-f <file>] [-W|-S] [-r] [-v|-t] [-P <maxprocs>] <xpath expression> <element name expr> <child name expr> <cmd> [arg [...]]" << std::endl;
}

int main( int argc, const char *argv[] ) {
  /*
   * Options parsing
   */
  // Defaults
  bool verbose = false;
  bool printroot = false;
  bool wholefile = true;
  int  maxprocs = 1;
  int  stop_on_error = false;
  char *errorfile = NULL;

  int c, bflg, aflg, errflg;
  char *ifile = NULL, *ofile = NULL;
  extern char *optarg;
  extern int optind, optopt;

  // Limit the number of arguments that getopt sees if '--' is part of the
  // argument list.
  int myargc = 0;
  while( myargc < argc and strcmp( "--", argv[ myargc ] ) )
    ++myargc;

  while( ( c = getopt( argc, const_cast<char**>(argv), "f:E:RvtP:WS" ) ) != -1 )
    switch (c) {
      case 'E' :
        errorfile = optarg;
        break;

      case 'R' :
        printroot = true;
        break;

      case 'S' :
        wholefile = false;
        break;

      case 'W' :
        wholefile = true;
        break;

      case 'f' :
        ifile = optarg;
        break;

      case 't' : case 'v' :
        verbose = true;
        break;

      case 'P' :
        for( const char *digit = optarg; *digit; ++digit )
          if( *digit < '0' or '9' < *digit ) {
            cerr << argv[0] << ": maxprocs must be a number > 0" << endl;
            usage( argv[0] );
            exit(1);
          }
        if( not strcmp( "0", optarg ) ) {
          cerr << argv[0] << ": maxprocs cannot be \"0\"" << endl;
          usage( argv[0] );
          exit(1);
        }
        maxprocs = atoi( optarg );
        break;

      case ':' : case '?' :
        usage( argv[0] );
        exit(1);
    }

  if( ( argc - optind ) < 4 ) {
    cerr << argv[0] << ": Not enough arguments" << endl;
    usage( argv[0] );
    exit(1);
  }

  std::ostream *errstream = NULL;
  if( errorfile ) {
    errstream = new ofstream( errorfile );
  }

  std::istream *in = &cin;
  if( ifile ) {
    in = new std::ifstream( ifile );
    if( not in or not *in ) {
      std::cerr << "Couldn't open file for reading!" << std::endl;
      exit(1);
    }
  }

  tsorter my_crawler( *in,
                       argv[optind],
                       argv[optind+1],
                       argv[optind+2],
                       argv + optind + 3,
                       wholefile,
                       errstream );
  my_crawler.set_stop_on_error( stop_on_error );
  my_crawler.set_max_procs( maxprocs );
  my_crawler.set_verbose( verbose );
  my_crawler.set_printroot( printroot );

  my_crawler.run();

  if( ifile )
    delete( in );

  if( errstream ) {
    *errstream << flush;
    delete errstream;
  }

  if( my_crawler.process_failed() )
    exit(123);

  // if( my_crawler.cycle_found() )
  //   exit(122);

  return 0;
}
