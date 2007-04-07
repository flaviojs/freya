<?php
$res = array();

$fil = fopen('../db/item_db.txt', 'r');
while(!feof($fil)) {
	$lin = rtrim(fgets($fil, 1024*1024));
	if (substr($lin, 0, 2)=='//') continue;
	if ($lin=='') continue;
	$lin = explode(',', $lin);
	if (!$lin[16]) continue;
	if (!is_numeric($lin[16])) continue;
	$res[(int)$lin[16]][(int)$lin[0]] = true;
}
ksort($res);

$res = var_export($res, true);

$out = fopen('dispdb.php', 'w');
fwrite($out, '<?php'."\n".'$dispdb = '.$res.';'."\n\n");
fclose($out);

