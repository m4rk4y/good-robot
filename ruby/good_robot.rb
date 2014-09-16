require_relative "game.rb"

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