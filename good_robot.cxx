#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <sstream>

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

class Interpreter
{
    public:
        Interpreter ( CommandStream & commandStream );
        // This is so one interpreter can retain state across multiple
        // sequential command streams.
        void setCommandStream ( CommandStream & commandStream );
        void run();
    private:
        CommandStream & m_commandStream;
};

//////////////////////////////////////////////////////////////////////////////

extern int main ( int argc, char ** argv )
{
    try
    {
        // Read from supplied files or else stdin.
        if ( argc > 1 )
        {
            // Awkward first attempt at handling straddling multiple input
            // files. This is really ugly.
            CommandStream commandStream ( argv[1] );
            Interpreter interpreter ( commandStream );
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
        // A better way would be to use an iterator with a function... need to go
        // back and look at the STL API again.
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
        cerr << "Got command \"" << command << "\"" << endl;
    }
    cerr << "EOF"<< endl;
}
