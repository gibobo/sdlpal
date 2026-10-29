Get-ChildItem .\src -Recurse -Include *.h, *.cpp, *.c | Foreach-Object {
  # CAVEAT: There's a slight risk of data loss if writing back to the input
  #         file is interrupted.
  [System.IO.File]::WriteAllText(
    $_.FullName,
    [System.IO.File]::ReadAllText($_.FullName)
  )
}