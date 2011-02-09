/*
 * © Copyright 2011 Carl N. Baldwin
 *
 * Confidential computer software. Valid license from Carl Baldwin required for
 * possession, use or copying.
 */
#ifndef STREAM_DFS_H
#define STREAM_DFS_H

#include <queue>
#include <algorithm>

// This class adapts an existing BGL compatible graph.  Its job is to iterate
// over edges of the graph from an input stream and use them to make the figure
// out when to make each vertex of the graph available to the Depth-First Search
// algorithm through an overloaded vertex iterator.
//
// There is some implementation dependant code that must be written.
//
//  * Vertex list iterator
//    * If there are no more vertices then the iterator should compare equal to
//    the "one-past-the-end" iterator
//    * If there are more vertices that have already been read (queued in the
//    vertex list container) then increment to the next one
//    * If there are no more queued vertices then read enough data from the
//    input stream to find at least one more vertex where all of the out edges
//    have been seen.

namespace boost {
  template< class Graph >
  class StreamDfs : public Graph {
    public:
      // Remember the base graph can grow arbitrarily large.
      void resize_properties() {
        int oldsize = hasoutedges.size();
        int newsize = num_vertices( *this );
        if( oldsize < newsize ) {
          hasoutedges.resize( newsize, false );
          waiting.resize(     newsize, 0 );
          connected.resize(   newsize, false );
        }
      }
      // Poor mans external properties.  I should probably follow the BGL
      // Property interfaces but I'm a little too lazy right now.

      // * Whether this vertex has all its out edges
      std::vector<bool> hasoutedges;
      // * Number of adjacent vertices waiting to be complete
      std::vector<int>  waiting;
      // * Whether this vertex is fully connected
      std::vector<bool> connected;
  };

  template< class Graph >
  struct graph_traits< StreamDfs< Graph > > : public graph_traits< Graph > {
    class vertex_iterator
      : public iterator_facade<vertex_iterator,
                               typename graph_traits< Graph >::vertex_descriptor,
                               forward_traversal_tag,
                               const typename graph_traits< Graph >::vertex_descriptor&,
                               const typename graph_traits< Graph >::vertex_descriptor* >
    {
    public:
      typedef typename graph_traits< Graph >::vertex_descriptor vertex_descriptor;
      typedef typename graph_traits< Graph >::out_edge_iterator out_edge_iterator;
      typedef typename graph_traits< Graph >::in_edge_iterator  in_edge_iterator;

      vertex_iterator() : g(NULL), end(true) {}

      explicit vertex_iterator( StreamDfs< Graph > &_g ) : g(&_g), end( false ) {
        tie( iter, enditer ) = vertices( static_cast<Graph&>(*g) );
        if( iter == enditer )
          end = true;
        else
          replenish_q();

        if( not q.empty() ) {
          current = q.front();
          q.pop();
        }
      }

    private:
      const vertex_descriptor& dereference() const { return current; }

      bool equal( const vertex_iterator& other ) const {
        if( end or other.end )
          return end and other.end;
        else
          return current == other.current;
      }

      void increment() {
        replenish_q();
        current = q.front();
        q.pop();
        return;
      }

      void replenish_q() {
        while( q.empty() ) {
          // Defer to the base class iterator to find a new vertex
          if( iter == enditer ) {
            // TODO Need to find implicit vertices and insert them into the
            // queue
            end = true;
            return;
          }

          // Since we deferred to the base iterator the graph could have grown.
          // The property map needs to grow with it.
          g->resize_properties();

          if( g->hasoutedges[ *iter ] )
            ++iter;

          // The base iterator found a vertex for which all out edges are known.
          g->hasoutedges[ *iter ] = true;

          // Find the number of adjacent edges that are still not marked as
          // connected.  Assign this to the waiting property for this vertex.
          std::pair< // I really want the auto keyword.
              out_edge_iterator,
              out_edge_iterator
            > edges = out_edges( *iter, *g );
          for( ; edges.first != edges.second; ++edges.first ) {
            if( not g->connected[ target( *edges.first, *g ) ] )
              ++g->waiting[ *iter ];
          }

          mark_connected( *iter );
        }
      }

      void mark_connected( vertex_descriptor v ) {
        // TODO Review this and get the properties attached to the graph.
        if( 0 == g->waiting[ v ] and not g->connected[ v ] ) {
          // Mark it connected and queue it up.
          g->connected[ v ] = true;
          q.push( v );

          // Traverse through in edges to see what else has become connected.
          std::pair< // I really want the auto keyword.
              in_edge_iterator,
              in_edge_iterator
            > edges = in_edges( v, *g );
          for( ; edges.first != edges.second; ++edges.first ) {
            vertex_descriptor u = source( *edges.first, *g );
            if( g->hasoutedges[ u ] ) {
              --g->waiting[ u ];
              mark_connected( u );
            }
          }
        }
      }

      // These data members keep track of where we are in the base class's
      // iterator.
      typename graph_traits< Graph >::vertex_iterator iter,enditer;
      std::queue<vertex_descriptor>                   q;

      StreamDfs< Graph >                             *g;
      vertex_descriptor                               current;
      bool                                            end;

      friend class iterator_core_access;
    };
  };

  template< class Graph >
  std::pair< typename graph_traits< StreamDfs< Graph > >::vertex_iterator,
             typename graph_traits< StreamDfs< Graph > >::vertex_iterator >
  vertices( StreamDfs< Graph >& g ) {
    return std::make_pair(
      typename graph_traits< StreamDfs< Graph > >::vertex_iterator( g ),
      typename graph_traits< StreamDfs< Graph > >::vertex_iterator()
    );
  }
};

#endif
