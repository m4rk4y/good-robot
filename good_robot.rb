module Direction
    INVALID = -1
    NORTH = 0
    EAST = 1
    SOUTH = 2
    WEST = 3
    def Direction.to_s ( arg )
        { INVALID => :Invalid,
          NORTH   => :North,
          EAST    => :East,
          SOUTH   => :South,
          WEST    => :West
        }[arg]
    end
    # We want to have a way to raise an exception for invalid values.
    # Easiest with case rather than this hash, which simply returns nil.
    def Direction.from_s ( arg )
        { "N" => NORTH,
          "E" => EAST,
          "S" => SOUTH,
          "W" => WEST
        }[arg]
    end
    def Direction.left ( arg )
        { INVALID => INVALID,
          NORTH   => WEST,
          EAST    => NORTH,
          SOUTH   => EAST,
          WEST    => NORTH
        }[arg]
    end
    def Direction.right ( arg )
        { INVALID => INVALID,
          NORTH   => EAST,
          EAST    => SOUTH,
          SOUTH   => WEST,
          WEST    => SOUTH
        }[arg]
    end
    def Direction.move ( direction, xpos, ypos )
        # Could do this with two { Direction, increment } hashes.
        case direction
        when Direction::NORTH
            ypos = ypos + 1
        when Direction::EAST
            xpos = xpos + 1
        when Direction::SOUTH
            ypos = ypos - 1
        when Direction::WEST
            xpos = xpos - 1
        end
    [ xpos, ypos ]
    end
end

module Constraint
    @@constrainers = nil
    def addConstrainer ( object, function )
        @@constrainers = {} unless @@constrainers
        @@constrainers[object] = function
    end
    def checkConstraints( *args )
        # Returns true if all the constrainers say ok.
        # We filter out self at this point.
        @@constrainers.all?{ | key, value | key === self || key.send( value, self, args ) }
    end
end

class Game
    def initialize
        @listeners = []
        # I really don't like this split... game and robots and table should
        # auto-register what they respond to.
        @specificCommands = [ "create", "table", "report", "help", "quit" ]
        @forwardedCommands = [ "place", "move", "left", "right", "remove" ]
        @table = Table.new( 0, 0, 10, 10 )
        @robots = {}
        [ "Robbie", "Arthur" ].each { |name| @robots[name] = Robot.new( name ) }
    end

    def interpret( line )
        return if line.empty?
        # Line can be either <verb> [ <args> ]
        # or <known-robot> <verb> [ <args> ]
        #
        # <verb> must be a known command and possibly one that goes via the
        # generic forwarding.
        #
        # This isn't using a Broadcaster/Listener mechanism yet. Adding that
        # should help with the auto-registration and the rejection of invalid
        # commands.
        #
        verb, rest = line.split( /[ :,]+/, 2 )
        if @robots.keys.include?( verb )
            robot = @robots[verb]
            verb, rest = rest.split( /[ ,]+/, 2 )
        end
        qualifiers = rest.split( /[ ,]+/ ) if rest
        if @forwardedCommands.include?( verb )
            forward( func:verb, robot:robot, args:qualifiers )
        elsif ( @specificCommands.include?( verb ) && respond_to?( verb ) )
            send verb, robot:robot, args:qualifiers
        else
            puts "Don't know how to #{verb}"
        end
    end

    def create( robot: nil, **other_args )
        other_args[:args].each do |name|
            if @robots.keys.include? name
                puts "Robot #{name} already exists"
            else
                @robots[name] = Robot.new( name )
            end
        end
    end

    def table( **other_args )
        @table.set( *other_args[:args] )
    end

    def forward( func: nil, robot: nil, **other_args )
        if robot
            robot.send( func, *other_args[:args] )
        else
            @robots.values.each { |robot| robot.send( func, *other_args[:args] ) }
        end
    end

    def report( **other_args )
        @table.report
        @robots.values.each { |robot| robot.report }
    end

    def help( **other_args )
        puts "Valid commands are:"
        # Yeah I could concatenate the arrays and do one join, but what's the point?
        puts @forwardedCommands.join "\n"
        puts @specificCommands.join "\n"
    end

    def quit( **other_args )
        puts "Bye!"
        exit
    end
end

class Table
    include Constraint
    attr_reader :xmin, :ymin, :xmax, :ymax
    def initialize( xmin, ymin, xmax, ymax )
        set( xmin, ymin, xmax, ymax )
        addConstrainer( self, :constrain )
    end
    def constrain( *args )
        actor = args[0]
        new_xpos = args[1][0].to_i
        new_ypos = args[1][1].to_i
        ( @xmin ... @xmax ).include?( new_xpos ) && ( @ymin ... @ymax ).include?( new_ypos )
    end
    def set ( xmin, ymin, xmax, ymax )
        xmin = xmin.to_i
        xmax = xmax.to_i
        ymin = ymin.to_i
        ymax = ymax.to_i
        if ( xmin >= xmax || ymin >= ymax )
            puts "Invalid table co-ordinates [ ( #{xmin}, #{ymin} ), ( #{xmax}, #{ymax} ) ]"
            return
        end
        @xmin = xmin
        @ymin = ymin
        @xmax = xmax
        @ymax = ymax
    end
    def report
        puts "Table is at [ ( #{xmin}, #{ymin} ), ( #{xmax}, #{ymax} ) ]"
    end
end

class Robot
    include Constraint
    attr_reader :name, :xpos, :ypos, :direction, :on_table
    def initialize( name )
        @name = name
        @xpos = -1
        @ypos = -1
        @direction = Direction::INVALID
        @on_table = false
        addConstrainer( self, :constrain )
    end
    def constrain( *args )
        # Named arguments would help here.
        actor = args[0]
        new_xpos = args[1][0].to_i
        new_ypos = args[1][1].to_i
        ! @on_table || @xpos != new_xpos || @ypos != new_ypos
    end
    def place( new_xpos, new_ypos, direction )
        new_direction = Direction.from_s direction.upcase
        unless new_direction
            puts "Invalid direction #{direction}"
            return
        end
        if checkConstraints( new_xpos, new_ypos )
            @xpos = new_xpos.to_i
            @ypos = new_ypos.to_i
            @direction = Direction.from_s direction.upcase
            @on_table = true
        else
            puts "Cannot place Robot #{name} at ( #{new_xpos}, #{new_ypos} )"
        end
    end
    def move
        if ! @on_table
            puts "Cannot move Robot #{name} when not on table"
            return
        end
        new_xpos, new_ypos = Direction.move( @direction, @xpos, @ypos )
        if checkConstraints( new_xpos, new_ypos )
            @xpos = new_xpos
            @ypos = new_ypos
        else
            puts "Cannot move Robot #{name} to ( #{new_xpos}, #{new_ypos} )"
        end
    end
    def left
        if ! @on_table
            puts "Cannot turn Robot #{name} left when not on table"
            return
        end
        @direction = Direction.left ( @direction )
    end
    def right
        if ! @on_table
            puts "Cannot turn Robot #{name} right when not on table"
            return
        end
        @direction = Direction.right ( @direction )
    end
    def remove
        @on_table = false
    end
    def report
        if @on_table
            puts "#{name} is at ( #{xpos}, #{ypos} ) facing #{Direction.to_s direction}"
        else
            puts "#{name} is not on the table"
        end
    end
end

# Start here.

game = Game.new
# Ugly, but I want tty STDIN to be interactive.
if ( ARGV.empty? && STDIN.tty? )
    game.help
    # Relying on "quit" to do an exit...
    while true
        printf "? "
        game.interpret ( gets.chomp )
    end
else
    # This works with <script> <input-file>
    # but *not* with <script> < <input-file> (i.e. redirected STDIN),
    # despite what all the web pages say. Maybe it's a Windows-specific bug?
    # I can't find a way to get Ruby to read redirected STDIN at all :-(
    ARGF.each { |line| game.interpret line.chomp }
end