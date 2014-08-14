/*
Toy robot game.

Synopsis:

    Accepts commands (from stdin or named input files):
        table <xmin> <ymin> <xmax> <ymax>
        create <new-robot-name>
        [ <robot-name>: ] place <x> <y> <direction>
        [ <robot-name>: ] move
        [ <robot-name>: ] left
        [ <robot-name>: ] right
        [ <robot-name>: ] report
        [ <robot-name>: ] remove
        quit
        help

    Commands are case-insensitive. Robot names are case-sensitive.

    Arguments (for "table" and "place") can be comma- or space-delimited.

    Starts with a table at [ ( 0, 0 ), ( 10, 10 ) ] but "table" resizes this.

    Starts with two robots called "Robbie" and "Arthur", not on the table.

    place/move/left/right/report/remove act on all robots or just the named one.

    Robots cannot be moved past the table boundaries, nor onto an occupied position.

    The table can however be resized on the fly so that a Robot can suddenly find
    itself outside the boundaries. Please don't do this as it upsets the
    Robot's world view :-)

Flow:

(1) main loop:

    while read-command
        if help
            help
        elsif quit
            quit
        elsif create
            create-robot
                register-as-command-listener
                register-as-constrainer
        else
            broadcast-command-to-all-listeners
                listeners.each
                    relay-command-to-listening-object
        endif
    endwhile

(2) command:

    constrainers.each
        check-this-proposal
    if ok
        do it
    endif

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

    Table: implementation of GameObject, which responds to (very few) Commands
           and provides a constraint-request verdict

    Interpreter: main controlling object which
                   - uses CommandStream to read lines
                   - creates Commands
                   - tells Broadcaster to broadcast the Commands

    Broadcaster: broadcasts Commands to CommandListeners

    Constraint: checks proposed moves etc; constructed by GameObject in order
                to relay constraint-verdict requests to the GameObject

    ConstraintFactory: constructs Constraints

    Tokeniser: DIY stand-in to handle comma and whitespace (because
               istringstream parsing only handles whitespace)

    Various Exception classes.
*/

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace std;

#include "my_scoped_ptr.hxx"
using namespace scoping;

enum Direction { Invalid, North, East, South, West };
static bool validDirection ( Direction direction );
static string directionAsString ( Direction direction );
static Direction directionFromString ( const string & str );
static void help();
static string lowerCaseString ( const string & str );

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

class GameObject;   // forward declaration

class Command
{
    public:
        string name() const;
        string qualifiers() const;
        GameObject * gameObject() const;
    private:
        Command
        (   const string & name,
            const string & qualifers,
            GameObject * gameObject = 0
        );
        string m_name;
        string m_qualifiers;
        GameObject * m_gameObject;
    friend class CommandFactory;
};

//////////////////////////////////////////////////////////////////////////////

class CommandFactory
{
    public:
        static CommandFactory * singleton();
        void checkValidCommand ( const string & command ) const;
        const vector<string> & validCommands() const;
        void setValidCommands ( const vector<string> & commands );
        Command * createCommand ( const string & commandString ) const;
    private:
        vector<string> m_validCommands;
};

//////////////////////////////////////////////////////////////////////////////
// This is what a GameObject registers with the Broadcaster in order to
// receive Commands.

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
        GameObject ( const string & name );
        string m_name;
        int m_xpos;
        int m_ypos;
        Direction m_direction;
        bool m_onTable;
};

//////////////////////////////////////////////////////////////////////////////
// This is what a GameObject registers with the corresponding Constraint in
// order for the Constraint to give a verdict.

typedef bool (GameObject::*ConstraintDecider)
    ( GameObject * object, int xpos, int ypos, Direction direction, bool onTable );

//////////////////////////////////////////////////////////////////////////////

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
        Robot ( const string & name );
    friend class RobotFactory;
};

//////////////////////////////////////////////////////////////////////////////

class RobotFactory
{
    public:
        static RobotFactory * singleton();
        Robot * createRobot ( const string & robotName );
        const map< string, Robot* > & robots() const;
    private:
        map< string, Robot* > m_robots;
};

//////////////////////////////////////////////////////////////////////////////
// Just to constrain objects to remain within the table limits.

class Table : public GameObject
{
    public:
        static void setTable ( int xmin, int ymin, int xmax, int ymax );
        void respond ( const Command & command );
        void report();
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
        Table ( int xmin, int ymin, int xmax, int ymax );
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

class Tokeniser
{
    public:
        Tokeniser ( const string & stringToParse, const string & separators );
        string nextToken();
    private:
        string m_stringToParse;
        string m_separators;
        size_t m_currentPosition;
};

//////////////////////////////////////////////////////////////////////////////

class InvalidCommandException : public exception
{
    public:
        InvalidCommandException ( const string & message )
          : exception ( message.c_str() ) {}
};

class InvalidDirectionException : public exception
{
    public:
        InvalidDirectionException
        (   const string & directionString,
            const string & message
        ) : exception ( message.c_str() ), m_directionString ( directionString ) {}
        const string & directionString() const { return m_directionString; }
    private:
        string m_directionString;
};

//////////////////////////////////////////////////////////////////////////////

extern int main ( int argc, char ** argv )
{
    try
    {
        vector<string> validCommands;
        validCommands.push_back ( "create" );
        validCommands.push_back ( "table" );
        validCommands.push_back ( "place" );
        validCommands.push_back ( "move" );
        validCommands.push_back ( "left" );
        validCommands.push_back ( "right" );
        validCommands.push_back ( "report" );
        validCommands.push_back ( "remove" );
        validCommands.push_back ( "help" );
        validCommands.push_back ( "quit" );
        CommandFactory::singleton()->setValidCommands ( validCommands );
        Table::setTable ( 0, 0, 10, 10 );
        RobotFactory::singleton()->createRobot ( "Robbie" );
        RobotFactory::singleton()->createRobot ( "Arthur" );

        // Be kind and emit help message first.
        help();

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
    catch ( const exception & error )
    {
        cerr << "Caught exception: " << error.what() << endl;
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
        throw exception ( errorStream.str().c_str() );
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
        command = buffer;
        // Trim trailing newline.
        command.resize ( command.length()-1 );
        if ( ! command.empty() )
        {
            return true;
        }
        // else content-free line so try for the next one
    }
}

//////////////////////////////////////////////////////////////////////////////

Command::Command
(   const string & name,
    const string & qualifiers,
    GameObject * gameObject
)
  : m_name ( name ), m_qualifiers ( qualifiers ),
    m_gameObject ( gameObject )
{
}

string Command::name() const
{
    return m_name;
}

string Command::qualifiers() const
{
    return m_qualifiers;
}

GameObject * Command::gameObject() const
{
    return m_gameObject;
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
        // else verb ends with a colon which I imagine will fail quite soon.
    }

    string lcVerb ( lowerCaseString ( verb ) );
    checkValidCommand ( lcVerb );

    // Store the rest of the command for later command-dependent parsing.
    string restOfString;
    getline ( parser, restOfString );
    return new Command ( lcVerb, restOfString, knownRobot );
}

void CommandFactory::setValidCommands ( const vector<string> & commands )
{
    m_validCommands = commands;
}

const vector<string> & CommandFactory::validCommands() const
{
    return m_validCommands;
}

void CommandFactory::checkValidCommand ( const string & command ) const
{
    for ( vector<string>::const_iterator iter = m_validCommands.begin();
          iter != m_validCommands.end(); ++iter
        )
    {
        if ( *iter == command )
        {
            return;
        }
    }
    throw InvalidCommandException ( command.c_str() );
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

GameObject::GameObject ( const string & name )
 : m_name ( name ),
   m_xpos ( 0 ),            // }
   m_ypos ( 0 ),            // } but irrelevant since not on table
   m_direction ( Invalid ), // }
   m_onTable ( false )
{
    // It would be tempting to put these here, but that would preclude derived
    // classes from choosing *not* to respond and/or constrain.
    // Broadcaster::singleton()->createCommandListener ( this, GameObject::respond );
    // ConstraintFactory::singleton()->createConstraint ( this, GameObject::constraintDecider );
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
    return true;    // notional GameObject doesn't care
}

//////////////////////////////////////////////////////////////////////////////

Robot::Robot ( const string & name )
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
        // DIY parsing to handle comma and whitespace.
        Tokeniser tokeniser ( command.qualifiers(), ", " );
        string newXposToken = tokeniser.nextToken();
        string newYposToken = tokeniser.nextToken();
        string newDirectionToken = tokeniser.nextToken();

        // Got tokens, now convert them.
        int newXpos = atoi ( newXposToken.c_str() );
        int newYpos = atoi ( newYposToken.c_str() );
        Direction newDirection ( directionFromString ( newDirectionToken ) );
        if ( newDirection == Invalid )
        {
            throw InvalidDirectionException ( newDirectionToken, "place" );
        }
        place ( newXpos, newYpos, newDirection );
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
            throw exception ( "impossible enum value" );
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
    // If I'm being asked about myself then I don't care.
    // If I'm not on the table or it's not on the table then I don't care.
    // If I am on the table, then I only care about not being in the same
    // place.
    return ( this == object ) || ( ! m_onTable ) || ( ! onTable ) ||
           ( m_xpos != xpos ) || ( m_ypos != ypos );
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

Robot * RobotFactory::createRobot ( const string & robotName )
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

Table::Table ( int xmin, int ymin, int xmax, int ymax )
 : GameObject ( "Table" ),
   m_xmin ( xmin ), m_ymin ( ymin ), m_xmax ( xmax ), m_ymax ( ymax )
{
    Broadcaster::singleton()->createCommandListener ( this, GameObject::respond );
    ConstraintFactory::singleton()->createConstraint ( this, GameObject::constraintDecider );
}

void Table::setTable ( int xmin, int ymin, int xmax, int ymax )
{
    static Table * table = 0;
    if ( table == 0 )
    {
        table = new Table ( xmin, ymin, xmax, ymax );
    }
    else
    {
        table->m_xmin = xmin;
        table->m_ymin = ymin;
        table->m_xmax = xmax;
        table->m_ymax = ymax;
    }
}

void Table::respond ( const Command & command )
{
    const string & commandName ( command.name() );

    if ( commandName == "report" )
    {
        report();
    }
    else if ( commandName == "table" )
    {
        // DIY parsing to handle comma and whitespace.
        Tokeniser tokeniser ( command.qualifiers(), ", " );
        string newXminToken = tokeniser.nextToken();
        string newYminToken = tokeniser.nextToken();
        string newXmaxToken = tokeniser.nextToken();
        string newYmaxToken = tokeniser.nextToken();

        // Got tokens, now convert them.
        int newXmin = atoi ( newXminToken.c_str() );
        int newYmin = atoi ( newYminToken.c_str() );
        int newXmax = atoi ( newXmaxToken.c_str() );
        int newYmax = atoi ( newYmaxToken.c_str() );
        setTable ( newXmin, newYmin, newXmax, newYmax );
    }
}

void Table::report()
{
    cout << "Table limits are: [ ( " << m_xmin << ", " << m_ymin << " ), ( "
         << m_xmax << ", " << m_ymax << " ) ]" << endl;
}

bool Table::constraintDecider
(   GameObject * object,
    int xpos,
    int ypos,
    Direction direction,
    bool onTable
)
{
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
                string newObjectName;
                istringstream parser ( command->qualifiers() );
                parser >> newObjectName;
                RobotFactory::singleton()->createRobot ( newObjectName );
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
        catch ( const string & error )
        {
            cerr << "Exception: " << error << endl;
        }
        catch ( const char * error )
        {
            cerr << "Exception: " << error << endl;
        }
        catch ( const InvalidCommandException & error )
        {
            cerr << "Invalid command: " << error.what() << endl;
            help();
        }
        catch ( const InvalidDirectionException & error )
        {
            cerr << "Invalid direction " << error.directionString() << " for " << error.what() << endl;
        }
        catch ( const exception & error )
        {
            cerr << "Caught exception: " << error.what() << endl;
        }
        catch ( ... )
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
    for ( vector< CommandListener* >::iterator iter = m_commandListeners.begin();
          iter != m_commandListeners.end(); ++iter )
    {
        // Broadcast to all listeners or just the one that the Command
        // specifies.
        if ( gameObject == 0 || gameObject == (*iter)->object() )
        {
            (*iter)->inform ( command );
        }
    }
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

Tokeniser::Tokeniser ( const string & stringToParse, const string & separators )
  : m_stringToParse ( stringToParse ),
    m_separators ( separators ),
    m_currentPosition ( 0 )
{
}

string Tokeniser::nextToken()
{
    size_t nextTokenStart = m_stringToParse.find_first_not_of ( m_separators, m_currentPosition );
    if ( nextTokenStart == string::npos )
    {
        return "";  // to signal EOS
    }
    m_currentPosition = m_stringToParse.find_first_of ( m_separators, nextTokenStart+1 );
    return m_currentPosition == string::npos ?
           m_stringToParse.substr ( nextTokenStart ) :
           m_stringToParse.substr ( nextTokenStart, m_currentPosition-nextTokenStart );
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
    string lcString ( lowerCaseString ( str ) );
    return ( lcString == "n" || lcString == "north" ) ? North :
           ( lcString == "w" || lcString == "west" )  ? West :
           ( lcString == "s" || lcString == "south" ) ? South :
           ( lcString == "e" || lcString == "east" )  ? East :
                                                        Invalid;
}

//////////////////////////////////////////////////////////////////////////////
// Some helpers.

// Should have map of name-to-function really.
static void help()
{
    cerr << "Valid commands are:" << endl;
    const vector<string> & validCommands = CommandFactory::singleton()->validCommands();
    for ( vector<string>::const_iterator iter = validCommands.begin();
          iter != validCommands.end(); ++iter
        )
    {
        cerr << *iter << endl;
    }
}

// string.tolower by steam. Ugh.
static string lowerCaseString ( const string & str )
{
    string lcStr;
    for ( const char * chPtr = str.c_str(); *chPtr != '\0'; ++chPtr )
    {
        lcStr.push_back ( tolower( *chPtr ) );
    }
    return lcStr;
}
