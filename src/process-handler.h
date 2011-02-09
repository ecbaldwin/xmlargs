/*
 * © Copyright 2011 Carl N. Baldwin
 *
 * Confidential computer software. Valid license from Carl Baldwin required for
 * possession, use or copying.
 */
#ifndef PROCESS_HANDLER_H
#define PROCESS_HANDLER_H

#include <set>

class process_handler {
  public:
    process_handler( const char **argv );
    virtual ~process_handler();

    // Derived classes use this to ask wether any process failed
    bool process_failed();

    // Use these API calls to control some aspects of how the process handler
    // behaves
    void set_stop_on_error( bool enabled );
    void set_max_procs( int max );
    void set_verbose( bool enabled );

  protected:
    virtual void post_reap_process( std::pair<pid_t,int> ) {}
    virtual std::pair<pid_t,int> reap_process( pid_t pid = -1 );

    /*
     * Spawns a child process and manages it
     *
     * Returns true if we're in the parent process.
     */
    pid_t spawn_worker();
    /*
     * Execs the program with the current argument list.
     */
    void exec_program();
    bool processes_are_active();
    void reap_all_active();

    const char **get_argv() {
      return _argv;
    }

    bool verbose() {
      return _verbose;
    }

    /*
     * Notes on "reserved" exit codes
     *
     * 1        catchall for general errors
     * 2        misuse of shell builtins, according to Bash documentation
     * 126      command invoked cannot execute
     * 127      "command not found"
     * 128      invalid argument to exit
     * 128+n    fatal error signal "n"
     * 130      script terminated by Control-C
     * 255*     exit status out of range
     */
    void abort( int status = 1 );
    void errno_msg( const char *name );

  private:
    const char **_argv;
    bool _verbose;

    std::set<pid_t> active_processes;

    // Options
    bool stop_on_error;
    bool a_process_failed;
    int  max_active_processes;

    process_handler();
    process_handler( const process_handler& );
};

#endif
