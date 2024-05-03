# Remove code enclosed by "BEGIN FULL-RUNTIME" and "END FULL-RUNTIME" to
# create the lite-only version of a test file.

BEGIN {
  in_full_runtime = 0;
}

/BEGIN FULL-RUNTIME/ {
  in_full_runtime = 1;
  next;
}

/END FULL-RUNTIME/ {
  in_full_runtime = 0;
  next;
}

in_full_runtime {
  # Skip full runtime code path.
  next;
}

{
  print;
}
