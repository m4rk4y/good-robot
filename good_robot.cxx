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

#include "my_scoped_ptr.hxx"
using namespace scoping;

enum Direction { Invalid, North, East, South, West };
static bool validDirection ( Direction direction );
static string directionAsString ( Direction direction );
static Direction directionFromString ( const string & str );

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
// Capacity to invent new commands on the fly is limited. For example this
// class has no ability to direct attention to a specific robot or robots.

class Command
{
    public:
        string name() const;
        int xpos() const;
        int ypos() const;
        Direction direction() const;
        string objectName() const;
    private:
        Command
        (   const char * name,
            int xpos = 0,
            int ypos = 0,
            Direction direction = Invalid,
            const char * objectName = 0
        );
        string m_name;
        int m_xpos;
        int m_ypos;
        Direction m_direction;
        string m_objectName;
    friend class CommandFactory;
};

//////////////////////////////////////////////////////////////////////////////

class CommandFactory
{
    public:
        static CommandFactory * instance();
        Command * createCommand ( const string & commandString ) const;
    private:
        static CommandFactory * m_factory;
};

//////////////////////////////////////////////////////////////////////////////
// Abstract interface.

class CommandListener
{
    public:
        virtual void inform ( const Command & command ) = 0;
};

//////////////////////////////////////////////////////////////////////////////

class Robot : public CommandListener
{
    public:
        virtual void inform ( const Command & command );
        string name();
        void place ( int xpos, int ypos, Direction direction );
        void move();
        void left();
        void right();
        void report();
    private:
        Robot ( const char * name );
        string m_name;
        int m_xpos;
        int m_ypos;
        Direction m_direction;
        bool m_onTable;
    friend class RobotFactory;
};

//////////////////////////////////////////////////////////////////////////////

class RobotFactory
{
    public:
        static RobotFactory * instance();
        Robot * createRobot ( const char * robotName );
        const set< Robot* > & robots() const;
    private:
        static RobotFactory * m_factory;
        set< Robot* > m_robots;
};

//////////////////////////////////////////////////////////////////////////////

class Interpreter
{
    public:
        Interpreter ( CommandStream & commandStream );
        // This is so one interpreter can retain state (its listeners) across
        // multiple sequential command streams. But a better way might be to
        // have a single intermediary between the interpreter and the
        // listeners, so that multiple interpreters can send commands to a
        // shared set of objects (in e.g. an MMO or for that matter that
        // social-media-based game where many players took it in turns to
        // co-operate in moving an object... what was it called?)
        void setCommandStream ( CommandStream & commandStream );
        void run();
        void registerCommandListener ( CommandListener * commandListener );
    private:
        void broadcast ( const Command & command );
        CommandStream & m_commandStream;
        vector< CommandListener* > m_commandListeners;
};

//////////////////////////////////////////////////////////////////////////////

extern int main ( int argc, char ** argv )
{
    try
    {
        // Also accepts robot-creation commands as input.
        Robot * robot1 = RobotFactory::instance()->createRobot ( "Robbie" );
        Robot * robot2 = RobotFactory::instance()->createRobot ( "Arthur" );

        // Read from supplied files or else stdin.
        if ( argc > 1 )
        {
            // Awkward first attempt at handling straddling multiple input
            // files. This is really ugly.

            // Note also that for now registerCommandListener stores
            // *pointers* to the listeners, so we need to be sure they don't
            // go out of scope while we're using them.

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

Command::Command
(   const char * name,
    int xpos,
    int ypos,
    Direction direction,
    const char * objectName
)
  : m_name ( name ), m_xpos ( xpos ), m_ypos ( ypos ),
    m_direction ( direction ), m_objectName ( objectName )
{
}

string Command::name() const
{
    return m_name;
}

int Command::xpos() const
{
    return m_xpos;
}

int Command::ypos() const
{
    return m_ypos;
}

Direction Command::direction() const
{
    return m_direction;
}

string Command::objectName() const
{
    return m_objectName;
}

//////////////////////////////////////////////////////////////////////////////

CommandFactory * CommandFactory::instance()
{
    static CommandFactory * factory = 0;
    if ( factory == 0 )
    {
        factory = new CommandFactory;
    }
    return factory;
}

Command * CommandFactory::createCommand ( const string & commandString ) const
{
    istringstream parser ( commandString );
    string verb;
    parser >> verb;
    string lcVerb;
    // string.tolower by steam. Ugh.
    // Also this lower-cases the whole string but we just want the command,
    // not the parameters.
    for ( const char * chPtr = verb.c_str(); *chPtr != '\0'; ++chPtr )
    {
        lcVerb.push_back ( tolower( *chPtr ) );
    }

    int newXpos = 0;
    int newYpos = 0;
    Direction newDirection = Invalid;
    string objectName ( "" );

    // Parse for extra arguments when appropriate.
    if ( lcVerb == "place" )
    {
        string newDirectionToken;
        parser >> newXpos >> newYpos >> newDirectionToken;
        newDirection = directionFromString ( newDirectionToken );
        if ( newDirection == Invalid )
        {
            throw "Invalid direction for PLACE";
        }
    }
    else if ( lcVerb == "create" )
    {
        parser >> objectName;
    }

    return new Command ( lcVerb.c_str(),
                         newXpos, newYpos, newDirection, objectName.c_str() );
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
    string commandString;
    while ( m_commandStream.getCommand ( commandString ) )
    {
        try
        {
            Command * command =
                CommandFactory::instance()->createCommand ( commandString );
            scoped_ptr<Command> freeCommand ( command );
            // Now this switching is ugly...
            if ( command->name() == "create" )
            {
                // ... and so is this: shouldn't the *robot* decide whether it
                // wants to listen?
                registerCommandListener (
                    RobotFactory::instance()->createRobot ( command->objectName().c_str() ) );
            }
            else if ( command->name() == "quit" )
            {
                return;
            }
            else
            {
                broadcast ( *command );
            }
        }
        catch ( ... )   // should invent specific exception
        {
            cerr << "Failed to create or run command \"" << commandString << "\"" << endl;
        }
    }
}

// For completeness, ought to have remove as well.
void Interpreter::registerCommandListener
(   CommandListener * commandListener
)
{
    m_commandListeners.push_back ( commandListener );
}

void Interpreter::broadcast ( const Command & command )
{
    for ( std::vector< CommandListener* >::iterator iter = m_commandListeners.begin();
          iter != m_commandListeners.end(); ++iter )
    {
        (*iter)->inform ( command );
    }
}

//////////////////////////////////////////////////////////////////////////////

RobotFactory * RobotFactory::instance()
{
    static RobotFactory * factory = 0;
    if ( factory == 0 )
    {
        factory = new RobotFactory;
    }
    return factory;
}

Robot * RobotFactory::createRobot ( const char * robotName )
{
    Robot * robot = new Robot ( robotName );
    m_robots.insert ( robot );
    return robot;
}

const set< Robot* > & RobotFactory::robots() const
{
    return m_robots;
}

//////////////////////////////////////////////////////////////////////////////

Robot::Robot ( const char * name )
 : m_name ( name ),
   m_xpos ( 0 ),            // }
   m_ypos ( 0 ),            // } but irrelevant since not on table
   m_direction ( Invalid ), // }
   m_onTable ( false )
{
}

string Robot::name()
{
    return m_name;
}

void Robot::inform ( const Command & command )
{
    string commandName ( command.name() );
    // Hmmm... could have a map of command-name-to-method...
    if ( commandName == "place" )
    {
        place ( command.xpos(), command.ypos(), command.direction() );
    }
    else if ( commandName == "move" )
    {
        move();
    }
    else if ( commandName == "left" )
    {
        left();
    }
    else if ( commandName == "right" )
    {
        right();
    }
    else if ( commandName == "report" )
    {
        report();
    }
    // ... else other cases?
}

// How best to report failures etc?

void Robot::place ( int xpos, int ypos, Direction direction )
{
    // Need to ask about Constraints here.
    if ( xpos >= 0 && xpos < 5 && ypos >= 0 && ypos < 5 && validDirection ( direction ) )
    {
        m_xpos = xpos;
        m_ypos = ypos;
        m_direction = direction;
        m_onTable = true;
    }
    else
    {
        cerr << "Ignoring invalid PLACE co-ordinates" << endl;
    }
}

void Robot::move()
{
    if ( ! m_onTable )
    {
        cout << "Robot " << m_name << " is not on the table" << endl;
        return;
    }

    // Need to ask about Constraints here.
    bool validMove = true;
    switch ( m_direction )
    {
        case North:
        {
            if ( m_ypos < 4 )
            {
                ++m_ypos;
            }
            else
            {
                validMove = false;
            }
            break;
        }
        case West:
        {
            if ( m_xpos > 0 )
            {
                --m_xpos;
            }
            else
            {
                validMove = false;
            }
            break;
        }
        case South:
        {
            if ( m_ypos > 0 )
            {
                --m_ypos;
            }
            else
            {
                validMove = false;
            }
            break;
        }
        case East:
        {
            if ( m_xpos < 4 )
            {
                ++m_xpos;
            }
            else
            {
                validMove = false;
            }
            break;
        }
        case Invalid:
        {
            cout << "Attempt to move robot " << m_name << " without placing it first" << endl;
            break;
        }
        default:    // impossible, it's an enum
        {
            throw "impossible enum value";
            break;
        }
    }
    if ( ! validMove )
    {
        cout << "Ignoring attempt to move robot " << m_name << " off table" << endl;
    }
}

void Robot::left()
{
    if ( ! m_onTable )
    {
        cout << "Robot " << m_name << " is not on the table" << endl;
        return;
    }

    m_direction = ( m_direction == North ) ? West :
                  ( m_direction == West )  ? South :
                  ( m_direction == South ) ? East :
                  ( m_direction == East )  ? North :
                                             Invalid;
}

void Robot::right()
{
    if ( ! m_onTable )
    {
        cout << "Robot " << m_name << " is not on the table" << endl;
        return;
    }

    m_direction = ( m_direction == North ) ? East :
                  ( m_direction == East )  ? South :
                  ( m_direction == South ) ? West :
                  ( m_direction == West )  ? North :
                                             Invalid;
}

void Robot::report()
{
    if ( m_onTable )
    {
        cout << "Robot " << m_name << " is at x = " << m_xpos
             << ", y = " << m_ypos
             << ", facing " << directionAsString(m_direction) << endl;
    }
    else
    {
        cout << "Robot " << m_name << " is not on the table" << endl;
    }
}

//////////////////////////////////////////////////////////////////////////////
// Direction utilities.

static bool validDirection ( Direction direction )
{
    return direction == North || direction == West ||
           direction == South || direction == East;
}

static string directionAsString ( Direction direction )
{
    return ( direction == North ) ? "North" :
           ( direction == West )  ? "West" :
           ( direction == South ) ? "South" :
           ( direction == East )  ? "East" :
                                    "Invalid";
}

static Direction directionFromString ( const string & str )
{
    string lcString;
    // string.tolower by steam. Ugh.
    for ( const char * chPtr = str.c_str(); *chPtr != '\0'; ++chPtr )
    {
        lcString.push_back ( tolower( *chPtr ) );
    }
    return ( lcString == "n" || lcString == "north" ) ? North :
           ( lcString == "w" || lcString == "west" )  ? West :
           ( lcString == "s" || lcString == "south" ) ? South :
           ( lcString == "e" || lcString == "east" )  ? East :
                                                        Invalid;
}
