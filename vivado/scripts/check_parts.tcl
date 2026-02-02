# Check available parts
puts "Checking available Versal parts..."
set versal_parts [get_parts -filter {FAMILY =~ *versal*}]
puts "Found [llength $versal_parts] Versal parts"
puts ""
puts "VP (Premium) parts:"
foreach p $versal_parts {
    if {[string match "*vp*" $p]} {
        puts "  $p"
    }
}
puts ""
puts "VC (Core) parts:"
foreach p $versal_parts {
    if {[string match "*xcvc*" $p]} {
        puts "  $p"
    }
}
puts ""
puts "Checking boards..."
catch {
    set all_boards [get_board_parts *]
    puts "Total boards: [llength $all_boards]"
    foreach b $all_boards {
        if {[string match "*versal*" $b] || [string match "*vck*" $b] || [string match "*vpk*" $b] || [string match "*vek*" $b]} {
            puts "  $b"
        }
    }
}
exit
