/*
 * © Copyright 2011 Carl N. Baldwin
 *
 * Confidential computer software. Valid license from Carl Baldwin required for
 * possession, use or copying.
 */
#include <unistd.h>
#include <cstring>

#include "crawl-with-fork.h"

using namespace std;

void usage( const char *name ) {
  cerr << endl;
  cerr << "Usage:" << endl;
  cerr << "  " << name << " [-f <file>] [-W|-S] [-R] [-v|-t] [-P <maxprocs>] <xpath expression> <cmd> [arg [...]]" << std::endl;
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

  int c, bflg, aflg, errflg;
  char *ifile = NULL, *ofile = NULL;
  extern char *optarg;
  extern int optind, optopt;

  // Limit the number of arguments that getopt sees if '--' is part of the
  // argument list.
  int myargc = 0;
  while( myargc < argc and strcmp( "--", argv[ myargc ] ) )
    ++myargc;

  while( ( c = getopt( myargc, const_cast<char**>(argv), "f:RvtP:WS" ) ) != -1 )
    switch (c) {
      case 'R' :
        printroot = true;
        break;

      case 't' : case 'v' :
        verbose = true;
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

  if( ( argc - optind ) < 2 ) {
    cerr << argv[0] << ": Not enough arguments" << endl;
    usage( argv[0] );
    exit(1);
  }

  std::istream *in = &cin;
  if( ifile ) {
    in = new std::ifstream( ifile );
    if( not in or not *in ) {
      std::cerr << "Couldn't open file for reading!" << std::endl;
      exit(1);
    }
  }

  marcher my_marcher( *in, argv[optind], argv + optind + 1, wholefile );
  my_marcher.set_stop_on_error( stop_on_error );
  my_marcher.set_max_procs( maxprocs );
  my_marcher.set_verbose( verbose );
  my_marcher.set_printroot( printroot );

  my_marcher.run();

  if( ifile )
    delete( in );

  if( my_marcher.process_failed() )
    exit(123);

  return 0;
}
