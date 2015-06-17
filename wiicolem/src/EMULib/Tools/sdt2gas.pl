#!/usr/bin/perl

sub ProcFile
{
  local $FileName = shift;
  local $Macro    = 0;
  local $Out;
  local *F;

  open(F,"<".$FileName) or die;

  while(<F>)
  {
    # Parse RN lines
    if(/^\s*(\S+)\s+RN\s+(\S+)/)     { $RN{$1}=$2; }
    # Parse INCLUDE lines
    elsif(/^\s+INCLUDE\s+(\S+)/)     { ProcFile($1); }
    # Parse MACRO lines
    elsif(/^\s+MACRO\s+/)            { $Macro=1; }
    # Parse AREA lines
    elsif(/^\s+AREA\s+/)             { print "\t.text\n"; }
    # Parse EQU lines
    elsif(/^\s*(\S+)\s+EQU\s+(\S+)/) { print "\t.equ\t$1,$2\n"; }
    # Print all other lines
    else
    {
      # Converting this line from ARMSDT to GAS syntax
      $Out = $_;

      # Apply current register aliases
      foreach $K (keys (%RN))
      {
        $Out =~ s/$K/$RN{$K}/g;
      }

      # When previous ARMSDT line was "MACRO", output a GAS macro
      if($Macro)
      {
        $Out   =~ s/^(\s+)(\S.*)$/$1\.macro $2/;
        $Out   =~ s/\$//g;
        $Macro = 0;
      }

      # Replace ARMSDT constructs with GAS constructs
      $Out =~ s/^(\s+)MEND/$1.endm/g;
      $Out =~ s/^(\s+)IMPORT/$1.extern/g;
      $Out =~ s/^(\s+)EXPORT/$1.global/g;
      $Out =~ s/^(\s+)END/$1/g;
      $Out =~ s/DCD/.word/g;
      $Out =~ s/DCB/.byte/g;
      $Out =~ s/\$/\\/g;
      $Out =~ s/;/@/g;
      $Out =~ s/^(\S+)\s*$/$1:/;
      $Out =~ s/^(\S+)(\s+)/$1:$2/;
      $Out =~ s/\[\s*:DEF:\s*(\S+)/.ifdef $1/g;
      $Out =~ s/\[(\s+)/.if$1/g;
      $Out =~ s/^(\s*)\|/$1.else/g;
      $Out =~ s/^(\s*)\]/$1.endif/g;
      $Out =~ s/\:AND\:/ \& /g;
      $Out =~ s/\:XOR\:/ \^ /g;
      $Out =~ s/\:SHL\:/ \<\< /g;
      $Out =~ s/\:SHR\:/ \>\> /g;
      $Out =~ s/\:OR\:/ \| /g;

      # Print out line in GAS syntax
      print $Out;
    }
  }

  close(F);
}


# No register aliases yet
@RN = ();

# Change directory to where input file is
if($ARGV[0]=~/^(.*)[\/\\](.*?)$/)
{
  $FileName = $2;
  chdir($1);
}
else
{
  $FileName = $ARGV[0];
}

# Process input file and its includes recursively
&ProcFile($FileName);

