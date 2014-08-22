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
          WEST    => SOUTH
        }[arg]
    end
    def Direction.right ( arg )
        { INVALID => INVALID,
          NORTH   => EAST,
          EAST    => SOUTH,
          SOUTH   => WEST,
          WEST    => NORTH
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
        @@constrainers ||= {}
        @@constrainers[object] = function
    end
    def checkConstraints( *args )
        # Returns true if all the constrainers say ok.
        # We filter out self at this point.
        @@constrainers.all?{ | key, value | key == self || key.send( value, self, args ) }
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
        @robots.values.each( &:report )
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
        new_xpos, new_ypos = args[1][0].to_i, args[1][1].to_i
        ( @xmin ... @xmax ).include?( new_xpos ) && ( @ymin ... @ymax ).include?( new_ypos )
    end
    def set ( xmin, ymin, xmax, ymax )
        xmin, xmax, ymin, ymax = xmin.to_i, xmax.to_i, ymin.to_i, ymax.to_i
        if ( xmin < xmax && ymin < ymax )
            @xmin, @ymin, @xmax, @ymax = xmin, ymin, xmax, ymax
            report
        else
            puts "Invalid table co-ordinates [ ( #{xmin}, #{ymin} ), ( #{xmax}, #{ymax} ) ]"
        end
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
        @xpos, @ypos = -1, -1
        @direction = Direction::INVALID
        @on_table = false
        addConstrainer( self, :constrain )
        report
    end
    def constrain( *args )
        # Named arguments would help here.
        actor = args[0]
        new_xpos, new_ypos = args[1][0].to_i, args[1][1].to_i
        ! @on_table || @xpos != new_xpos || @ypos != new_ypos
    end
    def place( new_xpos, new_ypos, direction )
        new_direction = Direction.from_s direction.upcase
        if new_direction
            if checkConstraints( new_xpos, new_ypos )
                @xpos, @ypos = new_xpos.to_i, new_ypos.to_i
                @direction = Direction.from_s direction.upcase
                @on_table = true
                report
            else
                puts "Cannot place Robot #{name} at ( #{new_xpos}, #{new_ypos} )"
            end
        else
            puts "Invalid direction #{direction}"
        end
    end
    def move
        if @on_table
            new_xpos, new_ypos = Direction.move( @direction, @xpos, @ypos )
            if checkConstraints( new_xpos, new_ypos )
                @xpos, @ypos = new_xpos, new_ypos
                report
            else
                puts "Cannot move Robot #{name} to ( #{new_xpos}, #{new_ypos} )"
            end
        else
            puts "Cannot move Robot #{name} when not on table"
        end
    end
    def left
        if @on_table
            @direction = Direction.left ( @direction )
            report
        else
            puts "Cannot turn Robot #{name} left when not on table"
        end
    end
    def right
        if @on_table
            @direction = Direction.right ( @direction )
            report
        else
            puts "Cannot turn Robot #{name} right when not on table"
        end
    end
    def remove
        @on_table = false
        report
    end
    def report
        if @on_table
            puts "#{name} is at ( #{xpos}, #{ypos} ) facing #{Direction.to_s direction}"
        else
            puts "#{name} is not on the table"
        end
    end
end

# Start execution here.

if __FILE__ == $0
    game = Game.new
    # Ugly, but I want tty STDIN to be interactive.
    if ( ARGV.empty? && STDIN.tty? )
        game.help
        # Relying on "quit" to do an exit...
        while true
            printf "? "
            game.interpret gets.chomp
        end
    else
        # This works with <script> <input-file>
        # but *not* with <script> < <input-file> (i.e. redirected STDIN),
        # despite what all the web pages say. Well it fails only Windows XP as far
        # as I can tell, and I can't find a way to get Ruby to read redirected
        # STDIN at all :-(
        ARGF.each { |line| game.interpret line.chomp }
    end
end