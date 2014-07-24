#!/usr/bin/ruby

puts "set width 0
set height 0
set verbose off\n\n"

IO.popen("nm -S /tmp/upb-jit-code.so").each_line { |line|
  # Input lines look like this:
  #   000000000000575a T X.0x10.OP_CHECKDELIM
  #
  # For each one we want to emit a command that looks like:
  #   b X.0x10.OP_CHECKDELIM
  #   commands
  #     silent
  #     printf "buf_ofs=%d data_rem=%d delim_rem=%d X.0x10.OP_CHECKDELIM\n", $rbx - (long)((upb_pbdecoder*)($r15))->buf, $r12 - $rbx, $rbp - $rbx
  #     continue
  #   end

  parts = line.split
  next if parts[1] != "T"
  sym = parts[2]
  next if sym !~ /X\./;
  if sym =~ /OP_/ then
    printcmd = "printf \"buf_ofs=%d data_rem=%d delim_rem=%d #{sym}\\n\", $rbx - (long)((upb_pbdecoder*)($r15))->buf, $r12 - $rbx, $rbp - $rbx"
  elsif sym =~ /enterjit/ then
    printcmd = "printf \"#{sym} bytes=%d\\n\", $rcx"
  else
    printcmd = "printf \"#{sym}\\n\""
  end
  puts "b #{sym}
commands
  silent
  #{printcmd}
  continue
end\n\n"
}
