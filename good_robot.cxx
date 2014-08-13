/*
Classes:

    CommandStream: reads files or stdin until EOF and produces command lines

    Command: what a command line gets turned into

    CommandFactory: constructs Commands

    CommandListener: intermediary, constructed by GameObject in order to relay
                     Commands to the GameObject

    GameObject: interface for objects which wish to be notified of commands
                and/or constraint-verdict requests

    Robot: implementation of GameObject, which responds to Commands
            while observing Constraints

    RobotFactory: constructs Robots

    Table: implementation of GameObject, which does not respond to Commands
           but does provide a constraint-request verdict

    Broadcaster: broadcasts Commands to CommandListeners

    Interpreter: main controlling object which
                   - uses CommandStream to read lines
                   - creates Commands
                   - tells Broadcaster to broadcast the Commands

    Constraint: checks proposed moves etc; constructed by GameObject in order
                to relay constraint-verdict requests to the GameObject

    ConstraintFactory: constructs Constraints
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

// Should have map of name-to-function.
static void help()
{
    cerr << "create/move/left/right/report/remove/quit/help" << endl;
}

//////////////////////////////////////////////////////////////////////////////

class CommandStream
{
    public:
        CommandStream ( const char * fileName );
        CommandStream ( FILE * stream );
        ~CommandStream();
        bool getCommand ( string & command ) const;
    private:
        FILE * m_stream;
};

//////////////////////////////////////////////////////////////////////////////

class Command;  // forward declaration for GameObject

//////////////////////////////////////////////////////////////////////////////

class GameObject
{
    public:
        virtual void respond ( const Command & command ) = 0;
        virtual bool constraintDecider
        (   GameObject * object,
            int xpos,
            int ypos,
            Direction direction,
            bool onTable
        );
        virtual string name();
        virtual int xpos();
        virtual int ypos();
        virtual Direction direction();
        virtual bool onTable();
    protected:
        GameObject ( const char * name );
        string m_name;
        int m_xpos;
        int m_ypos;
        Direction m_direction;
        bool m_onTable;
};

typedef void (GameObject::*GameObjectResponder) ( const Command & command );

//////////////////////////////////////////////////////////////////////////////

class CommandListener
{
    public:
        CommandListener ( GameObject * object, GameObjectResponder responder );
        GameObject * object() const;
        virtual void inform ( const Command & command );
    private:
        GameObject * m_object;
        GameObjectResponder m_responder;
};

//////////////////////////////////////////////////////////////////////////////
// Capacity to invent new commands on the fly is limited.

class Command
{
    public:
        string name() const;
        int xpos() const;
        int ypos() const;
        Direction direction() const;
        string objectName() const;
        GameObject * gameObject() const;
    private:
        Command
        (   const char * name,
            int xpos = 0,
            int ypos = 0,
            Direction direction = Invalid,
            const char * objectName = 0,
            GameObject * gameObject = 0
        );
        string m_name;
        int m_xpos;
        int m_ypos;
        Direction m_direction;
        string m_objectName;
        GameObject * m_gameObject;
    friend class CommandFactory;
};

//////////////////////////////////////////////////////////////////////////////

class CommandFactory
{
    public:
        static CommandFactory * singleton();
        Command * createCommand ( const string & commandString ) const;
};

//////////////////////////////////////////////////////////////////////////////
// This is what a GameObject registers with the corresponding Constraint in
// order for the Constraint to give a verdict.

typedef bool (GameObject::*ConstraintDecider)
    ( GameObject * object, int xpos, int ypos, Direction direction, bool onTable );


class Robot : public GameObject
{
    public:
        void respond ( const Command & command );
        void place ( int xpos, int ypos, Direction direction );
        void move();
        void left();
        void right();
        void report();
        void remove();
        static Robot * find ( const string & robotName );
        bool constraintDecider
        (   GameObject * object,
            int xpos,
            int ypos,
            Direction direction,
            bool onTable
        );

    private:
        Robot ( const char * name );
    friend class RobotFactory;
};

//////////////////////////////////////////////////////////////////////////////

class RobotFactory
{
    public:
        static RobotFactory * singleton();
        Robot * createRobot ( const char * robotName );
        const map< string, Robot* > & robots() const;
    private:
        map< string, Robot* > m_robots;
};

//////////////////////////////////////////////////////////////////////////////
// Just to constrain objects to remain within the table limits.

class Table : public GameObject
{
    public:
        Table ( int xmin, int ymin, int xmax, int ymax );   // for now, saves having a factory
        void respond ( const Command & command );
        bool constraintDecider
        (   GameObject * object,
            int xpos,
            int ypos,
            Direction direction,
            bool onTable
        );
        int xmin();
        int ymin();
        int xmax();
        int ymax();
    private:
        int m_xmin;
        int m_ymin;
        int m_xmax;
        int m_ymax;
};

//////////////////////////////////////////////////////////////////////////////

class Interpreter
{
    public:
        Interpreter ( CommandStream & commandStream );
        void run();
    private:
        CommandStream & m_commandStream;
};

//////////////////////////////////////////////////////////////////////////////
// Interpreter(s) submit(s) stuff on to singleton Broadcaster which broadcasts
// to all Listeners. That's not going to scale well for enormous games, is it?

class Broadcaster
{
    public:
        static Broadcaster * singleton();
        void createCommandListener
        (   GameObject * object,
            GameObjectResponder responder
        );
        void broadcast ( const Command & command );
    private:
        static Broadcaster * m_broadcaster;
        vector< CommandListener* > m_commandListeners;
};

//////////////////////////////////////////////////////////////////////////////

class Constraint
{
    public:
        static bool acceptable
        (   GameObject * object,
            int xpos,
            int ypos,
            Direction direction,
            bool onTable
        );
    private:
        Constraint
        (   GameObject * object,
            ConstraintDecider decider
        );
        GameObject * m_object;
        ConstraintDecider m_decider;
    friend class ConstraintFactory;
};

//////////////////////////////////////////////////////////////////////////////

class ConstraintFactory
{
    public:
        static ConstraintFactory * singleton();
        Constraint * createConstraint
        (   GameObject * object,
            ConstraintDecider decider
        );
        const set< Constraint* > & constraints() const;
    private:
        set< Constraint* > m_constraints;
};

//////////////////////////////////////////////////////////////////////////////

extern int main ( int argc, char ** argv )
{
    try
    {
        Table theTable ( 0, 0, 10, 20 );
        // Also accepts robot-creation commands as input.
        Robot * robot1 = RobotFactory::singleton()->createRobot ( "Robbie" );
        Robot * robot2 = RobotFactory::singleton()->createRobot ( "Arthur" );

        // Read from supplied files or else stdin.
        if ( argc > 1 )
        {
            for ( int inx = 1; inx < argc; ++inx )
            {
                CommandStream commandStream ( argv[inx] );
                Interpreter interpreter ( commandStream );
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
        // A better way would be to use an iterator with a function to
        // encapsulate isspace or whatever... need to go back and look at the
        // STL API again.
        // Also this may not be necessary as we later use istringstream to
        // parse it.
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
    const char * objectName,
    GameObject * gameObject
)
  : m_name ( name ), m_xpos ( xpos ), m_ypos ( ypos ),
    m_direction ( direction ), m_objectName ( objectName ),
    m_gameObject ( gameObject )
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

GameObject * Command::gameObject() const
{
    return m_gameObject;
}

//////////////////////////////////////////////////////////////////////////////

CommandListener::CommandListener
(   GameObject * object,
    GameObjectResponder responder
) : m_object ( object ), m_responder ( responder )
{
}

GameObject * CommandListener::object() const
{
    return m_object;
}

void CommandListener::inform ( const Command & command )
{
    (m_object->*m_responder) ( command );
}

//////////////////////////////////////////////////////////////////////////////

CommandFactory * CommandFactory::singleton()
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
    // Shame this only splits on whitespace; we would like to split on ":"
    // too.
    istringstream parser ( commandString );
    string verb;
    parser >> verb;

    // First see if this is "<known-robot-name>:".
    // The manipulation here is easier in C++11.
    Robot * knownRobot = 0;
    if ( verb[verb.length()-1] == ':' )
    {
        knownRobot = Robot::find ( verb.substr(0,verb.length()-1) );
        if ( knownRobot != 0 )
        {
            // Move on to actual verb.
            parser >> verb;
        }
        // else verb ends with a colon which I imagine will fail later, but we
        // certainly can't assert that here.
    }

    string lcVerb;
    // string.tolower by steam. Ugh.
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
                         newXpos, newYpos, newDirection, objectName.c_str(),
                         knownRobot );
}

//////////////////////////////////////////////////////////////////////////////

Interpreter::Interpreter ( CommandStream & commandStream )
  : m_commandStream ( commandStream )
{
}

void Interpreter::run()
{
    string commandString;
    while ( m_commandStream.getCommand ( commandString ) )
    {
        try
        {
            Command * command =
                CommandFactory::singleton()->createCommand ( commandString );
            scoped_ptr<Command> freeCommand ( command );
            // Now this switching is ugly...
            if ( command->name() == "create" )
            {
                RobotFactory::singleton()->createRobot ( command->objectName().c_str() );
            }
            else if ( command->name() == "help" )
            {
                help();
            }
            else if ( command->name() == "quit" )
            {
                return;
            }
            else
            {
                Broadcaster::singleton()->broadcast ( *command );
            }
        }
        catch ( ... )   // should invent specific exception
        {
            cerr << "Failed to create or run command \"" << commandString << "\"" << endl;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

Broadcaster * Broadcaster::singleton()
{
    static Broadcaster * broadcaster = 0;
    if ( broadcaster == 0 )
    {
        broadcaster = new Broadcaster;
    }
    return broadcaster;
}

// For completeness, ought to have remove as well.
void Broadcaster::createCommandListener
(   GameObject * object,
    GameObjectResponder responder
)
{
    m_commandListeners.push_back ( new CommandListener ( object, responder ) );
}

void Broadcaster::broadcast ( const Command & command )
{
    GameObject * gameObject = command.gameObject();
#if 0
    cerr << "Command: \"" << command.name() << "\""
         << " " << command.xpos()
         << " " << command.ypos()
         << " " << directionAsString(command.direction())
         << " \"" << command.objectName() << "\""
         << " " << command.gameObject() << endl;
#endif
    for ( std::vector< CommandListener* >::iterator iter = m_commandListeners.begin();
          iter != m_commandListeners.end(); ++iter )
    {
        // cerr << "Listener: " << (*iter) << endl;
        // Broadcast to all listeners or just the one that the Command
        // specifies.
        if ( gameObject == 0 || gameObject == (*iter)->object() )
        {
            (*iter)->inform ( command );
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

GameObject::GameObject ( const char * name )
 : m_name ( name ),
   m_xpos ( 0 ),            // }
   m_ypos ( 0 ),            // } but irrelevant since not on table
   m_direction ( Invalid ), // }
   m_onTable ( false )
{
}

string GameObject::name()
{
    return m_name;
}

int GameObject::xpos()
{
    return m_xpos;
}

int GameObject::ypos()
{
    return m_ypos;
}

Direction GameObject::direction()
{
    return m_direction;
}

bool GameObject::onTable()
{
    return m_onTable;
}

// Is the proposed placement of the given object acceptable to me?
bool GameObject::constraintDecider
(   GameObject * object,
    int xpos,
    int ypos,
    Direction direction,
    bool onTable
)
{
    // cerr << "GameObject::constraintDecider " << name() << endl;
    return true;    // notional GameObject doesn't care
}

//////////////////////////////////////////////////////////////////////////////

Table::Table ( int xmin, int ymin, int xmax, int ymax )
 : GameObject ( "Table" ),
   m_xmin ( xmin ), m_ymin ( ymin ), m_xmax ( xmax ), m_ymax ( ymax )
{
    ConstraintFactory::singleton()->createConstraint ( this, GameObject::constraintDecider );
}

void Table::respond ( const Command & command )
{
    // No-op.
}

bool Table::constraintDecider
(   GameObject * object,
    int xpos,
    int ypos,
    Direction direction,
    bool onTable
)
{
    // cerr << "Table::constraintDecider " << name() << endl;
    // cerr << "Ask about " << object->name() << " with " << xpos << ", " << ypos << endl;
    // cerr << "I'm at " << m_xmin << ", " << m_ymin << " to " << m_xmax << ", " << m_ymax << endl;

    // It's ok if it's the table itself or if it's not on the table or if it's
    // within the table boundaries.
    return object == this ||
           ( ! onTable ) ||
           ( m_xmin <= xpos && xpos < m_xmax &&
             m_ymin <= ypos && ypos < m_ymax
           );
}

int Table::xmin()
{
    return m_xmin;
}

int Table::ymin()
{
    return m_ymin;
}

int Table::xmax()
{
    return m_xmax;
}

int Table::ymax()
{
    return m_ymax;
}

//////////////////////////////////////////////////////////////////////////////

RobotFactory * RobotFactory::singleton()
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
    m_robots.insert ( pair< string, Robot* > ( robotName, robot ) );
    return robot;
}

const map< string, Robot* > & RobotFactory::robots() const
{
    return m_robots;
}

//////////////////////////////////////////////////////////////////////////////

Robot::Robot ( const char * name )
 : GameObject ( name )
{
    // This had better all be single-threaded, otherwise someone might
    // broadcast a command to (or ask for a constraint-verdict from) this
    // not-yet-fully-formed Robot.
    Broadcaster::singleton()->createCommandListener ( this, GameObject::respond );
    ConstraintFactory::singleton()->createConstraint ( this, GameObject::constraintDecider );
}

void Robot::respond ( const Command & command )
{
    const string & commandName ( command.name() );

    // Hmmm... could have a map of command-name-to-method... although only if
    // all the relevant methods have the same signature. This would be so much
    // easier in Ruby, as I could just use send().
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
    else if ( commandName == "remove" )
    {
        remove();
    }
    else
    {
        cerr << "Invalid command " << commandName << endl;
        help();
    }
}

// Return named robot or 0.
Robot * Robot::find ( const string & robotName )
{
    const map< string, Robot* > & robots = RobotFactory::singleton()->robots();
    map< string, Robot* >::const_iterator iter = robots.find ( robotName );
    return ( iter == robots.end() ) ? 0 : iter->second;
}

// How best to report failures etc?

void Robot::place ( int xpos, int ypos, Direction direction )
{
    if ( Constraint::acceptable ( this, xpos, ypos, direction, true ) )
    {
        m_xpos = xpos;
        m_ypos = ypos;
        m_direction = direction;
        m_onTable = true;
    }
    else
    {
        cout << "Ignoring attempt to place robot " << m_name << " in invalid position" << endl;
    }
}

void Robot::move()
{
    if ( ! m_onTable )
    {
        cout << "Robot " << m_name << " is not on the table" << endl;
        return;
    }

    int newXpos = m_xpos;
    int newYpos = m_ypos;

    switch ( m_direction )
    {
        case North:
        {
            ++newYpos;
            break;
        }
        case West:
        {
            --newXpos;
            break;
        }
        case South:
        {
            --newYpos;
            break;
        }
        case East:
        {
            ++newXpos;
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

    if ( Constraint::acceptable ( this, newXpos, newYpos, m_direction, true ) )
    {
        m_xpos = newXpos;
        m_ypos = newYpos;
    }
    else
    {
        cout << "Ignoring attempt to move robot " << m_name << " to invalid position" << endl;
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

void Robot::remove()
{
    m_onTable = false;
    m_direction = Invalid;  // for good measure
}

// Is the proposed placement of the given object acceptable to me?
bool Robot::constraintDecider
(   GameObject * object,
    int xpos,
    int ypos,
    Direction direction,
    bool onTable
)
{
    // cerr << "Robot::constraintDecider " << name() << endl;

    // If I'm being asked about myself then I don't care.
    // If I'm not on the table or it's not on the table then I don't care.
    // If I am on the table, then I only care about not being in the same
    // place.
    return ( this == object ) || ( ! m_onTable ) || ( ! onTable ) ||
           ( m_xpos != xpos ) || ( m_ypos != ypos );
}

//////////////////////////////////////////////////////////////////////////////

ConstraintFactory * ConstraintFactory::singleton()
{
    static ConstraintFactory * factory = 0;
    if ( factory == 0 )
    {
        factory = new ConstraintFactory;
    }
    return factory;
}

Constraint * ConstraintFactory::createConstraint
(   GameObject * object,
    ConstraintDecider decider
)
{
    Constraint * constraint = new Constraint ( object, decider );
    m_constraints.insert ( constraint );
    return constraint;
}

const set< Constraint* > & ConstraintFactory::constraints() const
{
    return m_constraints;
}

//////////////////////////////////////////////////////////////////////////////
Constraint::Constraint ( GameObject * object, ConstraintDecider decider )
  : m_object ( object ), m_decider ( decider )
{
}

bool Constraint::acceptable
(   GameObject * object,
    int xpos,
    int ypos,
    Direction direction,
    bool onTable
)
{
    // Check sane direction.
    if ( ! validDirection ( direction ) )
    {
        return false;
    }

    // Check against all the registered Constraints.
    const set< Constraint* > constraints = ConstraintFactory::singleton()->constraints();
    for ( set< Constraint* >::const_iterator iter = constraints.begin();
          iter != constraints.end(); ++iter
        )
    {
        GameObject * constrainerObject = (*iter)->m_object;
        ConstraintDecider decider = (*iter)->m_decider;
        if ( ! (constrainerObject->*decider) ( object, xpos, ypos, direction, onTable ) )
        {
            return false;
        }
    }

    // Looks good, then.
    return true;
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
