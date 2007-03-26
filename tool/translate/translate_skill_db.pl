#!/usr/bin/perl

my($jskill_db_txt) = "./japanese/skill_db.txt"; # Set here database to be translated
my($eskill_db_txt) = "./english/skill_db.txt"; # Set here an English database that the English text will be imported from

# define different types of prefix/postfix
$patternPrefix[0] = "#";
$patternPrefix2[0] = "";
$patternPostfix[0] = "";
$patternPrefix[1] = ",";
$patternPrefix2[1] = "";
$patternPostfix[1] = ",";
$patternPrefix[2] = "";
$patternPrefix2[2] = "// ";
$patternPostfix[2] = "";

# List of files to translate
$n = 0;
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/skill_db.txt";
$db_out_file[$n++] = "./output/skill_db.txt";
$db_pattern[$n] = 1;
$db_file[$n] = "./japanese/abra_db.txt";
$db_out_file[$n++] = "./output/abra_db.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/guild_skill_tree.txt";
$db_out_file[$n++] = "./output/guild_skill_tree.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/homun_skill_tree.txt";
$db_out_file[$n++] = "./output/homun_skill_tree.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/skill_cast_db.txt";
$db_out_file[$n++] = "./output/skill_cast_db.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/skill_db2.txt";
$db_out_file[$n++] = "./output/skill_db2.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/skill_require_db2.txt";
$db_out_file[$n++] = "./output/skill_require_db2.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/skill_require_db.txt";
$db_out_file[$n++] = "./output/skill_require_db.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/skill_tree.txt";
$db_out_file[$n++] = "./output/skill_tree.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/skill_unit_db.txt";
$db_out_file[$n++] = "./output/skill_unit_db.txt";
$db_pattern[$n] = 2;
$db_file[$n] = "./japanese/scdata_db.txt";
$db_out_file[$n++] = "./output/scdata_db.txt";

print "This tool converts all japanese skills databases in english based on '$jskill_db_txt' and '$eskill_db_txt'.\n";
print "\n";

# CHECK japanese file skill_db.txt
if (! -e $jskill_db_txt) {
	print "The file '$jskill_db_txt' doesn't exist.\n";
	print "End of program.\n";
	exit(1);
} elsif (! -r $jskill_db_txt) {
	print "The file '$jskill_db_txt' can not be readed.\n";
	print "End of program.\n";
	exit(2);
}
# CHECK english file skill_db.txt
if (! -e $eskill_db_txt) {
	print "The file '$eskill_db_txt' doesn't exist.\n";
	print "End of program.\n";
	exit(3);
} elsif (! -r $eskill_db_txt) {
	print "The file '$eskill_db_txt' can not be readed.\n";
	print "End of program.\n";
	exit(4);
}

# Open and read japanese and english skill_db.txt
if (open(JitemDB_HANDLE, $jskill_db_txt) == False) {
	print "The file '$jskill_db_txt' can not be openened.\n";
	print "End of program.\n";
	exit(5);
} else {
	print "Reading file '$jskill_db_txt'.\n";
	@jitemdb = <JitemDB_HANDLE>;
	if (open(EitemDB_HANDLE, $eskill_db_txt) == False) {
		close JitemDB_HANDLE;
		print "The file '$eskill_db_txt' can not be openened.\n";
		print "End of program.\n";
		exit(6);
	} else {
		print "Reading file '$eskill_db_txt'.\n";
		@eitemdb = <EitemDB_HANDLE>;
		close EitemDB_HANDLE;
	}
	close JitemDB_HANDLE;
}
# extract japanese names
foreach(@jitemdb) {
	if ($_ =~ /^\/*?(\d+),(.+?)\#(.+?)\#.+$/) {
		$k = $1;
		$v = $3;
		$v =~ s/(.)/"\\x".sprintf("%2X", unpack("C", $1))/ge; # convert in hex for replace function
		$jname{"$k"} = $v;
	} elsif ($_ =~ /^\/*?(\d+),(.+?)\#(.+?)$/) {
		$k = $1;
		$v = $3;
		$v =~ s/(.)/"\\x".sprintf("%2X", unpack("C", $1))/ge; # convert in hex for replace function
		$jname{"$k"} = $v;
	}
}
# extract english names
foreach(@eitemdb) {
	if ($_ =~ /^\/*?(\d+),(.+?)\#(.+?)\#.+$/) {
		$ename{"$1"} = $3;
	} elsif ($_ =~ /^\/*?(\d+),(.+?)\#(.+?)$/) {
		$ename{"$1"} = $3;
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

# Change each jname to ename
print "Renaming all skills names...\n";
foreach $key(keys %jname) {
	if ($ename{$key}) {
		if ($ename{$key} !~ /^$jname{$key}/i) { # don't replace similar text (case sensitive)
			# searching longer jname (example: 'Two-Handed Sword Mastery' is after 'Sword Mastery', and translation will be not good)
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
		print "No translation for this skill: $key -- ".$jname{$key}."!\n";
	}
}
print "skills' renaming done.\n";
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
