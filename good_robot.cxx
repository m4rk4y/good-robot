/*
Classes:
    CommandStream: reads files or stdin and produces command lines
    Interpreter: takes command lines and hands out Commands to interested
        parties
    Command: what to do
    CommandListener: interface to interested party, implemented here by Robot
    Constraint
    ConstraintListener

(perhaps CommandListener and ConstraintListener can have a common superclass
Listener?)

CommandStream + Interpreter read commands endlessly until EOF
Sends each command to every registered command-listener
Each command-listener (robot) responds to command appropriately, observing constraints
Constraint requests are broadcast to constraint-listeners

Set of commands map from each instruction (simple solution sets up hard-coded
map; better would be to read from setup file so can e.g. do multiple
languages)

Considerations...

Each command says what it would like to do: how best to encapsulate message
between command and robot, how to direct response, how to design set-of-
commands so that robot doesn't need to know what class of command it is, just
what the command is requesting

Different commands: class hierarchy? Could be best for extensibility but a bit
overkill for simple example.

Multiple robots need to avoid collisions. These and the table-edge are
examples of constraints. Need a uniform interface which takes Proposal and
returns verdict.
*/

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

//////////////////////////////////////////////////////////////////////////////

class CommandStream
{
    public:
        CommandStream ( const char * fileName );
        CommandStream ( FILE * stream );
        ~CommandStream();
        bool getCommand ( string & command ) const;
        bool active() const { return m_stream != 0; }
    private:
        FILE * m_stream;
};

//////////////////////////////////////////////////////////////////////////////
// Abstract interface.

class CommandListener
{
    public:
        virtual void inform ( const string & command ) = 0;
};

//////////////////////////////////////////////////////////////////////////////

class Robot : public CommandListener
{
    public:
        Robot ( const char * name );
        virtual void inform ( const string & command );
        const char * name() { return m_name.c_str(); }
    private:
        string m_name;
};

//////////////////////////////////////////////////////////////////////////////

class Interpreter
{
    public:
        Interpreter ( CommandStream & commandStream );
        // This is so one interpreter can retain state across multiple
        // sequential command streams.
        void setCommandStream ( CommandStream & commandStream );
        void run();
        void registerCommandListener ( CommandListener & commandListener );
    private:
        void broadcast ( const string & command );
        CommandStream & m_commandStream;
        vector< CommandListener* > m_commandListeners;
};

//////////////////////////////////////////////////////////////////////////////

extern int main ( int argc, char ** argv )
{
    try
    {
        Robot robot1 ( "Robbie" );
        Robot robot2 ( "Arthur" );

        // Read from supplied files or else stdin.
        if ( argc > 1 )
        {
            // Awkward first attempt at handling straddling multiple input
            // files. This is really ugly. And pointless for now because an
            // Interpreter *has* no state to retain.

            CommandStream commandStream ( argv[1] );
            Interpreter interpreter ( commandStream );
            interpreter.registerCommandListener ( robot1 );
            interpreter.registerCommandListener ( robot2 );
            interpreter.run();
            for ( int inx = 2; inx < argc; ++inx )
            {
                CommandStream commandStream ( argv[inx] );
                interpreter.setCommandStream ( commandStream );
                interpreter.run();
            }
        }
        else
        {
            CommandStream commandStream ( stdin );
            Interpreter interpreter ( commandStream );
            interpreter.registerCommandListener ( robot1 );
            interpreter.registerCommandListener ( robot2 );
            interpreter.run();
        }
    }
    catch ( const string & error )
    {
        cerr << "Caught exception: " << error << endl;
        return 1;
    }
    catch ( const char * error )
    {
        cerr << "Caught exception: " << error << endl;
        return 1;
    }
    catch ( ... )
    {
        cerr << "Caught unknown exception" << endl;
        return 1;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

CommandStream::CommandStream ( const char * fileName )
 : m_stream ( 0 )
{
    m_stream = fopen ( fileName, "r" );
    if ( m_stream == 0 )
    {
        stringstream errorStream;
        errorStream << "Failed to open file "
                    << fileName << " for reading";
        throw errorStream.str();
    }
}

CommandStream::CommandStream ( FILE * stream )
  : m_stream ( stream )
{
}

CommandStream::~CommandStream()
{
    if ( m_stream != 0 )
    {
        fclose ( m_stream );
    }
}

bool CommandStream::getCommand ( string & command ) const
{
    string line;
    for (;;)    // loop until we read a non-blank line or EOF
    {
        // Hideous but I can't get the STL equivalent to handle file streams
        // and cin equally transparently.
        char buffer[1024];
        if ( fgets ( buffer, 1024, m_stream ) == 0 &&
             feof ( m_stream ) != 0 )
        {
            return false;
        }
        line = buffer;

        // Got a line? Trim it.
        // A better way would be to use an iterator with a function to
        // encapsulate isspace or whatever... need to go back and look at the
        // STL API again.
        size_t first = line.find_first_not_of ( " \t\n");
        size_t last = line.find_last_not_of ( " \t\n");
        // "abc": first = 0, last = 2
        // " abc": first = 1, last = 3
        // "abc ": first = 0, last = 2
        // " abc ": first = 1, last = 3
        if ( first != string::npos )    // and by implication last != npos
        {
            command = line.substr ( first, last - first + 1 );
            return true;
        }
        // else content-free line so try for the next one
    }
}

//////////////////////////////////////////////////////////////////////////////

Interpreter::Interpreter ( CommandStream & commandStream )
  : m_commandStream ( commandStream )
{
}

void Interpreter::setCommandStream ( CommandStream & commandStream )
{
    m_commandStream = commandStream;
}

void Interpreter::run()
{
    string command;
    while ( m_commandStream.getCommand ( command ) )
    {
        broadcast ( command );
    }
}

// For completeness, ought to have remove as well.
void Interpreter::registerCommandListener
(   CommandListener & commandListener
)
{
    m_commandListeners.push_back ( &commandListener );
}

// Better as a Command object argument?
void Interpreter::broadcast ( const string & command )
{
    for ( std::vector< CommandListener* >::iterator iter = m_commandListeners.begin();
          iter != m_commandListeners.end(); ++iter )
    {
        (*iter)->inform ( command );
    }
}

//////////////////////////////////////////////////////////////////////////////

Robot::Robot ( const char * name )
 : m_name ( name )
{
}

void Robot::inform ( const string & command )
{
    cout << "Robot " << m_name << " has been told: " << command << endl;
}
