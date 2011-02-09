/*
 * © Copyright 2011 Carl N. Baldwin
 *
 * Confidential computer software. Valid license from Carl Baldwin required for
 * possession, use or copying.
 */
#include <unistd.h>

#include <vector>
#include <iterator>
#include <string>
#include <fstream>
#include <cstring>
#include <errno.h>

#include "crawl-with-fork.h"

template<class Ch, class Tr = std::char_traits<Ch> >
class basic_xmlargs : public basic_marcher<Ch, Tr> {
  public:
    typedef basic_marcher<Ch,Tr> parent;

    basic_xmlargs( std::istream &in, const char *expression, const char **argv, bool allatonce )
      : parent( in, expression, argv, allatonce ),
        run_if_empty( true ),
        ran( false ),
        max_chars(0), max_args(0),
        initial_length( 0 ),
        args_length( 0 )
      {
        while( *argv )
          initial_length += strlen( *argv++ ) + 1;
      }

    virtual ~basic_xmlargs() { /* Nothing to do here. */ }

    void finish() {
      if( ( not ran and run_if_empty ) or 0 != arguments.size() )
        handle_arguments();
    }

    void set_max_chars( int max ) {
      max_chars = max;
    }

    void set_max_args( int max ) {
      max_args = max;
    }

    void set_run_if_empty( bool on ) {
      run_if_empty = on;
    }

  private:
    pid_t handle_arguments() {
      ran = true;
      // This vector will build all of the arguments.
      std::vector<const char*> cmd_args;

      if( not strcmp( "echo", process_handler::get_argv()[0] ) and not process_handler::get_argv()[1] ) {
        // If the command is just echo then don't spawn children.  Just do it.
        int size = arguments.size();
        if( 1 < size )
          std::copy( arguments.begin(), arguments.end()-1,
                     std::ostream_iterator<std::string>( std::cout, " " ) );
        if( 0 < size )
          std::cout << *(arguments.end()-1) <<  std::endl;
        arguments.clear();
        args_length = 0;
        return 0;
      }

      // Start with the initial arguments.
      const char **initial_args = process_handler::get_argv();
      while( *initial_args )
        cmd_args.push_back( *initial_args++ );

      std::vector<std::string>::const_iterator i;
      for( i = arguments.begin(); i != arguments.end(); ++i )
        cmd_args.push_back( const_cast<char*>( i->c_str() ) );

      cmd_args.push_back( NULL );

      if( pid_t pid = process_handler::spawn_worker() ) {
        // The parent doesn't need the arguments list anymore.
        arguments.clear();
        args_length = 0;
        return pid;
      }

      exec_program( cmd_args );
    }

    void exec_program( std::vector<const char*> &arg_vector ) {
      if( process_handler::verbose() ) {
        const char **args = &arg_vector[0];
        while( *args ) {
          if( args != &arg_vector[0] ) std::cerr << ' ';
          std::cerr << *args;
          ++args;
        }
        std::cerr << std::endl;
      }

      execvp( arg_vector[0], const_cast<char**>(&arg_vector[0]) );

      // This section will only be reached if the exec failed
      switch( errno ) {
        case ENOENT : exit( 127 );
        default     : exit( 126 );
      }
    }

    void handle_node( xmlNodePtr node ) {
      std::string current_arg;

      // Get all of the text for the current node to current_arg
      // Should this be doing text children of descendant nodes too?
      for( xmlNodePtr child = node->children; child; child = child->next )
        if( XML_TEXT_NODE == child->type )
          current_arg += toChar( serialize_node( child ) );

      if( 0 < max_args and max_args == arguments.size() + 1 ) {
        arguments.push_back( current_arg );
        handle_arguments();
      } else {
        int current_length = current_arg.length() + 1;

        if( max_chars < initial_length + args_length + current_length )
          handle_arguments();

        arguments.push_back( current_arg );
        args_length += current_length;
      }
    }

    basic_xmlargs();
    basic_xmlargs( const basic_xmlargs& );

    bool run_if_empty, ran;
    int max_chars, max_args;
    int initial_length, args_length;
    std::vector<std::string> arguments;
};

typedef basic_xmlargs<char> xmlargs;
using namespace std;

void usage( const char *name ) {
  cerr << endl;
  cerr << "Usage:" << endl;
  cerr << "  " << name << " [-f <file>] [-W|-S] [-v|-t] [-r] [-n <maxargs>] <xpath expression> <cmd> [arg [...]]" << std::endl;
}

int main( int argc, char *argv[] ) {
  /*
   * Options parsing
   */
  // Defaults
  bool verbose = false;
  int  maxargs = 0;
  int  maxchars = 20 * 1024; // Is there a system #define for this?
  bool run_if_empty = true;
  bool wholefile = true;

  int c, bflg, aflg, errflg;
  char *ifile = NULL, *ofile = NULL;
  extern char *optarg;
  extern int optind, optopt;

  // Limit the number of arguments that getopt sees if '--' is part of the
  // argument list.
  int myargc = 0;
  while( myargc < argc and strcmp( "--", argv[ myargc ] ) )
    ++myargc;

  while( ( c = getopt( myargc, argv, "f:rn:vtWS" ) ) != -1 )
    switch (c) {
      case 't' : case 'v' :
        verbose = true;
        break;

      case 'r' :
        run_if_empty = false;
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

      case 'n' :
        for( const char *digit = optarg; *digit; ++digit )
          if( *digit < '0' or '9' < *digit ) {
            cerr << argv[0] << ": maxargs must be a number >= 0" << endl;
            usage( argv[0] );
            exit(1);
          }
        maxargs = atoi( optarg );
        break;

      case ':' : case '?' :
        usage( argv[0] );
        exit(1);
    }

  if( ( argc - optind ) < 1 ) {
    cerr << argv[0] << ": Not enough arguments" << endl;
    usage( argv[0] );
    exit(1);
  }

  const char *default_cmd[2];
  default_cmd[0] = "echo";
  default_cmd[1] = NULL;

  const char **command_args = const_cast<const char**>(argv) + optind + 1;
  if( ( argc - optind ) < 2 ) {
    command_args = default_cmd;
  }

  std::istream *in = &cin;
  if( ifile ) {
    in = new std::ifstream( ifile );
    if( not in or not *in ) {
      std::cerr << "Couldn't open file for reading!" << std::endl;
      exit(1);
    }
  }

  xmlargs my_xmlargs( *in, argv[optind], command_args, wholefile );
  my_xmlargs.set_max_chars( maxchars );
  my_xmlargs.set_max_args( maxargs );
  my_xmlargs.set_run_if_empty( run_if_empty );
  my_xmlargs.set_verbose( verbose );

  my_xmlargs.run();

  my_xmlargs.finish();

  if( ifile )
    delete( in );

  if( my_xmlargs.process_failed() )
    exit(123);

  return 0;
}
