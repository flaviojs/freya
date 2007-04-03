#!/usr/bin/perl

my($jitem_db_txt) = "./japanese/item_db.txt"; # Set here database to be translated
my($eitem_db_txt) = "./english/item_db.txt"; # Set here an English database that the English text will be imported from

# define different types of prefix/postfix
$patternPrefix[0] = ",";
$patternPrefix2[0] = "";
$patternPostfix[0] = ",";
$patternPrefix[1] = ",";
$patternPrefix2[1] = "";
$patternPostfix[1] = "";
$patternPrefix[2] = "//";
$patternPrefix2[2] = "// ";
$patternPostfix[2] = "";

# List of files to translate
$n = 0;
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_db.txt";
$db_out_file[$n++] = "./output/item_db.txt";
$db_pattern[$n] = 2;
$db_file[$n] = "./japanese/item_db2.txt";
$db_out_file[$n++] = "./output/item_db2.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_arrowquiver.txt";
$db_out_file[$n++] = "./output/random/item_arrowquiver.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_arrowtype.txt";
$db_out_file[$n++] = "./output/item_arrowtype.txt";
$db_pattern[$n] = 1;
$db_file[$n] = "./japanese/item_avail.txt";
$db_out_file[$n++] = "./output/item_avail.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_bluebox.txt";
$db_out_file[$n++] = "./output/random/item_bluebox.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_cardalbum.txt";
$db_out_file[$n++] = "./output/random/item_cardalbum.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_diamond_armor.txt";
$db_out_file[$n++] = "./output/random/item_diamond_armor.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_diamond_helm.txt";
$db_out_file[$n++] = "./output/random/item_diamond_helm.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_diamond_hood.txt";
$db_out_file[$n++] = "./output/random/item_diamond_hood.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_diamond_shield.txt";
$db_out_file[$n++] = "./output/random/item_diamond_shield.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_diamond_shoes.txt";
$db_out_file[$n++] = "./output/random/item_diamond_shoes.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_diamond_weapon.txt";
$db_out_file[$n++] = "./output/random/item_diamond_weapon.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_fabox.txt";
$db_out_file[$n++] = "./output/random/item_fabox.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_findingore.txt";
$db_out_file[$n++] = "./output/random/item_findingore.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_food.txt";
$db_out_file[$n++] = "./output/random/item_food.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_giftbox.txt";
$db_out_file[$n++] = "./output/random/item_giftbox.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_group_db.txt";
$db_out_file[$n++] = "./output/item_group_db.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_jewel_box.txt";
$db_out_file[$n++] = "./output/random/item_jewel_box.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_mask.txt";
$db_out_file[$n++] = "./output/random/item_mask.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_meiji_almond.txt";
$db_out_file[$n++] = "./output/random/item_meiji_almond.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_petbox.txt";
$db_out_file[$n++] = "./output/random/item_petbox.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_rjc2006.txt";
$db_out_file[$n++] = "./output/random/item_rjc2006.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_scroll.txt";
$db_out_file[$n++] = "./output/random/item_scroll.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_value_db.txt";
$db_out_file[$n++] = "./output/item_value_db.txt";
$db_pattern[$n] = 0;
$db_file[$n] = "./japanese/item_violetbox.txt";
$db_out_file[$n++] = "./output/random/item_violetbox.txt";

print "This tool converts all japanese items databases in english based on '$jitem_db_txt' and '$eitem_db_txt'.\n";
print "\n";

# CHECK japanese file item_db.txt
if (! -e $jitem_db_txt) {
	print "The file '$jitem_db_txt' doesn't exist.\n";
	print "End of program.\n";
	exit(1);
} elsif (! -r $jitem_db_txt) {
	print "The file '$jitem_db_txt' can not be readed.\n";
	print "End of program.\n";
	exit(2);
}
# CHECK english file item_db.txt
if (! -e $eitem_db_txt) {
	print "The file '$eitem_db_txt' doesn't exist.\n";
	print "End of program.\n";
	exit(3);
} elsif (! -r $eitem_db_txt) {
	print "The file '$eitem_db_txt' can not be readed.\n";
	print "End of program.\n";
	exit(4);
}

# Open and read japanese and english item_db.txt
if (open(JitemDB_HANDLE, $jitem_db_txt) == False) {
	print "The file '$jitem_db_txt' can not be openened.\n";
	print "End of program.\n";
	exit(5);
} else {
	print "Reading file '$jitem_db_txt'.\n";
	@jitemdb = <JitemDB_HANDLE>;
	if (open(EitemDB_HANDLE, $eitem_db_txt) == False) {
		close JitemDB_HANDLE;
		print "The file '$eitem_db_txt' can not be openened.\n";
		print "End of program.\n";
		exit(6);
	} else {
		print "Reading file '$eitem_db_txt'.\n";
		@eitemdb = <EitemDB_HANDLE>;
		close EitemDB_HANDLE;
	}
	close JitemDB_HANDLE;
}
# extract japanese names
foreach(@jitemdb) {
	if ($_ =~ /^\/*?(\d+),(.+?),(.+?),.+$/) {
		$k = $1;
		$v = $3;
		$v =~ s/(.)/"\\x".sprintf("%2X", unpack("C", $1))/ge; # convert in hex for replace function
		$jname{"$k"} = $v;
	}
}
# extract english names
foreach(@eitemdb) {
	if ($_ =~ /^\/*?(\d+),(.+?),(.+?),.+$/) {
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
print "Renaming all items names...\n";
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
		print "No translation for this item: $key -- ".$jname{$key}."!\n";
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
