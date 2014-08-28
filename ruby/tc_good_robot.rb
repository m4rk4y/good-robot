require_relative "good_robot"
require "minitest/autorun"

def caller ( context, *args )
    puts "Context: #{context}"
    puts "Stuff: #{args}"
end


class MyGame < Game
    def initialize
        super
    end
    def tell_user ( *args )
        args.each { |arg| puts arg }
    end
end





# Monkey-patch Game:

# class Game
#     def tell_user ( *args )
#         puts "Monkey-patched tell_user begins"
#         puts "Output:"
#         args.each { |arg| puts arg }
#         puts "Monkey-patched tell_user ends"
#     end
# end

class TestXxx < Minitest::Test
    attr_accessor :output
    def capturer(stuff)
        lambda { puts "Self #{self} captured: #{stuff}" }
    end
    def setup
        @tell_user_proc = Proc.new do
            def tell_user ( *args )
                args.each { |arg| puts arg }
            end
        end
        @tell_user_proc2 = Proc.new do
            def tell_user ( *args )
                args.each { |arg| puts arg }
            end
        end
        @thing = self
        tell_user = Proc.new do |args|
            caller( @thing, *args )
        end
        # Game.class_eval( &@tell_user_proc )

        @game = Game.new
        local_self = self
        # @game.instance_exec( &@tell_user_proc2 )
        @game.instance_eval do
            @game_copy_of_local_self = local_self
            def tell_user ( *args )
                args.each { |arg|
                    @game_copy_of_local_self.output << arg
                }
            end
        end
        @output = ""
    end
    def test_help
        @output.clear
        @game.help
        puts "Got: #{@output}"
        assert_match /create/, @output
    end
    def test_place
        @output.clear
        @game.interpret "Robbie place 2 3 N"
        @game.interpret "Robbie report"
        puts "Got: #{@output}"
        assert_match /Robbie is at \( 2, 3 \) facing North/, @output
    end
    def teardown
    end
end