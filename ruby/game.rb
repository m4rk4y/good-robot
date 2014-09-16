require_relative "robot.rb"
require_relative "table.rb"

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

    def tell_user( *args )
        puts( *args )
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
        begin
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
                tell_user "Don't know how to #{verb}"
            end
        rescue StandardError => exception
            tell_user exception.inspect
        end
    end

    def create( robot: nil, **other_args )
        other_args[:args].each do |name|
            if @robots.keys.include? name
                tell_user "Robot #{name} already exists"
            else
                @robots[name] = Robot.new( name )
            end
        end
    end

    def table( **other_args )
        tell_user @table.set( *other_args[:args] )
    end

    def forward( func: nil, robot: nil, **other_args )
        if robot
            tell_user( robot.send( func, *other_args[:args] ) )
        else
            @robots.values.each { |robot| tell_user( robot.send( func, *other_args[:args] ) ) }
        end
    end

    def report( **other_args )
        tell_user @table.report
        @robots.values.each { |robot| tell_user robot.report }
    end

    def help( **other_args )
        tell_user "Valid commands are:"
        # Yeah I could concatenate the arrays and do one join, but what's the point?
        tell_user @forwardedCommands.join "\n"
        tell_user @specificCommands.join "\n"
    end

    def quit( **other_args )
        tell_user "Bye!"
        exit
    end
end
