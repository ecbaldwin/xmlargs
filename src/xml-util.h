/*
 * © Copyright 2011 Carl N. Baldwin
 *
 * Confidential computer software. Valid license from Carl Baldwin required for
 * possession, use or copying.
 */
#ifndef XML_UTIL_H
#define XML_UTIL_H

#include <libxml/tree.h>

// For now it seems that converting from xmlChar * to char * is safe.  I
// don't if this will always be true.
xmlChar *toXmlChar( char *str ) {
  return reinterpret_cast< xmlChar* >( str );
}

const xmlChar *toXmlChar( const char *str ) {
  return reinterpret_cast< const xmlChar* >( str );
}

char *toChar( xmlChar *str ) {
  return reinterpret_cast< char* >( str );
}

const char    *toChar( const xmlChar *str ) {
  return reinterpret_cast< const char* >( str );
}

const xmlChar *serialize_node( xmlNodePtr node ) {
  static xmlBufferPtr serial_buf = xmlBufferCreate();
  // Tiny leak here
  // xmlBufferFree( serial_buf );

  if( not node )
    return toXmlChar( "" );

  xmlBufferEmpty( serial_buf );
  int len = xmlNodeDump( serial_buf, node->doc, node, 0, 0 );
  serial_buf->content[ len ] = '\0';
  return serial_buf->content;
}

static void dumpNode( xmlNodePtr node, std::ostream &out = std::cout ) {
  xmlBufferPtr buf = xmlBufferCreate();
  xmlNodeDump( buf, node->doc, node, 0, 0 );
  xmlChar *i = buf->content;
  std::copy( buf->content,
             buf->content + buf->use,
             std::ostreambuf_iterator<char>( out ) );
  out << std::flush;
  xmlBufferFree( buf );
}

#endif
