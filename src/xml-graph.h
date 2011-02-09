/*
 * © Copyright 2011 Carl N. Baldwin
 *
 * Confidential computer software. Valid license from Carl Baldwin required for
 * possession, use or copying.
 */
#ifndef XMLGRAPH_H
#define XMLGRAPH_H

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>

/*
 * These two boiler plate types attach an xmlDocPtr to each vertex in the graph
 * below.
 */
struct docptr_t { typedef boost::vertex_property_tag kind; };
typedef boost::property<docptr_t,xmlDocPtr> docptr_p;

/*
 * This class uses the BGL's adjacency_list class to create a basic graph type
 * where each vertex corresponds to an xml document.  The following class
 * sub-classes it one more time in order to provide the special vertex iterator
 * required by the stream DFS class.
 */
typedef boost::adjacency_list<
    // OutEdgeList : I chose vector because I don't anticipate the need to
    // remove edges.  The algorthim, for now, will only add edges.
    boost::vecS,
    // VertexList  : I chose vector here because the DFS default property map
    // uses the vertex index to map vertex to the external color property.  When
    // using vector as this container you get this for free.
    boost::vecS,
    // Directionality is bidirectionalS because this algorithm will have the
    // need to follow edges backwards often.
    boost::bidirectionalS,
    docptr_p
  > XmlGraph_base;

  template< class Graph >
struct strmQueueInterface {
  virtual bool empty() = 0;
  virtual void pop() = 0;
  virtual typename boost::graph_traits< Graph >::vertex_descriptor &front() = 0;
};

/*
 * This class provides the interface needed by the stream DFS class.  It
 * basically makes a vertex available when all of the out-edges for that vertex
 * are known.  All of the magic is done in the associated graph_traits class.
 */

struct XmlGraph : public XmlGraph_base {
  strmQueueInterface<XmlGraph> *strm_q;
};

namespace boost {
  template<>
  struct graph_traits< XmlGraph > : public graph_traits< XmlGraph_base > {
    class vertex_iterator
      : public iterator_facade<vertex_iterator,
                               vertex_descriptor,
                               forward_traversal_tag,
                               const vertex_descriptor&,
                               const vertex_descriptor*>
    {
    public:
      vertex_iterator() : q(NULL) {}

      explicit vertex_iterator( XmlGraph &_g ) : q( _g.strm_q ) { q->empty(); }

    private:
      const vertex_descriptor& dereference() const { return q->front(); }

      bool equal( const vertex_iterator& other ) const {
        if( atend() or other.atend() )
          return atend() and other.atend();
        else
          return q->front() == other.q->front();
      }

      bool atend() const { return not q or q->empty(); }

      void increment() { q->pop(); }

      strmQueueInterface<XmlGraph> *q;

      friend class iterator_core_access;
    };
  };

  std::pair< graph_traits< XmlGraph >::vertex_iterator,
             graph_traits< XmlGraph >::vertex_iterator >
  vertices( XmlGraph& g ) {
    return std::make_pair(
      graph_traits< XmlGraph >::vertex_iterator( g ),
      graph_traits< XmlGraph >::vertex_iterator()
    );
  }
}

#endif
