#!/usr/bin/perl

my($jhomun_db_txt) = "./japanese/homun_db.txt"; # Set here database to be translated
my($ehomun_db_txt) = "./english/homun_db.txt"; # Set here an English database that the English text will be imported from

# define different types of prefix/postfix
$patternPrefix[0] = ",";
$patternPrefix2[0] = "";
$patternPostfix[0] = ",";

# List of files to translate
$n = 0;
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/homun_db.txt";
$db_out_file[$n++] = "./output/homun_db.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/embryo_db.txt";
$db_out_file[$n++] = "./output/embryo_db.txt";

print "This tool converts all japanese homunculus databases in english based on '$jhomun_db_txt' and '$ehomun_db_txt'.\n";
print "\n";

# CHECK japanese file homun_db.txt
if (! -e $jhomun_db_txt) {
	print "The file '$jhomun_db_txt' doesn't exist.\n";
	print "End of program.\n";
	exit(1);
} elsif (! -r $jhomun_db_txt) {
	print "The file '$jhomun_db_txt' can not be readed.\n";
	print "End of program.\n";
	exit(2);
}
# CHECK english file homun_db.txt
if (! -e $ehomun_db_txt) {
	print "The file '$ehomun_db_txt' doesn't exist.\n";
	print "End of program.\n";
	exit(3);
} elsif (! -r $ehomun_db_txt) {
	print "The file '$ehomun_db_txt' can not be readed.\n";
	print "End of program.\n";
	exit(4);
}

# Open and read japanese and english homun_db.txt
if (open(JhomunDB_HANDLE, $jhomun_db_txt) == False) {
	print "The file '$jhomun_db_txt' can not be openened.\n";
	print "End of program.\n";
	exit(5);
} else {
	print "Reading file '$jhomun_db_txt'.\n";
	@jhomunDB = <JhomunDB_HANDLE>;
	if (open(EhomunDB_HANDLE, $ehomun_db_txt) == False) {
		close JhomunDB_HANDLE;
		print "The file '$ehomun_db_txt' can not be openened.\n";
		print "End of program.\n";
		exit(6);
	} else {
		print "Reading file '$ehomun_db_txt'.\n";
		@ehomunDB = <EhomunDB_HANDLE>;
		close EhomunDB_HANDLE;
	}
	close JhomunDB_HANDLE;
}
# extract japanese names
foreach(@jhomunDB) {
	if ($_ =~ /^\/*?(\d+),(.+?),(.+?),(.+?),.+$/) {
		$k = $1;
		$v = $4;
		$v =~ s/(.)/"\\x".sprintf("%2X", unpack("C", $1))/ge; # convert in hex for replace function
		$jname{"$k"} = $v;
	}
}
# extract english names
foreach(@ehomunDB) {
	if ($_ =~ /^\/*?(\d+),(.+?),(.+?),(.+?),.+$/) {
		$ename{"$1"} = $4;
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
print "Renaming all homunculus names...\n";
foreach $key(keys %jname) {
	if ($ename{$key}) {
		if ($ename{$key} !~ /^$jname{$key}/i) { # don't replace similar text (case sensitive)
			# searching longer jname (example: 'Apple Juice' is after 'Apple', and translation will be not good)
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
		print "No translation for this homunculus: $key -- ".$jname{$key}."!\n";
	}
}
print "Items' renaming done.\n";
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
