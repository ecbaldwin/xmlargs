/*
 * © Copyright 2011 Carl N. Baldwin
 *
 * Confidential computer software. Valid license from Carl Baldwin required for
 * possession, use or copying.
 */
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#include <utility>
#include <cassert>
#include <iostream>

#include "process-handler.h"

process_handler::process_handler( const char **argv )
  : _argv( argv ),
    _verbose( false ),
    stop_on_error( false ),
    a_process_failed( false ),
    max_active_processes( 1 )
{
  while( not strcmp( "--", _argv[0] ) )
    _argv++;
  if( not _argv[0] or not *_argv[0] )
    exit( 125 );
}

process_handler::~process_handler() {
  reap_all_active();
}

bool process_handler::process_failed() {
  reap_all_active();
  return a_process_failed;
}

void process_handler::set_stop_on_error( bool enabled ) {
  stop_on_error = enabled;
}

void process_handler::set_max_procs( int max ) {
  max_active_processes = max;
}

void process_handler::set_verbose( bool enabled ) {
  _verbose = enabled;
}

bool process_handler::processes_are_active() {
  return not active_processes.empty();
}

std::pair<pid_t,int> process_handler::reap_process( pid_t pid ) {
  if( not active_processes.empty() ) {
    int status;
    pid_t wpid;
    while( true ) {
      wpid = waitpid( pid, &status, 0 );
      if( -1 == wpid ) {
        errno_msg( "waitpid" );
        abort();
      }
      if( active_processes.count(wpid) )
        break;
    }

    if( 0 < pid )
      assert( wpid == pid );

    if( 255 == WEXITSTATUS( status ) )
      exit(124);

    if( WIFSIGNALED( status ) )
      exit(125);

    if( WEXITSTATUS( status ) ) {
      if( 126 <= WEXITSTATUS( status ) )
        exit( WEXITSTATUS( status ) );

      if( 1 == max_active_processes and stop_on_error )
        exit( 123 );
      else
        a_process_failed = true;
    }
    std::pair<pid_t,int> child = std::make_pair( wpid, WEXITSTATUS( status ) );
    active_processes.erase( wpid );
    post_reap_process( child );
    return child;
  }
  return std::make_pair( 0, 0 );
}

/*
 * Spawns a child process to handle the current node
 *
 * Returns true if we're in the parent process.
 */
pid_t process_handler::spawn_worker() {
  // If the maximum number of processes has been reached then wait
  if( active_processes.size() >= max_active_processes )
    reap_process();

  pid_t pid = fork();
  if( -1 == pid ) {
    errno_msg( "fork" );
    abort();
  }

  if( pid )
    active_processes.insert( pid );

  return pid;
}

void process_handler::abort( int status ) {
  reap_all_active();
  exit( status );
}

void process_handler::errno_msg( const char *name ) {
  std::cerr << name
    << " failed with errno="
    << errno
    << " '"
    << strerror( errno )
    << "'"
    << std::endl;
}

void process_handler::exec_program() {
  if( verbose() ) {
    const char **args = _argv;
    while( *args ) {
      if( args != _argv ) std::cerr << ' ';
      std::cerr << *args;
      ++args;
    }
    std::cerr << std::endl;
  }

  execvp( _argv[0], const_cast<char**>(_argv) );

  // This section will only be reached if the exec failed
  switch( errno ) {
    case ENOENT : exit( 127 );
    default     : exit( 126 );
  }
}

void process_handler::reap_all_active() {
  while( not active_processes.empty() )
    reap_process();
}
