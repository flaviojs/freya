<?php
/** The author disclaims copyright to this source code.  In place of
 ** a legal notice, here is a blessing:
 **
 **    May you do good and not evil.
 **    May you find forgiveness for yourself and forgive others.
 **    May you share freely, never taking more than you give.
 **/

class ladmin {
	var $sock=false;

	// connect(ip,password,[port]);
	function connect($ip,$admin,$port=6900) {
		$sock=@fsockopen($ip,$port,$errno,$errstr,10);
		if (!$sock) return false;
		fwrite($sock,pack("v",0x791a));
		$buf=fread($sock,4);
		$buf=unpack("v2val",$buf);
		$len=$buf['val2']-4;
		$buf=$buf['val1'];
		if ($buf!=0x01dc) return false;
		$buf=fread($sock,$len);
		$md5=md5($admin.$buf);
		$packet=pack("v2H32",0x7918,2,$md5);
		fwrite($sock,$packet);
		$res=fread($sock,3);
		if ($res!="\x19\x79\x00") return false; // failed !
		$this->sock=$sock;
		return true;
	}

	function make_account($login,$pass,$sex,$email) {
		if (!$this->sock) return false;
		$sex=strtoupper($sex);
		if ( ($sex!='F') and ($sex!='M') ) return false;
		if ((strlen($login)<4) or (strlen($login)>24)) return false;
		if ((strlen($pass)<4) or (strlen($pass)>24)) return false;
		$packet=pack('va24a24a1a40',0x7930,$login,$pass,$sex,$email);
		fwrite($this->sock,$packet);
		$res=fread($this->sock,2);
		if ($res!="\x31\x79") {
			$this->sock=false;
			return false;
		}
		$dat=fread($this->sock,28);
		$data=substr($dat,0,4);
		if (($data=="\x00\x00\x00\x00") or ($data=="\xFF\xFF\xFF\xFF")) return false;
		$accid=unpack('Vid',$dat);
		return $accid['id'];
	}

	function close() {
		if (!$this->sock) return false;
		fwrite($this->sock,pack('v',0x7532));
		fclose($this->sock);
		return true;
	}

	function get_version() {
		if (!$this->sock) return false;
		fwrite($this->sock, pack('v',0x7535));
		$answer=fread($this->sock, 2);
		if ($answer!="\x36\x75") {
			$this->sock = false;
			return false;
		}
		$data = fread($this->sock, 11);
		$data = unpack('cFREYA_MAJOR_VERSION/cFREYA_MINOR_VERSION/cFREYA_REVISION/cFREYA_RELEASE_FLAG/cFREYA_OFFICIAL_FLAG/cFREYA_SERVER_LOGIN/vFREYA_MOD_VERSION/vFREYA_SVN_VERSION/cFREYA_DB_SYSTEM',$data);
		$data['string']='Freya version '.($data['FREYA_RELEASE_FLAG']?'dev':'stable').'-'.$data['FREYA_MAJOR_VERSION'].'.'.$data['FREYA_MINOR_VERSION'];
		if ($data['FREYA_REVISION']) $data['string'].='r'.$data['FREYA_REVISION'];
		if ($data['FREYA_OFFICIAL_FLAG']) $data['string'].='-mod'.$data['FREYA_MOD_VERSION'];
		if ($data['FREYA_SVN_VERSION']) $data['string'].=' SVN rev. '.$data['FREYA_SVN_VERSION'];
		if ($data['FREYA_DB_SYSTEM'] == 0) $data['string'].=' (TXT version)';
		if ($data['FREYA_DB_SYSTEM'] == 1) $data['string'].=' (SQL version)';
		return $data;
	}

	function get_servers() {
		if (!$this->sock) return false;
		fwrite($this->sock,pack('v',0x7938)); // server list query
		$res=fread($this->sock,2);
		if ($res!="\x39\x79") {
			$this->sock=false; // wrong socket
			return false; // wrong answer Oo
		}
		$len=fread($this->sock,2); // packet len
		$len=unpack('vint',$len);
		$len=$len['int'];
//		$buf=fread($this->sock,43); // useless data
		$len-=4; // 4 is the len of packet code + packet len
		$numsrv=$len/32;
		if (floor($numsrv)!=$numsrv) {
			$this->sock=false;
			return false; // should not happen, again
		}
		$res=array();
		if ($len<1) return $res; // no servers
		$buf=fread($this->sock,$len);
		while(strlen($buf)>0) {
			$data=substr($buf,0,32);
			$buf=substr($buf,32);
			$dat=array();
			$dat=unpack('Nip/vport/a20name/vusers/vmaintenance/vnew',$data);
			$dat['ip']=long2ip($dat['ip']);
			$pos=strpos($dat['name'],"\0");
			if ($pos) $dat['name']=substr($dat['name'],0,$pos);
			$dat['version']=1;
			$res[]=$dat;
		}
		return $res;
	}

	function checkaccount($login, $pass) {
		if (!$this->sock) return false;
		$packet = pack('va24a24', 0x793a, $login, $pass);
		fwrite($this->sock, $packet);
		$res = fread($this->sock, 30);
		$dat = unpack('vpacket/Vid/a24accname', $res);
		if ($dat['id']==-1) return false;
		return $dat;
	}

	function accountinfo($id) {
		// ferch info from this account
		if (!$this->sock) return false;
		$packet = pack('vV', 0x7954, $id);
		fwrite($this->sock, $packet);
		$res = fread($this->sock, 150);
		$dat = unpack('vpacket/Vid/clevel/a24accname/csex/Vlogincount/vstate/cisonline/cnotused/a20error_msg/a24last_login/a16last_ip/a40email/Vaccexpire/Vbanexpire/vmemolen', $res);
		if ($dat['memolen'] > 0) {
			$dat['memo'] = fread($this->sock, $dat['memolen']);
		} else {
			$dat['memo'] = null;
		}
		if ($dat['packet'] != 0x7953) return false;
		if ($dat['accname'] === '') return false;
		return $dat;
	}

	function changepass($accname, $newpass) {
		if (!$this->sock) return false;
		$packet = pack('va24a24', 0x7934, $accname, $newpass);
		fwrite($this->sock, $packet);
		$res = fread($this->sock, 30);
		$dat = unpack('vpacket/Vid/a24accname', $res);
		if ($dat['packet'] != 0x7935) { fclose($this->sock); $this->sock=false; return false; }
		if ($dat['id']==-1) return false;
		return true;
	}

	function sendbroadcast($msg, $blue=false) {
		$blue=($blue)?1:0;
		$packet = pack('vvv', 0x794e, 6+strlen($msg)+1, $blue). $msg."\0";
		fwrite($this->sock, $packet);
		$res = fread($this->sock, 4);
		$dat = unpack('vpacket/vres', $res);
		if ($dat['res'] == -1) return false;
		return true;
	}
}

