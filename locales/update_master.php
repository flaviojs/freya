#!/bin/php
<?php
/* update_master.php : This code will update master .po files
 */

$master_language = 'en_US'; // native language of NPCs
$npc_dir = '../npc'; // where NPCs are located

$out_file = $master_language.'/LC_MESSAGES/npcs.po';
$main_index = array();

recursive_scan($main_index, $npc_dir);

$fp = fopen($out_file, 'w');
$now=date('Y-m-d H:i:sO');
fputs($fp, <<<EOF
# NEZUMI - MASTER LOCALES
# Copyright (C) 2006 MagicalTux
# This file is distributed under the same license as the Freya package.
# MagicalTux <MagicalTux@ooKoo.org>, 2006
#
msgid ""
msgstr ""
"Project-Id-Version: Nezumi-Locales 1.0\\n"
"Report-Msgid-Bugs-To: \\n"
"POT-Creation-Date: $now\\n"
"PO-Revision-Date: $now\\n"
"Last-Translator: MagicalTux <MagicalTux@ooKoo.org>\\n"
"Language-Team: Freya French <MagicalTux@ooKoo.org>\\n"
"MIME-Version: 1.0\\n"
"Content-Type: text/plain; charset=iso-8859-15\\n"
"Content-Transfer-Encoding: 8bit\\n"

EOF
);

foreach($main_index as $str=>$ref) {
	$lb = 1;
	$encoding = mb_detect_encoding($str, 'ASCII,JIS,UTF-8,EUC-JP,SJIS');
	if ($encoding != 'ASCII') {
		echo 'WARNING !! Non ascii message at: ';
		foreach($ref as $inf) echo $inf[0].':'.$inf[1].' ';
		echo "\n";
		continue;
	}
	foreach($ref as $inf) {
		fputs($fp, ($lb?"\n".'#: ':' ').$inf[0].':'.$inf[1]);
		$lb = 1 - $lb;
	}
	fputs($fp, "\n");
	if (strlen($str)>71) {
		// cut it out...
		$str = '"'.wordwrap($str, 78, "\"\n\"").'"';
		fputs($fp, 'msgid ""'."\n".$str."\n");
		fputs($fp, 'msgstr ""'."\n".$str."\n");
	} else {
		fputs($fp, 'msgid "'.$str.'"'."\n");
		fputs($fp, 'msgstr "'.$str.'"'."\n");
	}
}

function recursive_scan(&$index, $dir) {
	// Scan $dir for NPC files, and index all strings...
	$dh = opendir($dir);
	if (!$dh) {
		echo "Error opening dir $dir !\n";
		return;
	}
	echo "Scanning files in $dir...\n";
	while($fil = readdir($dh)) {
		if ($fil{0}=='.') continue;
		$fl = $dir.'/'.$fil;
		if (is_dir($fl)) {
			recursive_scan($index, $fl);
		} else {
			file_scan($index, $fl);
		}
	}
	closedir($dh);
}

function file_scan(&$index, $fl) {
	$c = file($fl);
	if (!$c) return;
	foreach($c as $num=>$lin) {
		// just grab text in double-quotes, do not care for the rest
		if (preg_match_all('/"([^"]+)"/', $lin, $out, PREG_PATTERN_ORDER) == 0) continue;
		foreach($out[1] as $str) {
			$index[$str][] = array($fl, $num+1);
		}
	}
}

