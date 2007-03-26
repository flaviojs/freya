#!/usr/bin/perl

my($jmob_db_txt) = "./japanese/mob_db.txt"; # Set here database to be translated
my($emob_db_txt) = "./english/mob_db.txt"; # Set here an English database that the English text will be imported from

# option: skills conversion (for the file mob_skill_db.txt)
my($jskill_db_txt) = "./japanese/skill_db.txt"; # Set here database to be translated
my($eskill_db_txt) = "./english/skill_db.txt"; # Set here an English database that the English text will be imported from

# define different types of prefix/postfix
$patternPrefix[0] = ",";
$patternPrefix2[0] = "";
$patternPostfix[0] = ",";
$patternPrefix[1] = "\t";
$patternPrefix2[1] = "";
$patternPostfix[1] = "\t";
$patternPrefix[2] = ",\"";
$patternPrefix2[2] = "";
$patternPostfix[2] = "\",";
$patternPrefix[3] = "@";
$patternPrefix2[3] = "Åó";
$patternPostfix[3] = ",";
$patternPrefix[4] = "//";
$patternPrefix2[4] = "// ";
$patternPostfix[4] = "";

# List of files to translate
$n = 0;
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/mob_db.txt";
$db_out_file[$n++] = "./output/mob_db.txt";
$db_pattern[$n] = 4;
$db_file[$n] = "./japanese/mob_avail.txt";
$db_out_file[$n++] = "./output/mob_avail.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/mob_boss.txt";
$db_out_file[$n++] = "./output/random/mob_bloodybranch.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/mob_branch.txt";
$db_out_file[$n++] = "./output/random/mob_deadbranch.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/mob_group_db.txt";
$db_out_file[$n++] = "./output/mob_group_db.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/mob_poring.txt";
$db_out_file[$n++] = "./output/random/mob_poringbox.txt";
$db_pattern[$n] = 3;
$db_file[$n] = "./japanese/mob_skill_db.txt";
$db_out_file[$n++] = "./output/mob_skill_db.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/pet_db.txt";
$db_out_file[$n++] = "./output/pet_db.txt";

print "This tool converts all japanese monsters databases in english based on '$jmob_db_txt' and '$emob_db_txt'.\n";
print "(It uses '$jskill_db_txt' and '$eskill_db_txt' to convert skill names).\n";
print "\n";

# CHECK japanese file mob_db.txt
if (! -e $jmob_db_txt) {
	print "The file '$jmob_db_txt' doesn't exist.\n";
	print "End of program.\n";
	exit(1);
} elsif (! -r $jmob_db_txt) {
	print "The file '$jmob_db_txt' can not be readed.\n";
	print "End of program.\n";
	exit(2);
}
# CHECK english file mob_db.txt
if (! -e $emob_db_txt) {
	print "The file '$emob_db_txt' doesn't exist.\n";
	print "End of program.\n";
	exit(3);
} elsif (! -r $emob_db_txt) {
	print "The file '$emob_db_txt' can not be readed.\n";
	print "End of program.\n";
	exit(4);
}

# Open and read japanese and english mob_db.txt
if (open(JMOBDB_HANDLE, $jmob_db_txt) == False) {
	print "The file '$jmob_db_txt' can not be openened.\n";
	print "End of program.\n";
	exit(5);
} else {
	print "Reading file '$jmob_db_txt'.\n";
	@jmobdb = <JMOBDB_HANDLE>;
	if (open(EMOBDB_HANDLE, $emob_db_txt) == False) {
		close JMOBDB_HANDLE;
		print "The file '$emob_db_txt' can not be openened.\n";
		print "End of program.\n";
		exit(6);
	} else {
		print "Reading file '$emob_db_txt'.\n";
		@emobdb = <EMOBDB_HANDLE>;
		close EMOBDB_HANDLE;
	}
	close JMOBDB_HANDLE;
}
# extract japanese names
foreach(@jmobdb) {
	if ($_ =~ /^\/*?(\d+),(.+?),(.+?),.+$/) {
		$k = $1;
		$v = $3;
		$v =~ s/(.)/"\\x".sprintf("%2X", unpack("C", $1))/ge; # convert in hex for replace function
		$jname{"$k"} = $v;
	}
}
# extract english names
foreach(@emobdb) {
	if ($_ =~ /^\/*?(\d+),(.+?),(.+?),.+$/) {
		$ename{"$1"} = $3;
	}
}
print "\n";

# Open and read japanese and english skill_db.txt
@jskilldb = "";
if (open(JskillDB_HANDLE, $jskill_db_txt) == False) {
	print "The file '$jskill_db_txt' can not be openened.\n";
	print "The skill_db will not be correctly translated.\n";
} else {
	print "Reading file '$jskill_db_txt'.\n";
	@jskilldb = <JskillDB_HANDLE>;
	@eskilldb = "";
	if (open(EskillDB_HANDLE, $eskill_db_txt) == False) {
		@jskilldb = "";
		close JskillDB_HANDLE;
		print "The file '$eskill_db_txt' can not be openened.\n";
		print "The skill_db will not be correctly translated.\n";
		exit(6);
	} else {
		print "Reading file '$eskill_db_txt'.\n";
		@eskilldb = <EskillDB_HANDLE>;
		close EskillDB_HANDLE;
	}
	close JskillDB_HANDLE;
}
# extract japanese names
foreach(@jskilldb) {
	if ($_ =~ /^\/*?(\d+),(.+?)\#(.+?)\#.+$/) {
		$k = $1;
		$v = $3;
		$v =~ s/(.)/"\\x".sprintf("%2X", unpack("C", $1))/ge; # convert in hex for replace function
		$jskillname{"$k"} = $v;
	} elsif ($_ =~ /^\/*?(\d+),(.+?)\#(.+?)$/) {
		$k = $1;
		$v = $3;
		$v =~ s/(.)/"\\x".sprintf("%2X", unpack("C", $1))/ge; # convert in hex for replace function
		$jskillname{"$k"} = $v;
	}
}
# extract english names
foreach(@eskilldb) {
	if ($_ =~ /^\/*?(\d+),(.+?)\#(.+?)\#.+$/) {
		$eskillname{"$1"} = $3;
	} elsif ($_ =~ /^\/*?(\d+),(.+?)\#(.+?)$/) {
		$eskillname{"$1"} = $3;
	}
}
print "\n";

# Read each file
for($i = 0; $i < $n; $i++) {
	if (open(FILE_HANDLE, $db_file[$i]) == False) {
		print "The file '".$db_file[$i]."' can not be readed.\n";
	} else {
		print "Reading file '".$db_file[$i]."'...";
		$db{$db_file[$i]} = "";
		foreach(<FILE_HANDLE>) {
			$db{$db_file[$i]} .= $_;
		}
		close FILE_HANDLE;
		print " Done.\n";
	}
}
print "\n";

# Change each jskillname to eskillname
print "Renaming all monster's skills...";
foreach $key(keys %jskillname) {
	if ($eskillname{$key}) {
		if ($eskillname{$key} !~ /^$jskillname{$key}/i) { # don't replace similar text (case sensitive)
			# searching longer jskillname (example: 'santa poring' is after 'poring', and translation is not good)
			foreach $key2(keys %jskillname) {
				if (index($jskillname{$key2}, $jskillname{$key}) >= 0 && length($jskillname{$key}) < length($jskillname{$key2})) {
					if ($eskillname{$key2}) {
						if ($eskillname{$key2} !~ /^$jskillname{$key2}/i) { # don't replace similar text (case sensitive)
							for($i = 0; $i < $n; $i++) {
								$pattern = $jskillname{$key2}."Åó"; # @ in japanese code
								$replace = $eskillname{$key2}."@";
								$db{$db_file[$i]} =~ s/$pattern/$replace/g;
							}
						}
					}
				}
			}
			for($i = 0; $i < $n; $i++) {
				$pattern = $jskillname{$key}."Åó"; # @ in japanese code
				$replace = $eskillname{$key}."@";
				$db{$db_file[$i]} =~ s/$pattern/$replace/g;
			}
		}
	}
}
print "Done.\n";
print "\n";

# Change each jname to ename
print "Renaming all monster names...\n";
foreach $key(keys %jname) {
	if ($ename{$key}) {
		if ($ename{$key} !~ /^$jname{$key}/i) { # don't replace similar text (case sensitive)
			# searching longer jname (example: 'santa poring' is after 'poring', and translation will be not good)
			foreach $key2(keys %jname) {
				if (index($jname{$key2}, $jname{$key}) >= 0 && length($jname{$key}) < length($jname{$key2})) {
					if ($ename{$key2}) {
						if ($ename{$key2} !~ /^$jname{$key2}/i) { # don't replace similar text (case sensitive)
							for($i = 0; $i < $n; $i++) {
								$pattern = $patternPrefix[$db_pattern[$i]].$jname{$key2}.$patternPostfix[$db_pattern[$i]];
								$replace = $patternPrefix[$db_pattern[$i]].$ename{$key2}.$patternPostfix[$db_pattern[$i]];
								$db{$db_file[$i]} =~ s/$pattern/$replace/g;
								if (length($patternPrefix2[$db_pattern[$i]]) > 0) {
									$pattern = $patternPrefix2[$db_pattern[$i]].$jname{$key2}.$patternPostfix[$db_pattern[$i]];
									$replace = $patternPrefix2[$db_pattern[$i]].$ename{$key2}.$patternPostfix[$db_pattern[$i]];
									$db{$db_file[$i]} =~ s/$pattern/$replace/g;
								}
							}
						}
					}
				}
			}
			for($i = 0; $i < $n; $i++) {
				$pattern = $patternPrefix[$db_pattern[$i]].$jname{$key}.$patternPostfix[$db_pattern[$i]];
				$replace = $patternPrefix[$db_pattern[$i]].$ename{$key}.$patternPostfix[$db_pattern[$i]];
				$db{$db_file[$i]} =~ s/$pattern/$replace/g;
				if (length($patternPrefix2[$db_pattern[$i]]) > 0) {
					$pattern = $patternPrefix2[$db_pattern[$i]].$jname{$key}.$patternPostfix[$db_pattern[$i]];
					$replace = $patternPrefix2[$db_pattern[$i]].$ename{$key}.$patternPostfix[$db_pattern[$i]];
					$db{$db_file[$i]} =~ s/$pattern/$replace/g;
				}
			}
		}
	} else {
		print "No translation for this monster: $key -- ".$jname{$key}."!\n";
	}
}
print "Monsters' renaming done.\n";
print "\n";

# save files
for($i = 0; $i < $n; $i++) {
	if (open(FILE_HANDLE, ">".$db_out_file[$i]) == False) {
		print "The file '".$db_out_file[$i]."' can not be created.\n";
	} else {
		print "Saving the file '".$db_out_file[$i]."'...";
		print FILE_HANDLE $db{$db_file[$i]};
		close FILE_HANDLE;
		print " Done.\n";
	}
}
print "\n";

print "Translation completed.\n";

exit(0);
