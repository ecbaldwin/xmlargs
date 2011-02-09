/*
 * © Copyright 2011 Carl N. Baldwin
 *
 * Confidential computer software. Valid license from Carl Baldwin required for
 * possession, use or copying.
 */
#ifndef CRAWL_WITH_FORK_H
#define CRAWL_WITH_FORK_H

#include "xpath-on-stream.h"
#include "process-handler.h"

template<class Ch, class Tr = std::char_traits<Ch> >
class basic_marcher : public basic_xpath_stream<Ch, Tr>, public process_handler {
  public:
    typedef basic_xpath_stream<Ch,Tr> parent;

    basic_marcher( std::istream &in, const char *expression, const char **argv, bool allatonce )
      : parent( in, expression, allatonce ),
        process_handler( argv ),
        printroot( false )
    {}


    virtual ~basic_marcher() {}

    void set_printroot( bool enabled ) { printroot = enabled; }

  protected:
    void end_xml() {
      if( printroot ) std::cout << "</" << basic_xpath_stream<Ch, Tr>::rootname << ">" << std::flush;
    }

    void begin_xml() {
      if( printroot ) std::cout << "<"  << basic_xpath_stream<Ch, Tr>::rootname << ">" << std::flush;
    }

    void handle_node( xmlNodePtr node ) {
      handle_node_fork( node );
    }

    void finish() {
      process_handler::reap_all_active();
    }

    /*
     * Sets XMLELEMENT equal to the name of the element passed as the node.
     *
     * Also, this method looks through a node to find "simple" children.  That
     * is children that have either no child nodes or only one text child.
     * Here are some examples:
     *
     *   <flag/>
     *   <empty></empty>
     *   <name>myname</name>
     *
     * For each such child an environment variable is set.  For example, if the
     * above nodes are found as children of the given node then environment
     * variables will be set as if these commands were run in the shell:
     *
     *   export flag=''
     *   export empty=''
     *   export name='myname'
     */
    int set_environment( xmlNodePtr node ) {
      int rc = setenv( "XMLELEMENT", toChar( node->name ), true );

      if( rc ) {
        std::cerr << "Couldn't set environment" << std::endl;
        abort();
      }

      bool settext = false;
      for( xmlNodePtr child = node->children; child; child = child->next ) {
        if( not child->children ) {
          rc = setenv( toChar( child->name ), "", true );
          if( rc ) {
            std::cerr << "Couldn't set environment" << std::endl;
            abort();
          }
        }
        if( child->children &&
            not child->children->next &&
            XML_TEXT_NODE == child->children->type ) {
          rc = setenv( toChar( child->name ),
                       toChar( serialize_node( child->children ) ),
                       true );
          if( rc ) {
            std::cerr << "Couldn't set environment" << std::endl;
            abort();
          }
        }
        if( not settext and XML_TEXT_NODE == child->type ) {
          settext = true;
          rc = setenv( "XMLTEXT", toChar( serialize_node( child ) ), true );

          if( rc ) {
            std::cerr << "Couldn't set environment" << std::endl;
            abort();
          }
        }

      }

      return 0;
    }

    /*
     * Spawns a child process who will provide the worker process with the XML
     * data on stdin.
     *
     * Returns only if we're in the parent process.  Otherwise exits.
     */
    void spawn_input_source( xmlNodePtr node ) {
      static int READ(0), WRITE(1);

      // Create the pipe that will be used for stdin of the parent process
      int fd[ 2 ];
      int rc = pipe( fd );
      if( 0 != rc ) {
        errno_msg( "pipe" );
        abort();
      }

      // Make the read end of the pipe the standard input of this process.
      dup2(  fd[READ], 0 );
      close( fd[READ] );

      pid_t pid = fork();
      if( -1 == pid ) {
        errno_msg( "fork" );
        abort();
      }

      if( not pid ) {
        // The grand-child process that provides stdin to its parent
        close( 0 );

        // Make the write end of the pipe the stdout of this process.
        dup2(  fd[WRITE], 1 );
        close( fd[WRITE] );

        // Now, simply dump the XML data to stdout.
        dumpNode( node );

        exit(0);
      }

      close( fd[WRITE] );
    }

    pid_t handle_node_fork( xmlNodePtr node ) {
      if( pid_t pid = spawn_worker() )
        return pid;

      spawn_input_source( node );
      set_environment( node );
      exec_program();
    }

  private:
    // Options
    bool printroot;

    basic_marcher();
    basic_marcher( const basic_marcher& );
};

typedef basic_marcher<char> marcher;

#endif
