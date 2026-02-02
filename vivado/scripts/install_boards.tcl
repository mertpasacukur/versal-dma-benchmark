# Install VPK120 board files from Xilinx Board Store
puts "Refreshing board catalog..."
xhub::refresh_catalog [xhub::get_xstores xilinx_board_store]

puts ""
puts "Available VPK120 boards:"
foreach item [xhub::get_xitems *vpk120*] {
    puts "  $item"
}

puts ""
puts "Installing VPK120 board files..."
catch {
    xhub::install [xhub::get_xitems xilinx.com:xilinx_board_store:vpk120:*]
} result
puts "Result: $result"

puts ""
puts "Checking installed boards..."
set boards [get_board_parts *vpk* -quiet]
if {[llength $boards] > 0} {
    puts "VPK120 boards found:"
    foreach b $boards {
        puts "  $b"
    }
} else {
    puts "No VPK120 boards found after installation"
}

puts ""
puts "Done"
