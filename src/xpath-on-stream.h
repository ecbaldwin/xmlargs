/*
 * © Copyright 2011 Carl N. Baldwin
 *
 * Confidential computer software. Valid license from Carl Baldwin required for
 * possession, use or copying.
 */
#ifndef XPATH_CRAWL_H
#define XPATH_CRAWL_H

#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
#include <stack>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "xml-util.h"

/*
 * This class extends the chunk parser and stuffs data from it into the libxml2
 * parser using its push parser interface.
 *
 * Its job is to look for XML elements in the stream that match the given XPath
 * expression.  When it finds any such elements it calls the pure virtual method
 * handle_node( xmlNodePtr ) for each element.
 *
 * When the entire XML document has been seen and processed by 'handle_node'
 * this class calls the pure virtual finish() to signal that the XML document
 * has been processed and the derived class should finish its work.
 */

template<class Ch, class Tr = std::char_traits<Ch> >
class basic_xpath_stream {
  public:
    basic_xpath_stream( std::istream &_in, const char *expression, bool allatonce )
      : in( _in ),
        bufsize( allatonce ? 8096 : 128 ),
        buf( new Ch[ bufsize+1 ] ),
        initialized( false ),
        rootfound( false ),
        completed( false ),
        fileatonce( allatonce ),
        ctxt( NULL ),
        xpathExpr( expression ),
        num_read(0)
    {
      LIBXML_TEST_VERSION
      buf[bufsize+1] = '\0';
    }

    virtual ~basic_xpath_stream() {
      delete buf;
      xmlCleanupParser();
      if( rootfound )
        end_xml( rootname );
    }

    void run() {
      while( not finished() and not not in )
        read_chunk();
    }

    void read_chunk() {
      in.read( buf, bufsize );
      handle_chunk( buf, buf + in.gcount() );
    }

    bool finished() { return completed; }

    virtual void finish() = 0;
    virtual void handle_node( xmlNodePtr node ) = 0;
    virtual void begin_xml( const std::string & ) {}
    virtual void end_xml(   const std::string & ) {}

  protected:
    void handle_chunk( Ch *b, Ch *e ) {
      if( b == e ) return;

      if( not initialized ) {
        // I need to read a few bytes from the beginning for the parser to be
        // able to detect the encoding.  It may not come all in the first chunk
        // so I need to save it and wait.
        // CNB here.  I had to add these static_cast to compile on 64 bit
        int num_in_header = std::min( static_cast<int>( e-b ), static_cast<int>( header_size-num_read ) );
        std::copy( b, b+num_in_header, header+num_read );
        num_read += num_in_header;

        if( num_read < header_size )
          return;

        initialized = true;

        ctxt = xmlCreatePushParserCtxt( NULL, NULL, header, header_size, NULL );
        assert( ctxt );

        xmlParseChunk( ctxt, b+num_in_header, e-(b+num_in_header), 0 );
      } else {
        if( not ctxt )
          return;
        xmlParseChunk( ctxt, b, e-b, 0 );
      }

      if( XML_PARSER_EPILOG == ctxt->instate )
        completed = true;

      if( not fileatonce or completed ) {
        xmlXPathContextPtr xpathCtx = xmlXPathNewContext( ctxt->myDoc );
        assert( xpathCtx );

        xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression( toXmlChar( xpathExpr ), xpathCtx );

        if( xpathObj ) {
          xmlNodeSetPtr nodes = xpathObj->nodesetval;
          if( nodes and nodes->nodeNr )
            for( xmlNodePtr *i = nodes->nodeTab; i != nodes->nodeTab + nodes->nodeNr; ++i )
              if( nodeIsComplete( *i ) and not processed.count( *i ) ) {
                processed.insert( *i );
                unprocessed.erase( *i );
                if( not rootfound ) {
                  rootname = toChar( xmlDocGetRootElement( ctxt->myDoc )->name );
                  begin_xml( rootname );
                  rootfound = true;
                }
                handle_node( *i );
              } else {
                unprocessed.insert( *i );
              }

          xmlXPathFreeObject(  xpathObj );
        }
        xmlXPathFreeContext( xpathCtx );
      }

      if( not fileatonce )
        trim_nodes( xmlDocGetRootElement( ctxt->myDoc ) );

      if( completed ) {
        xmlParseChunk( ctxt, NULL, 0, 1 );
        xmlDocPtr doc = ctxt->myDoc;
        int res = ctxt->wellFormed;
        xmlFreeParserCtxt( ctxt );
        ctxt = NULL;

        if( not res )
          std::cerr << "Failed to parse " << std::endl;

        xmlFreeDoc( doc );

        finish();
      }
    }

  protected:
    bool nodeIsComplete( xmlNodePtr node ) {
      if( not node )         return false;
      if( completed )        return true;
      if( node->next )       return true;
      if( not node->parent ) return false;

      return nodeIsComplete( node->parent );
    }

    // Returns true if the parent may unlink this node
    bool trim_nodes( xmlNodePtr node, const bool right = true ) {
      if( not node ) return false;

      if( unprocessed.count( node ) ) return false;

      bool trimall = true;
      std::stack<xmlNodePtr> trim;

      // First recurse into children to see which can be trimmed.
      for( xmlNodePtr child = node->children; child; child = child->next )
        if( trim_nodes( child, child == node->last ? right : false ) )
          trim.push( child );
        else
          trimall = false;

      if( trimall and not right ) {
        // This whole sub-tree can be trimmed.  Leave this up to the parent.
        processed.erase( node );
        return true;
      } else {
        // Trim the children
        while( not trim.empty() ) {
          xmlUnlinkNode(   trim.top() );
          xmlFreeNode(     trim.top() );
          trim.pop();
        }

        return false;
      }
    }

    std::istream &in;
    int bufsize;
    Ch *buf;

    bool                 initialized,printroot,rootfound,completed,fileatonce;
    xmlParserCtxtPtr     ctxt;
    const char          *xpathExpr;
    std::set<xmlNodePtr> processed;
    std::set<xmlNodePtr> unprocessed;

    static const int header_size = 5;
    Ch header[ header_size ];
    int num_read;
    std::string rootname;

    basic_xpath_stream();
    basic_xpath_stream( const basic_xpath_stream& );
};

#endif
