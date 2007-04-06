<?php
// char converter, php-based
// Will generate MySQL-compatible queries from TXT savefiles
// $Id$
// DO NOT TRY TO RUN THAT FROM A WEBHOST
// This is intented to be started from command line
// php char_converter.php | mysql -u ragnarok -pragnarok ragnarok
// If you get a "Fatal error: Allowed memory size of ..." error, try like this :
// php -d memory_limit=512M char_converter.php | mysql -u ragnarok -pragnarok ragnarok
//
// Known problems:
// - TODO: Code party convert
// - TODO: Strip '#' from various things (guild messages, positions, etc)

$stderr = fopen('php://stderr', 'w');

if (!function_exists('mysql_escape_string')) {
	function mysql_escape_string($str) {
		return addslashes($str);
	}
}

if ((is_dir('../conf')) && (!is_dir('conf'))) chdir('..');

$char_config = parse_config('conf/char_athena.conf');
$inter_config = parse_config('conf/inter_athena.conf');

$chars_data = array();

$tables = array(
	'cart_inventory',
	'char',
	'global_reg_value',
	'account_reg_db',
	'guild',
	'guild_alliance',
	'guild_castle',
	'guild_expulsion',
	'guild_member',
	'guild_position',
	'guild_skill',
	'guild_storage',
	'inventory',
	'memo',
	'party',
	'pet',
	'skill',
	'storage',
	'friends'
);

foreach($tables as $t) echo 'TRUNCATE TABLE `'.$t.'`;'."\n";

// read guild_storage_txt
fputs($stderr, "Reading/inserting guild_storage_txt file...\n");
$fil = fopen($inter_config['guild_storage_txt'], 'r');
if (!$fil) {
	fputs($stderr, "Error opening guild_storage_txt file!\n");
	exit(4);
}
while(!feof($fil)) {
	$lin = rtrim(fgets($fil, 1024*1024));
	if ($lin=='') continue;
	if (substr($lin, 0, 2)=='//') continue;
	list($guild_id, $items) = explode("\t", $lin);
	list($guild_id) = explode(',', $guild_id);
	$items = explode(' ', $items);
	foreach($items as $item) {
		$item = explode(',', $item);
		$insert = array(
//			'id'=>$item[0],
			'guild_id'=>$guild_id,
			'nameid'=>$item[1],
			'amount'=>$item[2],
			'equip'=>$item[3],
			'identify'=>$item[4],
			'refine'=>$item[5],
			'attribute'=>$item[6],
			'card0'=>$item[7],
			'card1'=>$item[8],
			'card2'=>$item[9],
			'card3'=>$item[10],
		);
		$req = '';
		foreach($insert as $var=>$val) $req.=($req==''?'':', ').'`'.$var.'` = \''.mysql_escape_string($val).'\'';
		$req = 'INSERT INTO `guild_storage` SET '.$req.";\n";
		echo $req;
	}
}

// read castle_txt
fputs($stderr, "Reading/inserting castle_txt file...\n");
$fil = fopen($inter_config['castle_txt'], 'r');
if (!$fil) {
	fputs($stderr, "Error opening castle_txt file!\n");
	exit(4);
}
while(!feof($fil)) {
	$lin = rtrim(fgets($fil, 1024*1024));
	if ($lin=='') continue;
	if (substr($lin, 0, 2)=='//') continue;
	$lin = explode(',', $lin);
	$insert = array(
		'castle_id'=>$lin[0],
		'guild_id'=>$lin[1],
		'economy'=>$lin[2],
		'defense'=>$lin[3],
		'triggerE'=>$lin[4],
		'triggerD'=>$lin[5],
		'nextTime'=>$lin[6],
		'payTime'=>$lin[7],
		'createTime'=>$lin[8],
		'visibleC'=>$lin[9],
		'visibleG0'=>$lin[10],
		'visibleG1'=>$lin[11],
		'visibleG2'=>$lin[12],
		'visibleG3'=>$lin[13],
		'visibleG4'=>$lin[14],
		'visibleG5'=>$lin[15],
		'visibleG6'=>$lin[16],
		'visibleG7'=>$lin[17],
		'gHP0'=>$lin[18],
		'ghP1'=>$lin[19],
		'gHP2'=>$lin[20],
		'gHP3'=>$lin[21],
		'gHP4'=>$lin[22],
		'gHP5'=>$lin[23],
		'gHP6'=>$lin[24],
		'gHP7'=>$lin[25],
	);
	$req = '';
	foreach($insert as $var=>$val) $req.=($req==''?'':', ').'`'.$var.'` = \''.mysql_escape_string($val).'\'';
	$req = 'REPLACE INTO `guild_castle` SET '.$req.";\n";
	echo $req;
}

// read guild_txt
fputs($stderr, "Reading/inserting guild_txt file...\n");
$fil = fopen($inter_config['guild_txt'], 'r');
if (!$fil) {
	fputs($stderr, "Error opening guild_txt file!\n");
	exit(4);
}
while(!feof($fil)) {
	$lin = rtrim(fgets($fil, 1024*1024));
	if ($lin=='') continue;
	if (substr($lin, 0, 2)=='//') continue;
	$guild = sscanf($lin, "%d\t%[^\t]\t%[^\t]\t%d,%d,%d,%d,%d\t%[^\t]\t%[^\t]\t%n");
	$lin = substr($lin, $guild[10]);
	$guild_id = $guild[0];
	while($char = sscanf($lin, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\t%[^\t]\t%n")) {
		if (is_null($char[11])) break;
		$lin=substr($lin, $char[11]);
		$insert = array(
			'guild_id'=>$guild_id,
			'account_id'=>$char[0],
			'char_id'=>$char[1],
			'hair'=>$char[2],
			'hair_color'=>$char[3],
			'gender'=>$char[4],
			'class'=>$char[5],
			'lv'=>$char[6],
			'exp'=>$char[7],
			'exp_payper'=>$char[8],
			'online'=>0,
			'position'=>$char[9],
			'rsv1'=>0,
			'rsv2'=>0,
			'name'=>$char[10],
		);
		$req = '';
		foreach($insert as $var=>$val) $req.=($req==''?'':', ').'`'.$var.'` = \''.mysql_escape_string($val).'\'';
		$req = 'INSERT INTO `guild_member` SET '.$req.";\n";
		echo $req;
	}
	// read guild positions
	$pn=0;
	while($pos = sscanf($lin, "%d,%d\t%[^\t]\t%n")) {
		if (is_null($pos[3])) break;
		if ($pos[2]{0}==',') break;
		$lin=substr($lin, $pos[3]);
		$insert = array(
			'guild_id'=>$guild_id,
			'position'=>$pn++,
			'name'=>$pos[2],
			'mode'=>$pos[0],
			'exp_mode'=>$pos[1],
		);
		$req = '';
		foreach($insert as $var=>$val) $req.=($req==''?'':', ').'`'.$var.'` = \''.mysql_escape_string($val).'\'';
		$req = 'INSERT INTO `guild_position` SET '.$req.";\n";
		echo $req;
	}
	// Read guild emblem
	$emblem = sscanf($lin, "%d,%d,%[^\t]\t%n");
	$lin=substr($lin, $emblem[3]);
	$lin = explode("\t", $lin);
	$acount = array_shift($lin);
	for($i=0;$i<$acount;$i++) {
		list($guild2_id, $opp) = explode(',', array_shift($lin));
		$al_name = array_shift($lin);
		$insert = array(
			'guild_id'=>$guild_id,
			'opposition'=>$opp,
			'alliance_id'=>$guild2_id,
			'name'=>$al_name,
		);
		$req = '';
		foreach($insert as $var=>$val) $req.=($req==''?'':', ').'`'.$var.'` = \''.mysql_escape_string($val).'\'';
		$req = 'INSERT INTO `guild_alliance` SET '.$req.";\n";
		echo $req;
	}
	// read expulsions
	$ecount = array_shift($lin);
	for($i=0;$i<$ecount;$i++) {
		$dat = array();
		$dat[]=explode(',', array_shift($lin)); // 2010123,0,0,0
		$dat[]=array_shift($lin); // char name
		$dat[]=array_shift($lin); // dummy
		$dat[]=array_shift($lin); // reason
		$insert = array(
			'guild_id'=>$guild_id,
			'name'=>$dat[1],
			'mes'=>$dat[3],
			'acc'=>$dat[2],
			'account_id'=>$dat[0][0],
			'rsv1'=>$dat[0][1],
			'rsv2'=>$dat[0][2],
			'rsv3'=>$dat[0][3],
		);
		$req = '';
		foreach($insert as $var=>$val) $req.=($req==''?'':', ').'`'.$var.'` = \''.mysql_escape_string($val).'\'';
		$req = 'INSERT INTO `guild_expulsion` SET '.$req.";\n";
		echo $req;
	}
	// read skills
	$lin = explode(' ',array_shift($lin));
	foreach($lin as $skill) {
		$skill=explode(',', $skill);
		$insert=array(
			'guild_id'=>$guild_id,
			'id'=>$skill[0],
			'lv'=>$skill[1],
		);
		$req = '';
		foreach($insert as $var=>$val) $req.=($req==''?'':', ').'`'.$var.'` = \''.mysql_escape_string($val).'\'';
		$req = 'INSERT INTO `guild_skill` SET '.$req.";\n";
		echo $req;
	}
	$insert = array(
		'guild_id'=>$guild[0],
		'name'=>$guild[1],
		'master'=>$guild[2], /* varchar */
		'guild_lv'=>$guild[3],
		'connect_member'=>0,
		'max_member'=>$guild[4],
		'average_lv'=>0,
		'exp'=>$guild[5],
		'next_exp'=>0,
		'skill_point'=>$guild[6],
		'castle_id'=>0, /* deprecated */
		'mes1'=>$guild[8],
		'mes2'=>$guild[9],
		'emblem_len'=>$emblem[0],
		'emblem_id'=>$emblem[1],
		'emblem_data'=>$emblem[2],
	);
	$req = '';
	foreach($insert as $var=>$val) $req.=($req==''?'':', ').'`'.$var.'` = \''.mysql_escape_string($val).'\'';
	$req = 'INSERT INTO `guild` SET '.$req.";\n";
	echo $req;
}
fclose($fil);

// Read pet_txt file...
fputs($stderr, "Reading/inserting pet_txt file...\n");
$fil = fopen($inter_config['pet_txt'], 'r');
if (!$fil) {
	fputs($stderr, "Error opening pet_txt file!\n");
	exit(4);
}
while(!feof($fil)) {
	$lin = rtrim(fgets($fil, 1024*1024));
	if ($lin=='') continue;
	if (substr($lin, 0, 2)=='//') continue;
	$lin = explode("\t", $lin);
	$lin[0] = explode(',', $lin[0]);
	$lin[1] = explode(',', $lin[1]);
	$insert = array(
		'pet_id'=>$lin[0][0],
		'class'=>$lin[0][1],
		'name'=>$lin[0][2],
		'account_id'=>$lin[1][0],
		'char_id'=>$lin[1][1],
		'level'=>$lin[1][2],
		'egg_id'=>$lin[1][3],
		'equip'=>$lin[1][4],
		'intimate'=>$lin[1][5],
		'hungry'=>$lin[1][6],
		'rename_flag'=>$lin[1][7],
		'incuvate'=>$lin[1][8],
	);
	$req = '';
	foreach($insert as $var=>$val) $req.=($req==''?'':', ').'`'.$var.'` = \''.mysql_escape_string($val).'\'';
	$req = 'INSERT INTO `pet` SET '.$req.";\n";
	echo $req;
}
fclose($fil);

// Read accreg_txt
fputs($stderr, "Reading/inserting accreg_txt file...\n");
$fil = fopen($inter_config['accreg_txt'], 'r');
if (!$fil) {
	fputs($stderr, "Error opening accreg_txt file!\n");
	exit(4);
}
while(!feof($fil)) {
	$lin = rtrim(fgets($fil, 1024*1024));
	if ($lin=='') continue;
	if (substr($lin, 0, 2)=='//') continue;
	list($account_id, $regs) = explode("\t", $lin);
	$regs = explode(' ', $regs);
	foreach($regs as $reg) {
		list($var,$val) = explode(',', $reg);
		$insert = array(
			'account_id'=>$account_id,
			'str'=>$var,
			'value'=>$val,
		);
		$req = '';
		foreach($insert as $var=>$val) $req.=($req==''?'':', ').'`'.$var.'` = \''.mysql_escape_string($val).'\'';
		$req = 'INSERT INTO `account_reg_db` SET '.$req.";\n";
		echo $req;
	}
}
fclose($fil);

// Read storage....
fputs($stderr, "Reading/inserting storage_txt file...\n");
$fil = fopen($inter_config['storage_txt'], 'r');
if (!$fil) {
	fputs($stderr, "Error opening storage file!\n");
	exit(4);
}
while(!feof($fil)) {
	$lin = rtrim(fgets($fil, 1024*1024));
	if ($lin=='') continue;
	if (substr($lin, 0, 2)=='//') continue;
	list($account_id, $items) = explode("\t", $lin);
	list($account_id) = explode(',', $account_id);
	$items = explode(' ',$items);
	foreach($items as $item) {
		$item = explode(',', $item);
		$insert = array(
//			'id'=>$item[0],
			'account_id'=>$account_id,
			'nameid'=>$item[1],
			'amount'=>$item[2],
			'equip'=>$item[3],
			'identify'=>$item[4],
			'refine'=>$item[5],
			'attribute'=>$item[6],
			'card0'=>$item[7],
			'card1'=>$item[8],
			'card2'=>$item[9],
			'card3'=>$item[10],
		);
		$req = '';
		foreach($insert as $var=>$val) $req.=($req==''?'':', ').'`'.$var.'` = \''.mysql_escape_string($val).'\'';
		$req = 'INSERT INTO `storage` SET '.$req.";\n";
		echo $req;
	}
}
fclose($fil);

// TODO: read party...
//fputs($stderr, "Reading/inserting parties...\n");
$fil=fopen($inter_config['party_txt'], 'r');
if (!$fil) {
	fputs($stderr, "Error opening parties file!\n");
	exit(4);
}
while(!feof($fil)) {
	$lin = rtrim(fgets($fil, 1024*1024));
	if ($lin=='') continue;
	if (substr($lin, 0, 2)=='//') continue;
}
fclose($fil);

// read chars...
fputs($stderr, "Reading char_txt file...\n");
$fil = fopen($char_config['char_txt'], 'r');
if (!$fil) {
	fputs($stderr, "Error opening accounts file!\n");
	exit(4);
}
while(!feof($fil)) {
	$lin = rtrim(fgets($fil, 65535));
	if ($lin=='') continue;
	if (substr($lin, 0, 2)=='//') continue;
	$lin=explode("\t", $lin);
	$lin[1] = explode(',', $lin[1]);
	$lin[3] = explode(',', $lin[3]);
	$lin[4] = explode(',', $lin[4]);
	$lin[5] = explode(',', $lin[5]);
	$lin[6] = explode(',', $lin[6]);
	$lin[7] = explode(',', $lin[7]);
	$lin[8] = explode(',', $lin[8]);
	$lin[9] = explode(',', $lin[9]);
	$lin[10]= explode(',', $lin[10]);
	$lin[11]= explode(',', $lin[11]);
	$lin[12]= explode(',', $lin[12]);
	$lin[13]= explode(',', $lin[13]);
	$insert = array(
		'char_id'=>$lin[0],
		'account_id'=>$lin[1][0],
		'char_num'=>$lin[1][1],
		'name'=>$lin[2],
		'class'=>$lin[3][0],
		'base_level'=>$lin[3][1],
		'job_level'=>$lin[3][2],
		'base_exp'=>$lin[4][0],
		'job_exp'=>$lin[4][1],
		'zeny'=>$lin[4][2],
		'str'=>$lin[6][0],
		'agi'=>$lin[6][1],
		'vit'=>$lin[6][2],
		'int'=>$lin[6][3],
		'dex'=>$lin[6][4],
		'luk'=>$lin[6][5],
		'max_hp'=>$lin[5][0],
		'hp'=>$lin[5][1],
		'max_sp'=>$lin[5][2],
		'sp'=>$lin[5][3],
		'status_point'=>$lin[7][0],
		'skill_point'=>$lin[7][1],
		'option'=>$lin[8][0],
		'karma'=>$lin[8][1],
		'manner'=>$lin[8][2],
		'party_id'=>0, // TODO: party support- $lin[9][0],
		'guild_id'=>$lin[9][1],
		'pet_id'=>$lin[9][2],
		'hair'=>$lin[10][0],
		'hair_color'=>$lin[10][1],
		'clothes_color'=>$lin[10][2],
		'weapon'=>$lin[11][0],
		'shield'=>$lin[11][1],
		'head_top'=>$lin[11][2],
		'head_mid'=>$lin[11][3],
		'head_bottom'=>$lin[11][4],
		'last_map'=>$lin[12][0],
		'last_x'=>$lin[12][1],
		'last_y'=>$lin[12][2],
		'save_map'=>$lin[13][0],
		'save_x'=>$lin[13][1],
		'save_y'=>$lin[13][2],
		'partner_id'=>$lin[13][3],
	);
	$char_id = (int)$lin[0];
	$chars_data[$char_id]['char'] = array($insert);
	$lin[16] = explode(' ',$lin[16]);
	foreach($lin[16] as $item) {
		if ($item=='') continue;
		$item=explode(',', $item);
		$insert = array(
//			'id'=>$item[0],
			'char_id'=>$char_id,
			'nameid'=>$item[1],
			'amount'=>$item[2],
			'equip'=>$item[3],
			'identify'=>$item[4],
			'refine'=>$item[5],
			'attribute'=>$item[6],
			'card0'=>$item[7],
			'card1'=>$item[8],
			'card2'=>$item[9],
			'card3'=>$item[10],
		);
		$chars_data[$char_id]['cart_inventory'][]=$insert;
	}
	$lin[18] = explode(' ', $lin[18]);
	foreach($lin[18] as $var) {
		if ($var=='') continue;
		list($var, $val) = explode(',', $var);
		$insert=array(
			'char_id'=>$char_id,
			'str'=>$var,
			'value'=>$val,
		);
		$chars_data[$char_id]['global_reg_value'][] = $insert;
	}
	while ($lin[14]!='') {
		$memo = sscanf($lin[14], '%[^,],%d,%d%n');
		$lin[14]=substr($lin[14],$memo[3]);
		$insert = array(
			'char_id'=>$char_id,
			'map'=>$memo[0],
			'x'=>$memo[1],
			'y'=>$memo[2],
		);
		$chars_data[$char_id]['memo'][] = $insert;
	}
	while($lin[15]!='') {
		$item = sscanf($lin[15], '%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n');
		$insert = array(
//			'id'=>$item[0],
			'char_id'=>$char_id,
			'nameid'=>$item[1],
			'amount'=>$item[2],
			'equip'=>$item[3],
			'identify'=>$item[4],
			'refine'=>$item[5],
			'attribute'=>$item[6],
			'card0'=>$item[7],
			'card1'=>$item[8],
			'card2'=>$item[9],
			'card3'=>$item[10],
		);
		$chars_data[$char_id]['inventory'][] = $insert;
		$lin[15] = substr($lin[15], $item[11]);
		if ($lin[15]{0}==' ') $lin[15] = substr($lin[15], 1);
		if ($lin[15]{0}==',') $lin[15] = substr($lin[15], 1);
	}
	$lin[17] = explode(' ', $lin[17]);
	foreach($lin[17] as $skill) {
		if ($skill=='') continue;
		$skill = explode(',', $skill);
		$insert = array(
			'char_id'=>$char_id,
			'id'=>$skill[0],
			'lv'=>$skill[1],
		);
		$chars_data[$char_id]['skill'][] = $insert;
	}
}
fclose($fil);
fputs($stderr, "Found ".count($chars_data)." characters, validating...\n");
$fix = 0;
foreach($chars_data as $id=>$foo) {
	$data=&$chars_data[$id]; // fix for PHP4 compatibility
	if($data['char'][0]['partner_id']) {
		if (!isset($chars_data[$data['char'][0]['partner_id']])) {
			$data['char'][0]['partner_id'] = 0;
			$fix++;
		}
	}
}
if ($fix>0) fputs($stderr, "Fixed $fix bad weddings\n");

fputs($stderr, "Reading friends_txt file...\n");
$fil = fopen($char_config['friends_txt'], 'r');
if (!$fil) {
	fputs($stderr, "Error opening accounts file!\n");
	exit(4);
}
while(!feof($fil)) {
	$lin = rtrim(fgets($fil, 65535));
	if ($lin=='') continue;
	if (substr($lin, 0, 2)=='//') continue;
	list($char_id, $friends) = explode("\t", $lin);
	$friends = explode(',', $friends);
	$char_id = (int)$char_id;
	if (!isset($chars_data[$char_id])) continue; // bad char -> skip
	foreach($friends as $f) {
		if (!isset($chars_data[(int)$f])) continue; // bad friend -> skip
		$insert = array(
			'char_id'=>$char_id,
			'friend_id'=>$f,
		);
		$chars_data[$char_id]['friends'][] = $insert;
	}
}
fclose($fil);


fputs($stderr, "Building inserts...\n");

foreach($chars_data as $tl) {
	foreach($tl as $t=>$j) {
		foreach($j as $i) {
			$req='';
			foreach($i as $var=>$val) $req.=($req==''?'':', ').'`'.$var.'` = \''.mysql_escape_string($val).'\'';
			$req = 'REPLACE INTO `'.$t.'` SET '.$req.";\n";
			echo $req;
		}
	}
}

function parse_config($cfg_file) {
	// parse config file...
	$res = array();
	$fp = fopen($cfg_file, 'r');
	if (!$fp) return $res;
	while(!feof($fp)) {
		$lin = rtrim(fgets($fp, 8192));
		if ($lin=='') continue; // skip empty lines
		if (substr($lin, 0, 2)=='//') continue; // skip comments
		$pos = strpos($lin, ':');
		if ($pos===false) continue; // skip invalid lines
		$lbl = trim(substr($lin, 0, $pos));
		$value = ltrim(substr($lin, $pos+1));
		$res[strtolower($lbl)]=$value;
	}
	fclose($fp);
	return $res;
}

